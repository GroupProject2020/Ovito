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

/**
 * \file LookAtController.h
 * \brief Contains the definition of the Ovito::LookAtController class.
 */

#pragma once


#include <ovito/core/Core.h>
#include <ovito/core/dataset/scene/SceneNode.h>
#include <ovito/core/dataset/animation/controller/Controller.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Anim)

/**
 * \brief Rotation controller that lets an object always "look" at another scene node.
 *
 * This RotationController computes a rotation matrix for a SceneNode such
 * that it always faces into the direction of the target SceneNode.
 */
class OVITO_CORE_EXPORT LookAtController : public Controller
{
	Q_OBJECT
	OVITO_CLASS(LookAtController)

public:

	/// \brief Constructor.
	Q_INVOKABLE LookAtController(DataSet* dataset);

	/// \brief Returns the value type of the controller.
	virtual ControllerType controllerType() const override { return ControllerTypeRotation; }

	/// Queries the controller for its value at a certain time.
	virtual void getRotationValue(TimePoint time, Rotation& result, TimeInterval& validityInterval) override;

	/// Sets the controller's value at the specified time.
	virtual void setRotationValue(TimePoint time, const Rotation& newValue, bool isAbsoluteValue) override;

	/// Lets the rotation controller apply its value to an existing transformation matrix.
	virtual void applyRotation(TimePoint time, AffineTransformation& result, TimeInterval& validityInterval) override;

	/// Computes the largest time interval containing the given time during which the
	/// controller's value is constant.
	virtual TimeInterval validityInterval(TimePoint time) override;

	/// This asks the controller to adjust its value after a scene node has got a new
	/// parent node.
	///		oldParentTM - The transformation of the old parent node
	///		newParentTM - The transformation of the new parent node
	///		contextNode - The node to which this controller is assigned to
	virtual void changeParent(TimePoint time, const AffineTransformation& oldParentTM, const AffineTransformation& newParentTM, SceneNode* contextNode) override {}

	/// \brief Returns whether the value of this controller is changing over time.
	virtual bool isAnimated() const override {
		return (rollController() && rollController()->isAnimated())
				|| (targetNode() && targetNode()->transformationController() && targetNode()->transformationController()->isAnimated());
	}

private:

	/// The sub-controller for rolling.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(Controller, rollController, setRollController);

	/// The target scene node to look at.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(SceneNode, targetNode, setTargetNode, PROPERTY_FIELD_NEVER_CLONE_TARGET | PROPERTY_FIELD_NO_SUB_ANIM);

	/// Stores the cached position of the source node.
	Vector3 _sourcePos;

	/// Stores the validity interval of the saved source position.
	TimeInterval _sourcePosValidity;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace


