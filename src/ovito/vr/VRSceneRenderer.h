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

#pragma once


#include <ovito/gui/GUI.h>
#include <ovito/opengl/OpenGLSceneRenderer.h>

namespace VRPlugin {

using namespace Ovito;

class VRSceneRenderer : public OpenGLSceneRenderer
{
	Q_OBJECT
	OVITO_CLASS(VRSceneRenderer)

public:

	/// Standard constructor.
	VRSceneRenderer(DataSet* dataset) : OpenGLSceneRenderer(dataset) {}

	/// This method is called just before renderFrame() is called.
	virtual void beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp) override;

	/// Returns whether this renderer is rendering an interactive viewport.
	virtual bool isInteractive() const override { return false; }

	/// Indicates whether the scene renderer is allowed to block execution until long-running
	/// operations, e.g. data pipeline evaluation, complete.
	virtual bool waitForLongOperationsEnabled() const override { return false; }

	/// Returns the final size of the rendered image in pixels.
	virtual QSize outputSize() const override;

	/// Returns the device pixel ratio of the output device we are rendering to.
	qreal devicePixelRatio() const;
};

}	// End of namespace
