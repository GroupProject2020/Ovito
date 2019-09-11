///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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

#include <ovito/core/Core.h>
#include <ovito/core/rendering/SceneRenderer.h>
#include <ovito/core/app/PluginManager.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include "RenderSettings.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering)

IMPLEMENT_OVITO_CLASS(RenderSettings);
DEFINE_PROPERTY_FIELD(RenderSettings, imageInfo);
DEFINE_REFERENCE_FIELD(RenderSettings, renderer);
DEFINE_REFERENCE_FIELD(RenderSettings, backgroundColorController);
DEFINE_PROPERTY_FIELD(RenderSettings, outputImageWidth);
DEFINE_PROPERTY_FIELD(RenderSettings, outputImageHeight);
DEFINE_PROPERTY_FIELD(RenderSettings, generateAlphaChannel);
DEFINE_PROPERTY_FIELD(RenderSettings, saveToFile);
DEFINE_PROPERTY_FIELD(RenderSettings, skipExistingImages);
DEFINE_PROPERTY_FIELD(RenderSettings, renderingRangeType);
DEFINE_PROPERTY_FIELD(RenderSettings, customRangeStart);
DEFINE_PROPERTY_FIELD(RenderSettings, customRangeEnd);
DEFINE_PROPERTY_FIELD(RenderSettings, customFrame);
DEFINE_PROPERTY_FIELD(RenderSettings, everyNthFrame);
DEFINE_PROPERTY_FIELD(RenderSettings, fileNumberBase);
DEFINE_PROPERTY_FIELD(RenderSettings, framesPerSecond);
SET_PROPERTY_FIELD_LABEL(RenderSettings, imageInfo, "Image info");
SET_PROPERTY_FIELD_LABEL(RenderSettings, renderer, "Renderer");
SET_PROPERTY_FIELD_LABEL(RenderSettings, backgroundColorController, "Background color");
SET_PROPERTY_FIELD_LABEL(RenderSettings, outputImageWidth, "Width");
SET_PROPERTY_FIELD_LABEL(RenderSettings, outputImageHeight, "Height");
SET_PROPERTY_FIELD_LABEL(RenderSettings, generateAlphaChannel, "Transparent background");
SET_PROPERTY_FIELD_LABEL(RenderSettings, saveToFile, "Save to file");
SET_PROPERTY_FIELD_LABEL(RenderSettings, skipExistingImages, "Skip existing animation images");
SET_PROPERTY_FIELD_LABEL(RenderSettings, renderingRangeType, "Rendering range");
SET_PROPERTY_FIELD_LABEL(RenderSettings, customRangeStart, "Range start");
SET_PROPERTY_FIELD_LABEL(RenderSettings, customRangeEnd, "Range end");
SET_PROPERTY_FIELD_LABEL(RenderSettings, customFrame, "Frame");
SET_PROPERTY_FIELD_LABEL(RenderSettings, everyNthFrame, "Every Nth frame");
SET_PROPERTY_FIELD_LABEL(RenderSettings, fileNumberBase, "File number base");
SET_PROPERTY_FIELD_LABEL(RenderSettings, framesPerSecond, "Frames per second");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(RenderSettings, outputImageWidth, IntegerParameterUnit, 1);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(RenderSettings, outputImageHeight, IntegerParameterUnit, 1);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(RenderSettings, everyNthFrame, IntegerParameterUnit, 1);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(RenderSettings, framesPerSecond, IntegerParameterUnit, 0);

/******************************************************************************
* Constructor.
******************************************************************************/
RenderSettings::RenderSettings(DataSet* dataset) : RefTarget(dataset),
	_outputImageWidth(640),
	_outputImageHeight(480),
	_generateAlphaChannel(false),
	_saveToFile(false),
	_skipExistingImages(false),
	_renderingRangeType(CURRENT_FRAME),
	_customRangeStart(0),
	_customRangeEnd(100),
	_customFrame(0),
	_everyNthFrame(1),
	_fileNumberBase(0),
	_framesPerSecond(0)
{
	// Setup default background color.
	setBackgroundColorController(ControllerManager::createColorController(dataset));
	setBackgroundColor(Color(1,1,1));

	// Create an instance of the default renderer class.
	OvitoClassPtr rendererClass = PluginManager::instance().findClass("OpenGLRenderer", "StandardSceneRenderer");
	if(rendererClass == nullptr) {
		QVector<OvitoClassPtr> classList = PluginManager::instance().listClasses(SceneRenderer::OOClass());
		if(!classList.empty()) rendererClass = classList.front();
	}
	if(rendererClass)
		setRenderer(static_object_cast<SceneRenderer>(rendererClass->createInstance(dataset)));
}

/******************************************************************************
* Sets the output filename of the rendered image.
******************************************************************************/
void RenderSettings::setImageFilename(const QString& filename)
{
	if(filename == imageFilename()) return;
	ImageInfo newInfo = imageInfo();
	newInfo.setFilename(filename);
	setImageInfo(newInfo);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
