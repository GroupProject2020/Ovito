////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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

#include <ovito/stdmod/StdMod.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include "ReplicateModifier.h"

namespace Ovito { namespace StdMod {

IMPLEMENT_OVITO_CLASS(ReplicateModifier);
DEFINE_PROPERTY_FIELD(ReplicateModifier, numImagesX);
DEFINE_PROPERTY_FIELD(ReplicateModifier, numImagesY);
DEFINE_PROPERTY_FIELD(ReplicateModifier, numImagesZ);
DEFINE_PROPERTY_FIELD(ReplicateModifier, adjustBoxSize);
DEFINE_PROPERTY_FIELD(ReplicateModifier, uniqueIdentifiers);
SET_PROPERTY_FIELD_LABEL(ReplicateModifier, numImagesX, "Number of images - X");
SET_PROPERTY_FIELD_LABEL(ReplicateModifier, numImagesY, "Number of images - Y");
SET_PROPERTY_FIELD_LABEL(ReplicateModifier, numImagesZ, "Number of images - Z");
SET_PROPERTY_FIELD_LABEL(ReplicateModifier, adjustBoxSize, "Adjust simulation box size");
SET_PROPERTY_FIELD_LABEL(ReplicateModifier, uniqueIdentifiers, "Assign unique IDs");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ReplicateModifier, numImagesX, IntegerParameterUnit, 1);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ReplicateModifier, numImagesY, IntegerParameterUnit, 1);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ReplicateModifier, numImagesZ, IntegerParameterUnit, 1);

IMPLEMENT_OVITO_CLASS(ReplicateModifierDelegate);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
ReplicateModifier::ReplicateModifier(DataSet* dataset) : MultiDelegatingModifier(dataset),
	_numImagesX(1),
	_numImagesY(1),
	_numImagesZ(1),
	_adjustBoxSize(true),
	_uniqueIdentifiers(true)
{
	// Generate the list of delegate objects.
	createModifierDelegates(ReplicateModifierDelegate::OOClass());
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool ReplicateModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	return MultiDelegatingModifier::OOMetaClass::isApplicableTo(input)
		&& input.containsObject<SimulationCellObject>();
}

/******************************************************************************
* Helper function that returns the range of replicated boxes.
******************************************************************************/
Box3I ReplicateModifier::replicaRange() const
{
	std::array<int,3> nPBC;
	nPBC[0] = std::max(numImagesX(),1);
	nPBC[1] = std::max(numImagesY(),1);
	nPBC[2] = std::max(numImagesZ(),1);
	Box3I replicaBox;
	replicaBox.minc[0] = -(nPBC[0]-1)/2;
	replicaBox.minc[1] = -(nPBC[1]-1)/2;
	replicaBox.minc[2] = -(nPBC[2]-1)/2;
	replicaBox.maxc[0] = nPBC[0]/2;
	replicaBox.maxc[1] = nPBC[1]/2;
	replicaBox.maxc[2] = nPBC[2]/2;
	return replicaBox;
}

/******************************************************************************
* Modifies the input data in an immediate, preliminary way.
******************************************************************************/
void ReplicateModifier::evaluatePreliminary(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	// Apply all enabled modifier delegates to the input data.
	MultiDelegatingModifier::evaluatePreliminary(time, modApp, state);

	// Resize the simulation cell if enabled.
	if(adjustBoxSize()) {
		SimulationCellObject* cellObj = state.expectMutableObject<SimulationCellObject>();
		AffineTransformation simCell = cellObj->cellMatrix();
		Box3I newImages = replicaRange();
		simCell.translation() += (FloatType)newImages.minc.x() * simCell.column(0);
		simCell.translation() += (FloatType)newImages.minc.y() * simCell.column(1);
		simCell.translation() += (FloatType)newImages.minc.z() * simCell.column(2);
		simCell.column(0) *= (newImages.sizeX() + 1);
		simCell.column(1) *= (newImages.sizeY() + 1);
		simCell.column(2) *= (newImages.sizeZ() + 1);
		cellObj->setCellMatrix(simCell);
	}
}

}	// End of namespace
}	// End of namespace
