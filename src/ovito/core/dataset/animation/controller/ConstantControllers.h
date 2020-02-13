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

#pragma once


#include <ovito/core/Core.h>
#include "Controller.h"

namespace Ovito {

/**
 * \brief An animation controller with a constant float value.
 */
class OVITO_CORE_EXPORT ConstFloatController : public Controller
{
	Q_OBJECT
	OVITO_CLASS(ConstFloatController)

public:

	/// Constructor.
	Q_INVOKABLE ConstFloatController(DataSet* dataset) : Controller(dataset), _value(0) {}

	/// \brief Returns the value type of the controller.
	virtual ControllerType controllerType() const override { return ControllerTypeFloat; }

	/// \brief Returns whether the value of this controller is changing over time.
	virtual bool isAnimated() const override { return false; }

	/// \brief Calculates the largest time interval containing the given time during which the controller's value does not change.
	virtual TimeInterval validityInterval(TimePoint time) override { return TimeInterval::infinite(); }

	/// \brief Gets the controller's value at a certain animation time.
	virtual FloatType getFloatValue(TimePoint time, TimeInterval& validityInterval) override { return value(); }

	/// \brief Sets the controller's value at the given animation time.
	virtual void setFloatValue(TimePoint time, FloatType newValue) override { setValue(newValue); }

private:

	/// Stores the constant value of the controller.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, value, setValue);
};

/**
 * \brief An animation controller with a constant int value.
 */
class OVITO_CORE_EXPORT ConstIntegerController : public Controller
{
	OVITO_CLASS(ConstIntegerController)
	Q_OBJECT

public:

	/// Constructor.
	Q_INVOKABLE ConstIntegerController(DataSet* dataset) : Controller(dataset), _value(0) {}

	/// \brief Returns the value type of the controller.
	virtual ControllerType controllerType() const override { return ControllerTypeInt; }

	/// \brief Returns whether the value of this controller is changing over time.
	virtual bool isAnimated() const override { return false; }

	/// \brief Calculates the largest time interval containing the given time during which the controller's value does not change.
	virtual TimeInterval validityInterval(TimePoint time) override { return TimeInterval::infinite(); }

	/// \brief Gets the controller's value at a certain animation time.
	virtual int getIntValue(TimePoint time, TimeInterval& validityInterval) override { return value(); }

	/// \brief Sets the controller's value at the given animation time.
	virtual void setIntValue(TimePoint time, int newValue) override { setValue(newValue); }

private:

	/// Stores the constant value of the controller.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, value, setValue);
};

/**
 * \brief An animation controller with a constant Vector3 value.
 */
class OVITO_CORE_EXPORT ConstVectorController : public Controller
{
	OVITO_CLASS(ConstVectorController)
	Q_OBJECT

public:

	/// Constructor.
	Q_INVOKABLE ConstVectorController(DataSet* dataset) : Controller(dataset), _value(Vector3::Zero()) {}

	/// \brief Returns the value type of the controller.
	virtual ControllerType controllerType() const override { return ControllerTypeVector3; }

	/// \brief Returns whether the value of this controller is changing over time.
	virtual bool isAnimated() const override { return false; }

	/// \brief Calculates the largest time interval containing the given time during which the controller's value does not change.
	virtual TimeInterval validityInterval(TimePoint time) override { return TimeInterval::infinite(); }

	/// \brief Gets the controller's value at a certain animation time.
	virtual void getVector3Value(TimePoint time, Vector3& result, TimeInterval& validityInterval) override { result = value(); }

	/// \brief Sets the controller's value at the given animation time.
	virtual void setVector3Value(TimePoint time, const Vector3& newValue) override { setValue(newValue); }

private:

	/// Stores the constant value of the controller.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Vector3, value, setValue);
};

/**
 * \brief An animation controller with a constant position value.
 */
class OVITO_CORE_EXPORT ConstPositionController : public Controller
{
	OVITO_CLASS(ConstPositionController)
	Q_OBJECT

public:

	/// Constructor.
	Q_INVOKABLE ConstPositionController(DataSet* dataset) : Controller(dataset), _value(Vector3::Zero()) {}

	/// \brief Returns the value type of the controller.
	virtual ControllerType controllerType() const override { return ControllerTypePosition; }

	/// \brief Returns whether the value of this controller is changing over time.
	virtual bool isAnimated() const override { return false; }

	/// \brief Calculates the largest time interval containing the given time during which the controller's value does not change.
	virtual TimeInterval validityInterval(TimePoint time) override { return TimeInterval::infinite(); }

	/// \brief Gets the controller's value at a certain animation time.
	virtual void getPositionValue(TimePoint time, Vector3& result, TimeInterval& validityInterval) override { result = value(); }

	/// \brief Sets a position controller's value at the given animation time.
	virtual void setPositionValue(TimePoint time, const Vector3& newValue, bool isAbsolute) override {
		setValue(isAbsolute ? newValue : (newValue + value()));
	}

private:

	/// Stores the constant value of the controller.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Vector3, value, setValue);
};

/**
 * \brief An animation controller with a constant rotation value.
 */
class OVITO_CORE_EXPORT ConstRotationController : public Controller
{
	OVITO_CLASS(ConstRotationController)
	Q_OBJECT

public:

	/// Constructor.
	Q_INVOKABLE ConstRotationController(DataSet* dataset) : Controller(dataset), _value(Rotation::Identity()) {}

	/// \brief Returns the value type of the controller.
	virtual ControllerType controllerType() const override { return ControllerTypeRotation; }

	/// \brief Returns whether the value of this controller is changing over time.
	virtual bool isAnimated() const override { return false; }

	/// \brief Calculates the largest time interval containing the given time during which the controller's value does not change.
	virtual TimeInterval validityInterval(TimePoint time) override { return TimeInterval::infinite(); }

	/// \brief Gets the controller's value at a certain animation time.
	virtual void getRotationValue(TimePoint time, Rotation& result, TimeInterval& validityInterval) override { result = value(); }

	/// \brief Sets a rotation controller's value at the given animation time.
	virtual void setRotationValue(TimePoint time, const Rotation& newValue, bool isAbsolute) override {
		setValue(isAbsolute ? newValue : (newValue * value()));
	}

private:

	/// Stores the constant value of the controller.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Rotation, value, setValue);
};

/**
 * \brief An animation controller with a constant scaling value.
 */
class OVITO_CORE_EXPORT ConstScalingController : public Controller
{
	OVITO_CLASS(ConstScalingController)
	Q_OBJECT

public:

	/// Constructor.
	Q_INVOKABLE ConstScalingController(DataSet* dataset) : Controller(dataset), _value(Scaling::Identity()) {}

	/// \brief Returns the value type of the controller.
	virtual ControllerType controllerType() const override { return ControllerTypeScaling; }

	/// \brief Returns whether the value of this controller is changing over time.
	virtual bool isAnimated() const override { return false; }

	/// \brief Calculates the largest time interval containing the given time during which the controller's value does not change.
	virtual TimeInterval validityInterval(TimePoint time) override { return TimeInterval::infinite(); }

	/// \brief Gets the controller's value at a certain animation time.
	virtual void getScalingValue(TimePoint time, Scaling& result, TimeInterval& validityInterval) override { result = value(); }

	/// \brief Sets a scaling controller's value at the given animation time.
	virtual void setScalingValue(TimePoint time, const Scaling& newValue, bool isAbsolute) override {
		setValue(isAbsolute ? newValue : (newValue * value()));
	}

private:

	/// Stores the constant value of the controller.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Scaling, value, setValue);
};

}	// End of namespace
