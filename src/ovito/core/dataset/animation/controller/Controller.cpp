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
#include <ovito/core/dataset/animation/controller/Controller.h>
#include <ovito/core/dataset/animation/controller/ConstantControllers.h>
#include <ovito/core/dataset/animation/controller/LinearInterpolationControllers.h>
#include <ovito/core/dataset/animation/controller/SplineInterpolationControllers.h>
#include <ovito/core/dataset/animation/controller/TCBInterpolationControllers.h>
#include <ovito/core/dataset/animation/controller/PRSTransformationController.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/DataSet.h>

namespace Ovito {

IMPLEMENT_OVITO_CLASS(Controller);

/******************************************************************************
* Returns the float controller's value at the current animation time.
******************************************************************************/
FloatType Controller::currentFloatValue()
{
	TimeInterval iv;
	return getFloatValue(dataset()->animationSettings()->time(), iv);
}

/******************************************************************************
* Returns the integers controller's value at the current animation time.
******************************************************************************/
int Controller::currentIntValue()
{
	TimeInterval iv;
	return getIntValue(dataset()->animationSettings()->time(), iv);
}

/******************************************************************************
* Returns the Vector3 controller's value at the current animation time.
******************************************************************************/
Vector3 Controller::currentVector3Value()
{
	Vector3 v; TimeInterval iv;
	getVector3Value(dataset()->animationSettings()->time(), v, iv);
	return v;
}

/******************************************************************************
* Sets the controller's value at the current animation time.
******************************************************************************/
void Controller::setCurrentFloatValue(FloatType newValue)
{
	setFloatValue(dataset()->animationSettings()->time(), newValue);
}

/******************************************************************************
* Sets the controller's value at the current animation time.
******************************************************************************/
void Controller::setCurrentIntValue(int newValue)
{
	setIntValue(dataset()->animationSettings()->time(), newValue);
}

/******************************************************************************
* Sets the controller's value at the current animation time.
******************************************************************************/
void Controller::setCurrentVector3Value(const Vector3& newValue)
{
	setVector3Value(dataset()->animationSettings()->time(), newValue);
}

/******************************************************************************
* Creates a new float controller.
******************************************************************************/
OORef<Controller> ControllerManager::createFloatController(DataSet* dataset)
{
	return new LinearFloatController(dataset);
}

/******************************************************************************
* Creates a new integer controller.
******************************************************************************/
OORef<Controller> ControllerManager::createIntController(DataSet* dataset)
{
	return new LinearIntegerController(dataset);
}

/******************************************************************************
* Creates a new Vector3 controller.
******************************************************************************/
OORef<Controller> ControllerManager::createVector3Controller(DataSet* dataset)
{
	return new LinearVectorController(dataset);
}

/******************************************************************************
* Creates a new position controller.
******************************************************************************/
OORef<Controller> ControllerManager::createPositionController(DataSet* dataset)
{
	//return new TCBPositionController(dataset);
	return new SplinePositionController(dataset);
}

/******************************************************************************
* Creates a new rotation controller.
******************************************************************************/
OORef<Controller> ControllerManager::createRotationController(DataSet* dataset)
{
	return new LinearRotationController(dataset);
}

/******************************************************************************
* Creates a new scaling controller.
******************************************************************************/
OORef<Controller> ControllerManager::createScalingController(DataSet* dataset)
{
	return new LinearScalingController(dataset);
}

/******************************************************************************
* Creates a new transformation controller.
******************************************************************************/
OORef<Controller> ControllerManager::createTransformationController(DataSet* dataset)
{
	return new PRSTransformationController(dataset);
}

}	// End of namespace
