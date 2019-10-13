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

#include <ovito/gui/GUI.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/rendering/RenderSettings.h>
#include "VRSceneRenderer.h"

namespace VRPlugin {

IMPLEMENT_OVITO_CLASS(VRSceneRenderer);

/******************************************************************************
* This method is called just before renderFrame() is called.
******************************************************************************/
void VRSceneRenderer::beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp)
{
	OpenGLSceneRenderer::beginFrame(time, params, vp);

	// Set viewport background color.
	setClearColor(ColorA(renderSettings()->backgroundColor()));
}

/******************************************************************************
* Returns the final size of the rendered image in pixels.
******************************************************************************/
QSize VRSceneRenderer::outputSize() const
{
	return OpenGLSceneRenderer::outputSize();
}

/******************************************************************************
* Returns the device pixel ratio of the output device we are rendering to.
******************************************************************************/
qreal VRSceneRenderer::devicePixelRatio() const
{
	return OpenGLSceneRenderer::devicePixelRatio();
}

}	// End of namespace
