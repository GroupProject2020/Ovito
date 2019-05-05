///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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

#include <plugins/particles/Particles.h>
#include <plugins/particles/objects/ParticlesObject.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include "ParticlesSliceModifierDelegate.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_OVITO_CLASS(ParticlesSliceModifierDelegate);

/******************************************************************************
* Performs the actual rejection of particles.
******************************************************************************/
PipelineStatus ParticlesSliceModifierDelegate::apply(Modifier* modifier, PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
{
	const ParticlesObject* inputParticles = state.expectObject<ParticlesObject>();
	QString statusMessage = tr("%n input particles", 0, inputParticles->elementCount());

	SliceModifier* mod = static_object_cast<SliceModifier>(modifier);
	boost::dynamic_bitset<> mask(inputParticles->elementCount());

	// Get the required input properties.
	const PropertyObject* posProperty = inputParticles->expectProperty(ParticlesObject::PositionProperty);
	const PropertyObject* selProperty = mod->applyToSelection() ? inputParticles->expectProperty(ParticlesObject::SelectionProperty) : nullptr;
	OVITO_ASSERT(posProperty->size() == mask.size());
	OVITO_ASSERT(!selProperty || selProperty->size() == mask.size());

	// Obtain modifier parameter values.
	Plane3 plane;
	FloatType sliceWidth;
	std::tie(plane, sliceWidth) = mod->slicingPlane(time, state.mutableStateValidity());
	sliceWidth /= 2;

	boost::dynamic_bitset<>::size_type i = 0;
	const Point3* p = posProperty->constDataPoint3();
	const Point3* p_end = p + posProperty->size();

	if(sliceWidth <= 0) {
		if(selProperty) {
			const int* s = selProperty->constDataInt();
			for(; p != p_end; ++p, ++s, ++i) {
				if(*s && plane.pointDistance(*p) > 0) {
					mask.set(i);
				}
			}
		}
		else {
			for(; p != p_end; ++p, ++i) {
				if(plane.pointDistance(*p) > 0) {
					mask.set(i);
				}
			}
		}
	}
	else {
		bool invert = mod->inverse();
		if(selProperty) {
			const int* s = selProperty->constDataInt();
			for(; p != p_end; ++p, ++s, ++i) {
				if(*s && invert == (plane.classifyPoint(*p, sliceWidth) == 0)) {
					mask.set(i);
				}
			}
		}
		else {
			for(; p != p_end; ++p, ++i) {
				if(invert == (plane.classifyPoint(*p, sliceWidth) == 0)) {
					mask.set(i);
				}
			}
		}
	}

	// Make sure we can safely modify the particles object.
	ParticlesObject* outputParticles = state.makeMutable(inputParticles);
	if(mod->createSelection() == false) {

		// Delete the selected particles.
		size_t numDeleted = outputParticles->deleteElements(mask);
		statusMessage += tr("\n%n particles deleted", 0, numDeleted);
		statusMessage += tr("\n%n particles remaining", 0, outputParticles->elementCount());
	}
	else {
		size_t numSelected = 0;
		PropertyObject* newSelProperty = outputParticles->createProperty(ParticlesObject::SelectionProperty);
		OVITO_ASSERT(mask.size() == newSelProperty->size());
		boost::dynamic_bitset<>::size_type i = 0;
		for(int& s : newSelProperty->intRange()) {
			if(mask.test(i++)) {
				s = 1;
				numSelected++;
			}
			else s = 0;
		}

		statusMessage += tr("\n%n particles selected", 0, numSelected);
		statusMessage += tr("\n%n particles unselected", 0, outputParticles->elementCount() - numSelected);
	}

	return PipelineStatus(PipelineStatus::Success, statusMessage);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
