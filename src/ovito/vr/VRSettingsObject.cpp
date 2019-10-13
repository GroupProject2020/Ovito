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

#include <ovito/core/Core.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/scene/RootSceneNode.h>
#include <ovito/core/viewport/ViewportSettings.h>
#include "VRSettingsObject.h"

namespace VRPlugin {

IMPLEMENT_OVITO_CLASS(VRSettingsObject);
DEFINE_PROPERTY_FIELD(VRSettingsObject, supersamplingEnabled);
DEFINE_PROPERTY_FIELD(VRSettingsObject, scaleFactor);
DEFINE_PROPERTY_FIELD(VRSettingsObject, showFloor);
DEFINE_PROPERTY_FIELD(VRSettingsObject, flyingMode);
DEFINE_PROPERTY_FIELD(VRSettingsObject, viewerTM);
DEFINE_PROPERTY_FIELD(VRSettingsObject, translation);
DEFINE_PROPERTY_FIELD(VRSettingsObject, rotationZ);
DEFINE_PROPERTY_FIELD(VRSettingsObject, modelCenter);
DEFINE_PROPERTY_FIELD(VRSettingsObject, movementSpeed);
SET_PROPERTY_FIELD_LABEL(VRSettingsObject, supersamplingEnabled, "Supersampling");
SET_PROPERTY_FIELD_LABEL(VRSettingsObject, scaleFactor, "Scale factor");
SET_PROPERTY_FIELD_LABEL(VRSettingsObject, translation, "Position");
SET_PROPERTY_FIELD_LABEL(VRSettingsObject, rotationZ, "Rotation angle");
SET_PROPERTY_FIELD_LABEL(VRSettingsObject, showFloor, "Show floor rectangle");
SET_PROPERTY_FIELD_LABEL(VRSettingsObject, flyingMode, "Fly mode");
SET_PROPERTY_FIELD_LABEL(VRSettingsObject, viewerTM, "Viewer transformation");
SET_PROPERTY_FIELD_LABEL(VRSettingsObject, modelCenter, "Center of rotation");
SET_PROPERTY_FIELD_LABEL(VRSettingsObject, movementSpeed, "Speed");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(VRSettingsObject, scaleFactor, PercentParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS(VRSettingsObject, rotationZ, AngleParameterUnit);
SET_PROPERTY_FIELD_UNITS(VRSettingsObject, modelCenter, WorldParameterUnit);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(VRSettingsObject, movementSpeed, FloatParameterUnit, 0);

/******************************************************************************
* Adjusts the transformation to bring the model into the center of the
* playing area.
******************************************************************************/
void VRSettingsObject::recenter()
{
    // Reset model position to center of scene bounding box.
    const Box3& bbox = dataset()->sceneRoot()->worldBoundingBox(dataset()->animationSettings()->time());
    if(!bbox.isEmpty()) {
        setModelCenter(bbox.center() - Point3::Origin());
    }
    setRotationZ(0);
    if(flyingMode() == false) {
        FloatType height = bbox.size(ViewportSettings::getSettings().upDirection()) * scaleFactor() / FloatType(1.9);
        setTranslation(Vector3(0, 0, height));
        setViewerTM(AffineTransformation::Identity());
    }
    else {
        FloatType offset = bbox.size().length() * scaleFactor() / 2;
        setTranslation(Vector3(0, 0, 0));
        setViewerTM(AffineTransformation::translation(
            (ViewportSettings::getSettings().coordinateSystemOrientation() *
            AffineTransformation::rotationX(FLOATTYPE_PI/2)).inverse() *
            Vector3(0, -offset, 0)));
    }
}

/******************************************************************************
* Computes the apparent model size in meters.
******************************************************************************/
Vector3 VRSettingsObject::apparentModelSize()
{
    const Box3& bbox = dataset()->sceneRoot()->worldBoundingBox(dataset()->animationSettings()->time());
    if(!bbox.isEmpty())
        return bbox.size() * scaleFactor();
    else
        return Vector3::Zero();
}

} // End of namespace
