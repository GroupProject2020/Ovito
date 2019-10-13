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
