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
#include <ovito/core/dataset/data/DataObject.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene) OVITO_BEGIN_INLINE_NAMESPACE(StdObj)

/**
 * \brief Abstract base class for camera objects.
 */
class OVITO_CORE_EXPORT AbstractCameraObject : public DataObject
{
	Q_OBJECT
	OVITO_CLASS(AbstractCameraObject)

protected:

	/// Constructor.
	using DataObject::DataObject;

public:

	/// \brief Returns a structure describing the camera's projection.
	/// \param[in] time The animation time for which the camera's projection parameters should be determined.
	/// \param[in,out] projParams The structure that is to be filled with the projection parameters.
	///     The following fields of the ViewProjectionParameters structure are already filled in when the method is called:
	///   - ViewProjectionParameters::aspectRatio (The aspect ratio (height/width) of the viewport)
	///   - ViewProjectionParameters::viewMatrix (The world to view space transformation)
	///   - ViewProjectionParameters::boundingBox (The bounding box of the scene in world space coordinates)
	virtual void projectionParameters(TimePoint time, ViewProjectionParameters& projParams) const = 0;

	/// \brief Returns whether this camera uses a perspective projection.
	virtual bool isPerspectiveCamera() const = 0;

	/// \brief Sets whether this camera uses a perspective projection.
	virtual void setPerspectiveCamera(bool perspective) = 0;

	/// \brief Returns the field of view of the camera.
	virtual FloatType fieldOfView(TimePoint time, TimeInterval& validityInterval) const = 0;

	/// \brief Changes the field of view of the camera.
	virtual void setFieldOfView(TimePoint time, FloatType newFOV) = 0;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
