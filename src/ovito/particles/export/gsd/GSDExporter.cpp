////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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

#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/import/gsd/GSDFile.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/utilities/concurrent/Promise.h>
#include <ovito/core/utilities/concurrent/AsyncOperation.h>
#include "GSDExporter.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Export) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

IMPLEMENT_OVITO_CLASS(GSDExporter);

/******************************************************************************
 * Constructor.
 *****************************************************************************/
GSDExporter::GSDExporter(DataSet* dataset) : ParticleExporter(dataset)
{
}

/******************************************************************************
 * Destructor.
 *****************************************************************************/
GSDExporter::~GSDExporter()
{
}

/******************************************************************************
 * This is called once for every output file to be written and before
 * exportFrame() is called.
 *****************************************************************************/
bool GSDExporter::openOutputFile(const QString& filePath, int numberOfFrames, AsyncOperation& operation)
{
    OVITO_ASSERT(!outputFile().isOpen());
    outputFile().setFileName(filePath);

    // Open the input file for writing.
    _gsdFile = GSDFile::create(QDir::toNativeSeparators(filePath).toLocal8Bit().constData(),
                    "ovito", "hoomd", 1, 4);

    return true;
}

/******************************************************************************
 * This is called once for every output file written after exportFrame()
 * has been called.
 *****************************************************************************/
void GSDExporter::closeOutputFile(bool exportCompleted)
{
    OVITO_ASSERT(!outputFile().isOpen());

    // Close the output file.
    _gsdFile.reset();

    if(!exportCompleted)
        outputFile().remove();
}

/******************************************************************************
* Writes the particles of one animation frame to the current output file.
******************************************************************************/
bool GSDExporter::exportData(const PipelineFlowState& state, int frameNumber, TimePoint time, const QString& filePath, AsyncOperation&& operation)
{
    // Get particles.
    const ParticlesObject* particles = state.expectObject<ParticlesObject>();
    particles->verifyIntegrity();

    // Get simulation cell info.
    const SimulationCellObject* simulationCellObj = state.expectObject<SimulationCellObject>();
    const SimulationCell cell = simulationCellObj->data();
    const AffineTransformation& simCell = cell.matrix();

    // Output simulation step.
    uint64_t timestep = state.getAttributeValue(QStringLiteral("Timestep"), frameNumber).toLongLong();
    _gsdFile->writeChunk<uint64_t>("configuration/step", 1, 1, &timestep);

    // Output dimensionality of the particle system.
    if(cell.is2D()) {
        uint8_t dimensionality = 2;
        _gsdFile->writeChunk<uint8_t>("configuration/dimensions", 1, 1, &dimensionality);
    }

    // Transform triclinic simulation cells to HOOMD canonical format.
    AffineTransformation hoomdCell;
    hoomdCell(0,0) = simCell.column(0).length();
    hoomdCell(1,0) = hoomdCell(2,0) = 0;
    hoomdCell(0,1) = simCell.column(1).dot(simCell.column(0)) / hoomdCell(0,0);
    hoomdCell(1,1) = sqrt(simCell.column(1).squaredLength() - hoomdCell(0,1)*hoomdCell(0,1));
    hoomdCell(2,1) = 0;
    hoomdCell(0,2) = simCell.column(2).dot(simCell.column(0)) / hoomdCell(0,0);
    hoomdCell(1,2) = (simCell.column(1).dot(simCell.column(2)) - hoomdCell(0,1)*hoomdCell(0,2)) / hoomdCell(1,1);
    hoomdCell(2,2) = sqrt(simCell.column(2).squaredLength() - hoomdCell(0,2)*hoomdCell(0,2) - hoomdCell(1,2)*hoomdCell(1,2));
    hoomdCell.translation() = hoomdCell.linear() * Vector3(FloatType(-0.5));
    AffineTransformation transformation = hoomdCell * simCell.inverse();

    // Output simulation cell geometry.
    float box[6] = { (float)hoomdCell(0,0), (float)hoomdCell.column(1).length(), (float)hoomdCell.column(2).length(),
                    (float)(hoomdCell(0,1) / hoomdCell.column(1).length()),		// xy
                    (float)(hoomdCell(0,2) / hoomdCell.column(2).length()),		// xz
                    (float)(hoomdCell(1,2) / hoomdCell.column(2).length()) };	// yz
    _gsdFile->writeChunk<float>("configuration/box", 6, 1, box);

    // Output number of particles.
    if(particles->elementCount() > (size_t)std::numeric_limits<uint32_t>::max())
        throwException(tr("Number of particles exceeds maximum number supported by the GSD/HOOMD format."));
    uint32_t particleCount = particles->elementCount();
    _gsdFile->writeChunk<uint32_t>("particles/N", 1, 1, &particleCount);
    if(operation.isCanceled()) return false;

    // Determine particle ordering.
    std::vector<size_t> ordering(particles->elementCount());
    std::iota(ordering.begin(), ordering.end(), (size_t)0);
    if(const PropertyObject* idProperty = particles->getProperty(ParticlesObject::IdentifierProperty)) {
        std::sort(ordering.begin(), ordering.end(), [&](size_t a, size_t b) { return idProperty->getInt64(a) < idProperty->getInt64(b); });
    }
    if(operation.isCanceled()) return false;

    // Output particle coordinates.
    const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
    // Apply coordinate transformation matrix, wrapping a periodic box boundaries and data type conversion:
    std::vector<Point_3<float>> posBuffer(posProperty->size());
    std::vector<Vector_3<int32_t>> imageBuffer(posProperty->size());
    for(size_t i = 0; i < ordering.size(); i++) {
        const Point3& p = posProperty->getPoint3(ordering[i]);
		for(size_t dim = 0; dim < 3; dim++) {
            FloatType s = std::floor(cell.inverseMatrix().prodrow(p, dim));
            posBuffer[i][dim] = transformation.prodrow(p - s * cell.matrix().column(dim), dim);
            imageBuffer[i][dim] = s;
  		}
    }
    _gsdFile->writeChunk<float>("particles/position", posBuffer.size(), 3, posBuffer.data());
    if(operation.isCanceled()) return false;
    _gsdFile->writeChunk<int32_t>("particles/image", imageBuffer.size(), 3, imageBuffer.data());
    if(operation.isCanceled()) return false;

    // Output particle types.
    if(const PropertyObject* typeProperty = particles->getProperty(ParticlesObject::TypeProperty)) {

        // GSD/HOOMD requires particle types to form a contiguous range starting at base index 0.
        std::map<int,int> idMapping;
        ConstPropertyPtr typeIds;
        std::tie(idMapping, typeIds) = typeProperty->generateContiguousTypeIdMapping(0);

        // Build list of type names.
        std::vector<QByteArray> typeNames(idMapping.size());
        int maxStringLength = 0;
        for(size_t i = 0; i < typeNames.size(); i++) {
            OVITO_ASSERT(idMapping.find(i) != idMapping.end());
            if(const ElementType* ptype = typeProperty->elementType(idMapping[i]))
                typeNames[i] = ptype->name().toUtf8();
            if(typeNames[i].size() == 0 && i < 26)
                typeNames[i] = QByteArray(1, 'A' + (char)i);
            maxStringLength = std::max(maxStringLength, typeNames[i].size());
        }
        maxStringLength++; // Include terminating null character.
        std::vector<int8_t> typeNameBuffer(maxStringLength * typeNames.size(), 0);
        for(size_t i = 0; i < typeNames.size(); i++) {
            std::copy(typeNames[i].cbegin(), typeNames[i].cend(), typeNameBuffer.begin() + (i * maxStringLength));
        }
        _gsdFile->writeChunk<int8_t>("particles/types", typeNames.size(), maxStringLength, typeNameBuffer.data());

        // Build typeid array.
        std::vector<uint32_t> typeIdBuffer(typeIds->size());
        std::transform(ordering.cbegin(), ordering.cend(), typeIdBuffer.begin(),
            [&](size_t i) { return typeIds->getInt(i); });
        _gsdFile->writeChunk<uint32_t>("particles/typeid", typeIdBuffer.size(), 1, typeIdBuffer.data());
        if(operation.isCanceled()) return false;
    }

    // Output particle masses.
    if(const PropertyObject* massProperty = particles->getProperty(ParticlesObject::MassProperty)) {
        // Apply particle index mapping and data type conversion:
        std::vector<float> massBuffer(massProperty->size());
        std::transform(ordering.cbegin(), ordering.cend(), massBuffer.begin(),
            [&](size_t i) { return massProperty->getFloat(i); });
        _gsdFile->writeChunk<float>("particles/mass", massBuffer.size(), 1, massBuffer.data());
        if(operation.isCanceled()) return false;
    }

    // Output particle charges.
    if(const PropertyObject* chargeProperty = particles->getProperty(ParticlesObject::ChargeProperty)) {
        // Apply particle index mapping and data type conversion:
        std::vector<float> chargeBuffer(chargeProperty->size());
        std::transform(ordering.cbegin(), ordering.cend(), chargeBuffer.begin(),
            [&](size_t i) { return chargeProperty->getFloat(i); });
        _gsdFile->writeChunk<float>("particles/charge", chargeBuffer.size(), 1, chargeBuffer.data());
        if(operation.isCanceled()) return false;
    }

    // Output particle diameters.
    if(const PropertyObject* radiusProperty = particles->getProperty(ParticlesObject::RadiusProperty)) {
        // Apply particle index mapping, data type conversion and
        // multiplying with a factor of 2 to convert from radii to diameters:
        std::vector<float> diameterBuffer(radiusProperty->size());
        std::transform(ordering.cbegin(), ordering.cend(), diameterBuffer.begin(),
            [&](size_t i) { return 2 * radiusProperty->getFloat(i); });
        _gsdFile->writeChunk<float>("particles/diameter", diameterBuffer.size(), 1, diameterBuffer.data());
        if(operation.isCanceled()) return false;
    }

    // Output particle orientations.
    if(const PropertyObject* orientationProperty = particles->getProperty(ParticlesObject::OrientationProperty)) {
        // Apply particle index mapping and data type conversion.
        // Also right-shift the quaternion components, because GSD uses a different representation.
        // (X,Y,Z,W) -> (W,X,Y,Z).
        std::vector<std::array<float,4>> orientationBuffer(orientationProperty->size());
        std::transform(ordering.cbegin(), ordering.cend(), orientationBuffer.begin(),
            [&](size_t i) { const Quaternion& q = orientationProperty->getQuaternion(i);
                return std::array<float,4>{{ (float)q.w(), (float)q.x(), (float)q.y(), (float)q.z() }}; });
        _gsdFile->writeChunk<float>("particles/orientation", orientationBuffer.size(), 4, orientationBuffer.data());
        if(operation.isCanceled()) return false;
    }

    // Output particle velocities.
    if(const PropertyObject* velocityProperty = particles->getProperty(ParticlesObject::VelocityProperty)) {
        // Apply particle index mapping and data type conversion:
        // Also apply affine transform of simulation cell to velocity vectors.
        std::vector<Vector_3<float>> velocityBuffer(velocityProperty->size());
        std::transform(ordering.cbegin(), ordering.cend(), velocityBuffer.begin(),
            [&](size_t i) { return Vector_3<float>(transformation * velocityProperty->getVector3(i)); });
        _gsdFile->writeChunk<float>("particles/velocity", velocityBuffer.size(), 3, velocityBuffer.data());
        if(operation.isCanceled()) return false;
    }

    // See if there any bonds to be exported.
	if(const BondsObject* bonds = particles->bonds()) {
        bonds->verifyIntegrity();
    	const PropertyObject* bondTopologyProperty = bonds->expectProperty(BondsObject::TopologyProperty);

        // Output number of bonds.
        if(bonds->elementCount() > (size_t)std::numeric_limits<uint32_t>::max())
            throwException(tr("Number of bonds exceeds maximum number supported by the GSD/HOOMD format."));
        uint32_t bondsCount = bonds->elementCount();
        _gsdFile->writeChunk<uint32_t>("bonds/N", 1, 1, &bondsCount);
        if(operation.isCanceled()) return false;

        // Build reverse mapping of particle indices.
        std::vector<size_t> reverseOrdering(ordering.size());
        for(size_t i = 0; i < ordering.size(); i++)
            reverseOrdering[ordering[i]] = i;

        // Output topology array.
        std::vector<std::array<uint32_t,2>> bondsBuffer(bondTopologyProperty->size());
        for(size_t i = 0; i < bondTopologyProperty->size(); i++) {
            size_t a = bondTopologyProperty->getInt64Component(i, 0);
            size_t b = bondTopologyProperty->getInt64Component(i, 1);
            if(a >= reverseOrdering.size() || b >= reverseOrdering.size())
                throwException(tr("GSD/HOOMD file export error: Bond topology entry is out of range."));
            bondsBuffer[i][0] = reverseOrdering[a];
            bondsBuffer[i][1] = reverseOrdering[b];
        }
        _gsdFile->writeChunk<uint32_t>("bonds/group", bondsBuffer.size(), 2, bondsBuffer.data());
        if(operation.isCanceled()) return false;

        // Output bond types.
        if(const PropertyObject* typeProperty = bonds->getProperty(BondsObject::TypeProperty)) {

            // GSD/HOOMD requires bond types to form a contiguous range starting at base index 0.
            std::map<int,int> idMapping;
            ConstPropertyPtr typeIds;
            std::tie(idMapping, typeIds) = typeProperty->generateContiguousTypeIdMapping(0);

            // Build list of type names.
            std::vector<QByteArray> typeNames(idMapping.size());
            int maxStringLength = 0;
            for(size_t i = 0; i < typeNames.size(); i++) {
                OVITO_ASSERT(idMapping.find(i) != idMapping.end());
                if(const ElementType* ptype = typeProperty->elementType(idMapping[i]))
                    typeNames[i] = ptype->name().toUtf8();
                if(typeNames[i].size() == 0 && i < 26)
                    typeNames[i] = QByteArray(1, 'A' + (char)i);
                maxStringLength = std::max(maxStringLength, typeNames[i].size());
            }
            maxStringLength++; // Include terminating null character.
            std::vector<int8_t> typeNameBuffer(maxStringLength * typeNames.size(), 0);
            for(size_t i = 0; i < typeNames.size(); i++) {
                std::copy(typeNames[i].cbegin(), typeNames[i].cend(), typeNameBuffer.begin() + (i * maxStringLength));
            }
            _gsdFile->writeChunk<int8_t>("bonds/types", typeNames.size(), maxStringLength, typeNameBuffer.data());

            // Output typeid array.
            _gsdFile->writeChunk<uint32_t>("bonds/typeid", typeIds->size(), 1, typeIds->constDataInt());
            if(operation.isCanceled()) return false;
        }
    }

    // Close the current frame that has been written to the GSD file.
    _gsdFile->endFrame();

    return !operation.isCanceled();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
