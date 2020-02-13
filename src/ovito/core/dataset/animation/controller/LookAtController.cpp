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
#include <ovito/core/utilities/units/UnitsManager.h>
#include <ovito/core/dataset/animation/controller/LookAtController.h>
#include <ovito/core/dataset/scene/SceneNode.h>

namespace Ovito {

IMPLEMENT_OVITO_CLASS(LookAtController);
DEFINE_REFERENCE_FIELD(LookAtController, rollController);
DEFINE_REFERENCE_FIELD(LookAtController, targetNode);
SET_PROPERTY_FIELD_LABEL(LookAtController, rollController, "Roll");
SET_PROPERTY_FIELD_LABEL(LookAtController, targetNode, "Target");
SET_PROPERTY_FIELD_UNITS(LookAtController, rollController, AngleParameterUnit);

/******************************************************************************
* Constructor.
******************************************************************************/
LookAtController::LookAtController(DataSet* dataset) : Controller(dataset)
{
	// Create sub-controller.
	setRollController(ControllerManager::createFloatController(dataset));
}

/******************************************************************************
* Queries the controller for its absolute value at a certain time.
******************************************************************************/
void LookAtController::getRotationValue(TimePoint time, Rotation& result, TimeInterval& validityInterval)
{
	// Get position of target node.
	Vector3 targetPos = Vector3::Zero();
	if(targetNode()) {
		const AffineTransformation& targetTM = targetNode()->getWorldTransform(time, validityInterval);
		targetPos = targetTM.translation();
	}

	if(!_sourcePosValidity.isEmpty())
		validityInterval.intersect(_sourcePosValidity);
	else
		validityInterval.intersect(TimeInterval(time));

	// Get rolling angle.
	FloatType rollAngle = 0.0;
	if(rollController())
		rollAngle = rollController()->getFloatValue(time, validityInterval);

	if(targetPos == _sourcePos) {
		result.setIdentity();
		return;
	}

	AffineTransformation tm = AffineTransformation::lookAt(Point3::Origin() + _sourcePos, Point3::Origin() + targetPos, Vector3(0,0,1));
	tm.translation() = Vector3::Zero();
	result = Rotation(tm).inverse();

	if(rollAngle != 0.0)
		result = result * Rotation(Vector3(0,0,1), rollAngle);

	// Reset source validity.
	_sourcePosValidity.setEmpty();
}

/******************************************************************************
* Sets the controller's value at the specified time.
******************************************************************************/
void LookAtController::setRotationValue(TimePoint time, const Rotation& newValue, bool isAbsoluteValue)
{
	// Cannot set value for this controller type.
}

/******************************************************************************
* Lets a rotation controller apply its value to an existing transformation matrix.
******************************************************************************/
void LookAtController::applyRotation(TimePoint time, AffineTransformation& result, TimeInterval& validityInterval)
{
	// Save source position for later use.
	_sourcePos = result.translation();
	_sourcePosValidity = validityInterval;

	Controller::applyRotation(time, result, validityInterval);
}

/******************************************************************************
* Computes the largest time interval containing the given time during which the
* controller's value is constant.
******************************************************************************/
TimeInterval LookAtController::validityInterval(TimePoint time)
{
	TimeInterval iv(TimeInterval::infinite());
	if(rollController())
		iv.intersect(rollController()->validityInterval(time));
	if(targetNode())
		targetNode()->getWorldTransform(time, iv);
	return iv;
}

}	// End of namespace
