///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2019) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/crystalanalysis/objects/Microstructure.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/utilities/concurrent/ParallelFor.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include "SimplifyMicrostructureModifier.h"

namespace Ovito { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(SimplifyMicrostructureModifier);
DEFINE_PROPERTY_FIELD(SimplifyMicrostructureModifier, smoothingLevel);
DEFINE_PROPERTY_FIELD(SimplifyMicrostructureModifier, kPB);
DEFINE_PROPERTY_FIELD(SimplifyMicrostructureModifier, lambda);
SET_PROPERTY_FIELD_LABEL(SimplifyMicrostructureModifier, smoothingLevel, "Smoothing level");
SET_PROPERTY_FIELD_LABEL(SimplifyMicrostructureModifier, kPB, "Smoothing param kPB");
SET_PROPERTY_FIELD_LABEL(SimplifyMicrostructureModifier, lambda, "Smoothing param lambda");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(SimplifyMicrostructureModifier, smoothingLevel, IntegerParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(SimplifyMicrostructureModifier, kPB, FloatParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(SimplifyMicrostructureModifier, lambda, FloatParameterUnit, 0);

/******************************************************************************
* Constructor.
******************************************************************************/
SimplifyMicrostructureModifier::SimplifyMicrostructureModifier(DataSet* dataset) : AsynchronousModifier(dataset),
    _smoothingLevel(8),
    _kPB(0.1),
    _lambda(0.7)
{
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool SimplifyMicrostructureModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	return input.containsObject<Microstructure>();
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the
* modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> SimplifyMicrostructureModifier::createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Get modifier input.
	const Microstructure* microstructure = input.getObject<Microstructure>();
	if(!microstructure)
		throwException(tr("No microstructure found in the modifier's input."));

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<SimplifyMicrostructureEngine>(microstructure, smoothingLevel(), kPB(), lambda());
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void SimplifyMicrostructureModifier::SimplifyMicrostructureEngine::perform()
{
	task()->setProgressText(tr("Simplifying microstructure"));

    // Implementation of the mesh smoothing algorithm:
	// Gabriel Taubin
	// A Signal Processing Approach To Fair Surface Design
	// In SIGGRAPH 95 Conference Proceedings, pages 351-358 (1995)

	FloatType mu = FloatType(1) / (_kPB - FloatType(1)/_lambda);
	task()->setProgressMaximum(_smoothingLevel);

	for(int iteration = 0; iteration < _smoothingLevel; iteration++) {
		if(!task()->setProgressValue(iteration)) return;
		smoothMeshIteration(_lambda);
		smoothMeshIteration(mu);
	}
}

/******************************************************************************
* Performs one iteration of the smoothing algorithm.
******************************************************************************/
void SimplifyMicrostructureModifier::SimplifyMicrostructureEngine::smoothMeshIteration(FloatType prefactor)
{
#if 0
	// Compute displacement for each vertex.
	std::vector<Vector3> displacements(microstructure()->vertexCount(), Vector3::Zero());
	std::vector<int> edgeCount(microstructure()->vertexCount(), 0);

    for(Microstructure::Face* face : microstructure()->faces()) {
        if(face->isSlipSurfaceFace() && face->isEvenFace()) {
            Microstructure::Edge* edge = face->edges();
            do {
                int mc = edge->countManifolds();
                if(mc <= 2) {
                    displacements[edge->vertex1()->index()] += edgeVector(edge);
                    edgeCount[edge->vertex1()->index()]++;
                }
                if(mc == 1) {
                    displacements[edge->vertex2()->index()] -= edgeVector(edge);
                    edgeCount[edge->vertex2()->index()]++;
                }
				edge = edge->nextFaceEdge();
			}
			while(edge != face->edges());
		}
    }

	// Apply computed displacements.
	auto d = displacements.cbegin();
    auto c = edgeCount.cbegin();
	for(Microstructure::Vertex* vertex : microstructure()->vertices()) {
        if(*c >= 2)
    		vertex->pos() += (*d) * (prefactor / (*c));
        ++d;
        ++c;
    }
#endif
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
void SimplifyMicrostructureModifier::SimplifyMicrostructureEngine::emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	 // Output simplified microstructure to the pipeline state, overwriting the input microstructure.
	if(const Microstructure* microstructureObj = state.getObject<Microstructure>()) {
        microstructure().transferTo(state.makeMutable(microstructureObj));
    }
}

}	// End of namespace
}	// End of namespace
