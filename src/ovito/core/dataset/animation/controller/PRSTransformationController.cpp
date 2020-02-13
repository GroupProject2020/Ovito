////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 Alexander Stukowski
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

#include <ovito/core/Core.h>
#include <ovito/core/dataset/animation/controller/PRSTransformationController.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include <ovito/core/dataset/animation/TimeInterval.h>
#include <ovito/core/utilities/linalg/AffineDecomposition.h>

namespace Ovito {

IMPLEMENT_OVITO_CLASS(PRSTransformationController);
DEFINE_REFERENCE_FIELD(PRSTransformationController, positionController);
DEFINE_REFERENCE_FIELD(PRSTransformationController, rotationController);
DEFINE_REFERENCE_FIELD(PRSTransformationController, scalingController);
SET_PROPERTY_FIELD_LABEL(PRSTransformationController, positionController, "Position");
SET_PROPERTY_FIELD_LABEL(PRSTransformationController, rotationController, "Rotation");
SET_PROPERTY_FIELD_LABEL(PRSTransformationController, scalingController, "Scaling");
SET_PROPERTY_FIELD_UNITS(PRSTransformationController, positionController, WorldParameterUnit);
SET_PROPERTY_FIELD_UNITS(PRSTransformationController, rotationController, AngleParameterUnit);
SET_PROPERTY_FIELD_UNITS(PRSTransformationController, scalingController, PercentParameterUnit);

/******************************************************************************
* Default constructor.
******************************************************************************/
PRSTransformationController::PRSTransformationController(DataSet* dataset) : Controller(dataset)
{
	setPositionController(ControllerManager::createPositionController(dataset));
	setRotationController(ControllerManager::createRotationController(dataset));
	setScalingController(ControllerManager::createScalingController(dataset));
}

/******************************************************************************
* Let the controller apply its value at a certain time to the input value.
******************************************************************************/
void PRSTransformationController::applyTransformation(TimePoint time, AffineTransformation& result, TimeInterval& validityInterval)
{
	positionController()->applyTranslation(time, result, validityInterval);
	rotationController()->applyRotation(time, result, validityInterval);
	scalingController()->applyScaling(time, result, validityInterval);
}

/******************************************************************************
* Sets the controller's value at the specified time.
******************************************************************************/
void PRSTransformationController::setTransformationValue(TimePoint time, const AffineTransformation& newValue, bool isAbsolute)
{
	AffineDecomposition decomp(newValue);
	positionController()->setPositionValue(time, decomp.translation, isAbsolute);
	rotationController()->setRotationValue(time, Rotation(decomp.rotation), isAbsolute);
	scalingController()->setScalingValue(time, decomp.scaling, isAbsolute);
}

/******************************************************************************
* Adjusts the controller's value after a scene node has gotten a new parent node.
******************************************************************************/
void PRSTransformationController::changeParent(TimePoint time, const AffineTransformation& oldParentTM, const AffineTransformation& newParentTM, SceneNode* contextNode)
{
	positionController()->changeParent(time, oldParentTM, newParentTM, contextNode);
	rotationController()->changeParent(time, oldParentTM, newParentTM, contextNode);
	scalingController()->changeParent(time, oldParentTM, newParentTM, contextNode);
}

/******************************************************************************
* Computes the largest time interval containing the given time during which the
* controller's value is constant.
******************************************************************************/
TimeInterval PRSTransformationController::validityInterval(TimePoint time)
{
	TimeInterval iv = TimeInterval::infinite();
	iv.intersect(positionController()->validityInterval(time));
	iv.intersect(rotationController()->validityInterval(time));
	iv.intersect(scalingController()->validityInterval(time));
	return iv;
}

}	// End of namespace
