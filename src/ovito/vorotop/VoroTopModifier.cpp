////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
//  Copyright 2017 Emanuel A. Lazar
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify it either under the
//  terms of the GNU General Public License version 3 as published by the Free Software
//  Foundation (the "GPL") or, at your option, under the terms of the MIT License.
//  If you do not alter this notice, a recipient may use your version of this
//  file under either the GPL or the MIT License.
//
//  You should have received a copy of the GPL along with this program in a
//  file LICENSE.GPL.txt.  You should have received a copy of the MIT License along
//  with this program in a file LICENSE.MIT.txt
//
//  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
//  either express or implied. See the GPL or the MIT License for the specific language
//  governing rights and limitations.
//
////////////////////////////////////////////////////////////////////////////////////////

#include <ovito/vorotop/VoroTopPlugin.h>
#include <ovito/particles/util/NearestNeighborFinder.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/utilities/concurrent/ParallelFor.h>
#include <ovito/core/utilities/concurrent/Task.h>
#include <ovito/core/utilities/concurrent/AsyncOperation.h>
#include <ovito/core/utilities/io/CompressedTextReader.h>
#include <ovito/core/utilities/io/FileManager.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include "VoroTopModifier.h"
#include "Filter.h"

#include <voro++.hh>

namespace Ovito { namespace VoroTop {

IMPLEMENT_OVITO_CLASS(VoroTopModifier);
DEFINE_PROPERTY_FIELD(VoroTopModifier, useRadii);
DEFINE_PROPERTY_FIELD(VoroTopModifier, filterFile);
SET_PROPERTY_FIELD_LABEL(VoroTopModifier, useRadii, "Use particle radii");
SET_PROPERTY_FIELD_LABEL(VoroTopModifier, filterFile, "Filter file");

/******************************************************************************
 * Constructs the modifier object.
 ******************************************************************************/
VoroTopModifier::VoroTopModifier(DataSet* dataset) : StructureIdentificationModifier(dataset),
       _useRadii(false)
{
}

/******************************************************************************
 * Loads a new filter definition into the modifier.
 ******************************************************************************/
bool VoroTopModifier::loadFilterDefinition(const QString& filepath, AsyncOperation&& operation)
{
    operation.setProgressText(tr("Loading VoroTop filter %1").arg(filepath));

    // Open filter file for reading.
    FileHandle fileHandle(QUrl::fromLocalFile(filepath), filepath);
    CompressedTextReader stream(fileHandle);

    // Load filter file header (i.e. list of structure types).
    std::shared_ptr<Filter> filter = std::make_shared<Filter>();
    if(!filter->load(stream, true, *operation.task()))
        return false;

    // Rebuild structure types list.
    setStructureTypes({});
    for(int i = 0; i < filter->structureTypeCount(); i++) {
        OORef<ParticleType> stype(new ParticleType(dataset()));
        stype->setNumericId(i);
        stype->setName(filter->structureTypeLabel(i));
        stype->setColor(ParticleType::getDefaultParticleColor(ParticlesObject::StructureTypeProperty, stype->name(), i));
        addStructureType(stype);
    }

    // Filter file was successfully loaded. Accept it as the new filter.
    setFilterFile(filepath);

    return !operation.isCanceled();
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the
* modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> VoroTopModifier::createEngine(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input)
{
    // Get the current positions.
    const ParticlesObject* particles = input.expectObject<ParticlesObject>();
	particles->verifyIntegrity();
    const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);

	// The Voro++ library uses 32-bit integers. It cannot handle more than 2^31 input points.
	if(particles->elementCount() > std::numeric_limits<int>::max())
        throwException(tr("VoroTop analysis modifier is limited to a maximum of %1 particles in the current program version.").arg(std::numeric_limits<int>::max()));

    // Get simulation cell.
    const SimulationCellObject* inputCell = input.expectObject<SimulationCellObject>();

    // Get selection particle property.
    ConstPropertyPtr selectionProperty;
    if(onlySelectedParticles())
        selectionProperty = particles->expectProperty(ParticlesObject::SelectionProperty)->storage();

    // Get particle radii.
    std::vector<FloatType> radii;
    if(useRadii())
        radii = particles->inputParticleRadii();

    // Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
    return std::make_shared<VoroTopAnalysisEngine>(particles,
                                                   input.stateValidity(),
                                                   posProperty->storage(),
                                                   std::move(selectionProperty),
                                                   std::move(radii),
                                                   inputCell->data(),
                                                   filterFile(),
                                                   filter(),
                                                   getTypesToIdentify(structureTypes().size()));
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
void VoroTopModifier::VoroTopAnalysisEngine::emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	StructureIdentificationEngine::emitResults(time, modApp, state);

    // Cache loaded filter definition for future use.
    static_object_cast<VoroTopModifier>(modApp->modifier())->_filter = this->filter();

    state.setStatus(PipelineStatus(PipelineStatus::Success, tr("%1 Weinberg vectors loaded").arg(filter() ? filter()->size() : 0)));
}

/******************************************************************************
 * Processes a single Voronoi cell.
 ******************************************************************************/
int VoroTopModifier::VoroTopAnalysisEngine::processCell(voro::voronoicell_neighbor& vcell)
{
    const int max_epf = 256;    // MAXIMUM EDGES PER FACE
    const int max_epc = 512;    // MAXIMUM EDGES PER CELL
    const int max_vpc = 512;    // MAXIMUM VERTICES PER CELL

    int   edge_count     = vcell.number_of_edges();
    int   vertex_count   = vcell.p;    // TOTAL NUMBER OF VERTICES
    int*  vertex_degrees = vcell.nu;   // VERTEX DEGREE ARRAY
    int** ed             = vcell.ed;   // EDGE CONNECTIONS ARRAY

    // TOO MANY VERTICES OR EDGES
    if(vertex_count > filter()->maximumVertices ||
       edge_count   > filter()->maximumEdges    ||
       vertex_count >= max_vpc                  ||
       edge_count   >= max_epc)
    {
        return 0; // structureType OTHER
    }

    int face_count         = 0;
    int max_face_edges     = 3;     // EVERY CONVEX POLYHEDRON MUST HAVE AT LEAST ONE FACE WITH 3 OR MORE EDGES
    int min_face_edges     = 5;     // EVERY CONVEX POLYHEDRON MUST HAVE AT LEAST ONE FACE WITH 5 OR FEWER EDGES
    int pvector[max_epf]   = {};    // RECORDS NUMBER OF FACES WITH EACH NUMBER OF EDGES, NO FACE IN FILTER HAS MORE THAN max_epf-1 EDGES
    int origins[2*max_epc] = {};    // NO VORONOI CELL IN FILTER HAS MORE THAN max_epc EDGES
    int origin_c           = 0;

    // DETERMINE VERTICES ON FACES WITH MINIMAL EDGES
    for(int i=0;i<vertex_count;i++)
    {
        for(int j=0;j<vertex_degrees[i];j++)
        {
            int k = ed[i][j];
            if(k >= 0)
            {
                int face[max_epf]={};  // NO SINGLE FACE WILL HAVE MORE THAN max_epf EDGES
                int face_c=0;

                ed[i][j]=-1-k;      // INDICATE THAT WE HAVE CHECKED THIS VERTEX
                int l=vcell.cycle_up(ed[i][vertex_degrees[i]+j],k);
                face[face_c++]=k;
                do {
                    int m=ed[k][l];
                    ed[k][l]=-1-m;
                    l=vcell.cycle_up(ed[k][vertex_degrees[k]+l],m);
                    k=m;

                    face[face_c++]=m;
                } while (k!=i);

                // KEEP TRACK OF MINIMAL AND MAXIMAL FACE EDGES
                if(face_c>max_face_edges)
                    max_face_edges = face_c;
                if(face_c<min_face_edges)
                {
                    min_face_edges = origin_c = face_c;
                    for(int c=0; c<face_c; c++)
                        origins[c] = face[c];
                }
                else if(face_c==min_face_edges)
                {
                    for(int c=0; c<face_c; c++)
                        origins[origin_c+c] = face[c];
                    origin_c += face_c;
                }
                pvector[face_c]++;
                face_count++;
            }
        }
    }

    // RESET EDGES
    for(int i=0;i<vertex_count;i++)
        for(int j=0;j<vertex_degrees[i];j++)
            ed[i][j]=-1-ed[i][j];

    // KEEPING TRACK OF THIS WILL ALLOW US TO SPEED UP SOME COMPUTATION, OF BCC
    int likely_bcc=0;
    if(face_count==14 && pvector[4]==6 && pvector[6]==8) likely_bcc=1;   // THIS PVECTOR (0,6,0,8,0,...) OF A SIMPLE POLYHEDRON APPEARS IN 3 DIFFERENT TYPES, WITH SYMMETRIES 4, 8, AND 48


    ////////////////////////////////////////////////////////////////
    // BUILD THE CANONICAL CODE
    ////////////////////////////////////////////////////////////////

    using WeinbergVector = Filter::WeinbergVector;
    WeinbergVector canonical_code(2*edge_count,0);  // CANONICAL CODE WILL BE STORED HERE
    int vertices_temp_labels[max_vpc] = {};         // TEMPORARY LABELS FOR ALL VERTICES; MAX max_vpc VERTICES

    int finished   =  0;
    int chirality  = -1;
    int symmetry_counter = 0;     // TRACKS NUMBER OF REPEATS OF A CODE, I.E. SYMMETRY ORDER

    for(int orientation=0; orientation<2 && finished==0; orientation++)
    {
        for(int q=0; q<origin_c && finished==0; q++)
        {
            // CLEAR ALL LABELS; MARK ALL BRANCHES OF ALL VERTICES AS NEW
            std::fill(vertices_temp_labels, vertices_temp_labels+vertex_count, 0);

            for(int i=0;i<vertex_count;i++)
                for(int j=0;j<vertex_degrees[i];j++)
                    if(ed[i][j]<0) ed[i][j]=-1-ed[i][j];

            int initial = origins[q];
            int next;
            int branch;

            if(orientation==0)
            {
                if((q+1)%min_face_edges==0) next = origins[q - min_face_edges + 1];
                else next = origins[q + 1];
            }
            else
            {
                if(q    %min_face_edges==0) next = origins[q + min_face_edges - 1];
                else next = origins[q - 1];
            }
            for(int j=0; j<vertex_degrees[origins[q]]; j++)
                if(ed[origins[q]][j]==next) branch=j;
            ed[initial][branch] = -1-next;

            int current_code_length   = 0;
            int current_highest_label = 1;
            int continue_code         = 0;    // 0: UNDECIDED; 1: GO AHEAD, DO NOT EVEN CHECK.
            if(q==0 && orientation==0)        // FIRST CODE, GO AHEAD
                continue_code=1;

            vertices_temp_labels[initial] = current_highest_label++;
            canonical_code[current_code_length]  = vertices_temp_labels[initial];
            current_code_length++;

            // BUILD EACH CODE FOLLOWING WEINBERG'S RULES FOR TRAVERSING A GRAPH TO BUILD
            // A HAMILTONIAN PATH, LABELING VERTICES ALONG THE WAY, AND RECORDING VERTICES
            // AS VISITED.
            int end_flag=0;
            while(end_flag==0)
            {
                // NEXT VERTEX HAS NOT BEEN VISITED; TAKE RIGHT-MOST BRANCH TO CONTINUE.
                if(vertices_temp_labels[next]==0)
                {
                    // LABEL THE NEW VERTEX
                    vertices_temp_labels[next] = current_highest_label++;

                    if(continue_code==0)
                    {
                        if(vertices_temp_labels[next]>canonical_code[current_code_length]) break;
                        if(vertices_temp_labels[next]<canonical_code[current_code_length])
                        {
                            symmetry_counter = 0;
                            continue_code    = 1;
                            if(orientation==1) chirality=1;
                        }
                    }

                    // BUILD THE CODE
                    canonical_code[current_code_length] = vertices_temp_labels[next];
                    current_code_length++;

                    // FIND NEXT DIRECTION TO MOVE ALONG, UPDATE, AND RELOOP
                    if(orientation==0) branch  = vcell.cycle_up  (ed[initial][vertex_degrees[initial]+branch],next);
                    else               branch  = vcell.cycle_down(ed[initial][vertex_degrees[initial]+branch],next);
                    initial = next;
                    next    = ed[initial][branch];
                    ed[initial][branch] = -1-next;
                }

                else    // NEXT VERTEX *HAS* BEEN VISITED BEFORE
                {
                    int next_branch = ed[initial][vertex_degrees[initial]+branch];
                    int branches_tested = 0;

                    while(ed[next][next_branch] < 0 && branches_tested<vertex_degrees[next])
                    {
                        if(orientation==0) next_branch = vcell.cycle_up  (next_branch,next);
                        else               next_branch = vcell.cycle_down(next_branch,next);

                        branches_tested++;
                    }

                    if(branches_tested < vertex_degrees[next])
                    {
                        if(continue_code==0)
                        {
                            if(vertices_temp_labels[next]>canonical_code[current_code_length]) break;
                            if(vertices_temp_labels[next]<canonical_code[current_code_length])
                            {
                                symmetry_counter = 0;
                                continue_code    = 1;
                                if(orientation==1) chirality=1;
                            }
                        }

                        // BUILD THE CODE
                        canonical_code[current_code_length] = vertices_temp_labels[next];
                        current_code_length++;

                        // FIND NEXT BRANCH
                        branch  = next_branch;
                        initial = next;
                        next    = ed[initial][branch];
                        ed[initial][branch] = -1-next;
                    }

                    else
                    {
                        end_flag=1;

                        if(likely_bcc && symmetry_counter>4 && orientation==0) { chirality=0; symmetry_counter = 48; finished=1; }
                        else if(chirality==-1 && orientation==1)               { chirality=0; symmetry_counter *= 2; finished=1; }
                        else symmetry_counter++;
                    }
                }
            }
        }
    }

    canonical_code.push_back(1);

    return filter()->findType(canonical_code);
}

/******************************************************************************
 * Performs the actual computation. This method is executed in a worker thread.
 ******************************************************************************/
void VoroTopModifier::VoroTopAnalysisEngine::perform()
{
    if(!filter()) {
        if(_filterFile.isEmpty())
            throw Exception(tr("No filter file selected"));
        task()->setProgressText(tr("Loading VoroTop filter file: %1").arg(_filterFile));

        // Open filter file for reading.
        FileHandle fileHandle(QUrl::fromLocalFile(_filterFile), _filterFile);
        CompressedTextReader stream(fileHandle);

        // Parse filter definition.
        _filter = std::make_shared<Filter>();
        if(!_filter->load(stream, false, *task()))
            return;
    }

    if(positions()->size() == 0)
        return;	// Nothing to do when there are zero particles.

    task()->setProgressText(tr("Performing VoroTop analysis"));

    ConstPropertyAccess<Point3> positionsArray(positions());
    ConstPropertyAccess<int> selectionArray(selection());
    PropertyAccess<int> structuresArray(structures());

    // Decide whether to use Voro++ container class or our own implementation.
    if(cell().isAxisAligned()) {
        // Use Voro++ container.
        double ax = cell().matrix()(0,3);
        double ay = cell().matrix()(1,3);
        double az = cell().matrix()(2,3);
        double bx = ax + cell().matrix()(0,0);
        double by = ay + cell().matrix()(1,1);
        double bz = az + cell().matrix()(2,2);
        if(ax > bx) std::swap(ax,bx);
        if(ay > by) std::swap(ay,by);
        if(az > bz) std::swap(az,bz);
        double volumePerCell = (bx - ax) * (by - ay) * (bz - az) * voro::optimal_particles / positions()->size();
        double cellSize = pow(volumePerCell, 1.0/3.0);
        int nx = (int)std::ceil((bx - ax) / cellSize);
        int ny = (int)std::ceil((by - ay) / cellSize);
        int nz = (int)std::ceil((bz - az) / cellSize);

        if(_radii.empty()) {
            voro::container voroContainer(ax, bx, ay, by, az, bz, nx, ny, nz,
                                          cell().pbcFlags()[0], cell().pbcFlags()[1], cell().pbcFlags()[2], (int)std::ceil(voro::optimal_particles));

            // Insert particles into Voro++ container.
            size_t count = 0;
            for(size_t index = 0; index < positions()->size(); index++) {
                // Skip unselected particles (if requested).
                if(selectionArray && selectionArray[index] == 0) {
                    structuresArray[index] = 0;
                    continue;
                }
                const Point3& p = positionsArray[index];
                voroContainer.put(index, p.x(), p.y(), p.z());
                count++;
            }
            if(!count) return;

            task()->setProgressMaximum(count);
            task()->setProgressValue(0);
            voro::c_loop_all cl(voroContainer);
            voro::voronoicell_neighbor v;
            if(cl.start()) {
                do {
                    if(!task()->incrementProgressValue())
                        return;
                    if(!voroContainer.compute_cell(v,cl))
                        continue;
                    structuresArray[cl.pid()] = processCell(v);
                    count--;
                }
                while(cl.inc());
            }
            if(count)
                throw Exception(tr("Could not compute Voronoi cell for some particles."));
        }
        else {
            voro::container_poly voroContainer(ax, bx, ay, by, az, bz, nx, ny, nz,
                                               cell().pbcFlags()[0], cell().pbcFlags()[1], cell().pbcFlags()[2], (int)std::ceil(voro::optimal_particles));

            // Insert particles into Voro++ container.
            size_t count = 0;
            for(size_t index = 0; index < positions()->size(); index++) {
                structuresArray[index] = 0;
                // Skip unselected particles (if requested).
                if(selectionArray && selectionArray[index] == 0) {
                    continue;
                }
                const Point3& p = positionsArray[index];
                voroContainer.put(index, p.x(), p.y(), p.z(), _radii[index]);
                count++;
            }

            if(!count) return;
            task()->setProgressMaximum(count);
            task()->setProgressValue(0);
            voro::c_loop_all cl(voroContainer);
            voro::voronoicell_neighbor v;
            if(cl.start()) {
                do {
                    if(!task()->incrementProgressValue())
                        return;
                    if(!voroContainer.compute_cell(v,cl))
                        continue;
                    structuresArray[cl.pid()] = processCell(v);
                    count--;
                }
                while(cl.inc());
            }
            if(count)
                throw Exception(tr("Could not compute Voronoi cell for some particles."));
        }
    }
    else {
        // Prepare the nearest neighbor list generator.
        NearestNeighborFinder nearestNeighborFinder;
        if(!nearestNeighborFinder.prepare(positions(), cell(), selection(), task().get()))
            return;

        // Squared particle radii (input was just radii).
        for(auto& r : _radii)
            r = r*r;

        // This is the size we use to initialize Voronoi cells. Must be larger than the simulation box.
        double boxDiameter = sqrt(
                                  cell().matrix().column(0).squaredLength()
                                  + cell().matrix().column(1).squaredLength()
                                  + cell().matrix().column(2).squaredLength());

        // The normal vectors of the three cell planes.
        std::array<Vector3,3> planeNormals;
        planeNormals[0] = cell().cellNormalVector(0);
        planeNormals[1] = cell().cellNormalVector(1);
        planeNormals[2] = cell().cellNormalVector(2);

        Point3 corner1 = Point3::Origin() + cell().matrix().column(3);
        Point3 corner2 = corner1 + cell().matrix().column(0) + cell().matrix().column(1) + cell().matrix().column(2);

        // Perform analysis, particle-wise parallel.
        parallelFor(positions()->size(), *task(), [&](size_t index) {

                        // Reset structure type.
                        structuresArray[index] = 0;

                        // Skip unselected particles (if requested).
                        if(selectionArray && selectionArray[index] == 0)
                            return;

                        // Build Voronoi cell.
                        voro::voronoicell_neighbor v;

                        // Initialize the Voronoi cell to be a cube larger than the simulation cell, centered at the origin.
                        v.init(-boxDiameter, boxDiameter, -boxDiameter, boxDiameter, -boxDiameter, boxDiameter);

                        // Cut Voronoi cell at simulation cell boundaries in non-periodic directions.
                        bool skipParticle = false;
                        for(size_t dim = 0; dim < 3; dim++) {
                            if(!cell().pbcFlags()[dim]) {
                                double r;
                                r = 2 * planeNormals[dim].dot(corner2 - positionsArray[index]);
                                if(r <= 0) skipParticle = true;
                                v.nplane(planeNormals[dim].x() * r, planeNormals[dim].y() * r, planeNormals[dim].z() * r, r*r, -1);
                                r = 2 * planeNormals[dim].dot(positionsArray[index] - corner1);
                                if(r <= 0) skipParticle = true;
                                v.nplane(-planeNormals[dim].x() * r, -planeNormals[dim].y() * r, -planeNormals[dim].z() * r, r*r, -1);
                            }
                        }
                        // Skip particles that are located outside of non-periodic box boundaries.
                        if(skipParticle)
                            return;

                        // This function will be called for every neighbor particle.
                        int nvisits = 0;
                        auto visitFunc = [&](const NearestNeighborFinder::Neighbor& n, FloatType& mrs) {
                            // Skip unselected particles (if requested).
                            OVITO_ASSERT(!selectionArray || selectionArray[n.index]);
                            FloatType rs = n.distanceSq;
                            if(!_radii.empty())
                                rs += _radii[index] - _radii[n.index];
                            v.nplane(n.delta.x(), n.delta.y(), n.delta.z(), rs, n.index);
                            if(nvisits == 0) {
                                mrs = v.max_radius_squared();
                                nvisits = 100;
                            }
                            nvisits--;
                        };

                        // Visit all neighbors of the current particles.
                        nearestNeighborFinder.visitNeighbors(nearestNeighborFinder.particlePos(index), visitFunc);

                        structuresArray[index] = processCell(v);
                    });
    }
}

/******************************************************************************
 * Is called when the value of a property of this object has changed.
 ******************************************************************************/
void VoroTopModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
    StructureIdentificationModifier::propertyChanged(field);

    // Throw away loaded filter definition whenever a new filter file has been selected.
    if(field == PROPERTY_FIELD(VoroTopModifier::filterFile))
        _filter.reset();
}

}	// End of namespace
}	// End of namespace
