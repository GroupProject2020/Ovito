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

#pragma once


#include <ovito/core/Core.h>
#include <ovito/core/oo/RefTarget.h>

namespace VRPlugin {

using namespace Ovito;

/**
 * \brief An object that stores the current VR settings.
 */
class VRSettingsObject : public RefTarget
{
	Q_OBJECT
	OVITO_CLASS(VRSettingsObject)

public:

	/// \brief Default constructor.
	Q_INVOKABLE VRSettingsObject(DataSet* dataset) : RefTarget(dataset),
		_supersamplingEnabled(true),
		_scaleFactor(1e-1),
		_showFloor(false),
		_flyingMode(false),
		_translation(Vector3::Zero()),
		_rotationZ(0),
		_modelCenter(Vector3::Zero()),
		_viewerTM(AffineTransformation::Identity()),
		_movementSpeed(4) {}

	/// Adjusts the transformation to bring the model into the center of the playing area.
	void recenter();

	/// Computes the apparent model size in meters.
	Vector3 apparentModelSize();

private:

	/// Enables supersampling.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, supersamplingEnabled, setSupersamplingEnabled);

	/// The scaling applied to the model.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, scaleFactor, setScaleFactor, PROPERTY_FIELD_MEMORIZE);

	/// The translation applied to the model.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Vector3, translation, setTranslation);

	/// The rotation angle around vertical axis applied to the model.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, rotationZ, setRotationZ);

	/// The center point of the model, around which it is being rotated.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Vector3, modelCenter, setModelCenter);

	/// Enables the display of the floor rectangle.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, showFloor, setShowFloor);

	/// Activates the flying mode.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, flyingMode, setFlyingMode, PROPERTY_FIELD_MEMORIZE);

	/// Current flying position.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(AffineTransformation, viewerTM, setViewerTM);

	/// The speed of motion when navigating.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, movementSpeed, setMovementSpeed);
};

}	// End of namespace
