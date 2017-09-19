///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2013) Alexander Stukowski
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

#include <core/Core.h>
#include <core/rendering/SceneRenderer.h>
#include <core/viewport/Viewport.h>
#include <core/app/PluginManager.h>
#include <core/utilities/units/UnitsManager.h>
#include "RenderSettings.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering)

IMPLEMENT_OVITO_CLASS(RenderSettings);
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
DEFINE_PROPERTY_FIELD(RenderSettings, everyNthFrame);
DEFINE_PROPERTY_FIELD(RenderSettings, fileNumberBase);
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
SET_PROPERTY_FIELD_LABEL(RenderSettings, everyNthFrame, "Every Nth frame");
SET_PROPERTY_FIELD_LABEL(RenderSettings, fileNumberBase, "File number base");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(RenderSettings, outputImageWidth, IntegerParameterUnit, 1);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(RenderSettings, outputImageHeight, IntegerParameterUnit, 1);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(RenderSettings, everyNthFrame, IntegerParameterUnit, 1);

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
	_everyNthFrame(1), 
	_fileNumberBase(0)
{
	// Setup default background color.
	setBackgroundColorController(ControllerManager::createColorController(dataset));
	setBackgroundColor(Color(1,1,1));

	// Create an instance of the default renderer class.
	OvitoClassPtr rendererClass = PluginManager::instance().findClass("OpenGLRenderer", "StandardSceneRenderer");
	if(rendererClass == nullptr) {
		QVector<OvitoClassPtr> classList = PluginManager::instance().listClasses(SceneRenderer::OOClass());
		if(classList.isEmpty() == false) rendererClass = classList.front();
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
	_imageInfo.setFilename(filename);
	notifyDependents(ReferenceEvent::TargetChanged);
}

/******************************************************************************
* Sets the output image info of the rendered image.
******************************************************************************/
void RenderSettings::setImageInfo(const ImageInfo& imageInfo)
{
	if(imageInfo == _imageInfo) return;
	_imageInfo = imageInfo;
	notifyDependents(ReferenceEvent::TargetChanged);
}

#define RENDER_SETTINGS_FILE_FORMAT_VERSION		1

/******************************************************************************
* Saves the class' contents to the given stream. 
******************************************************************************/
void RenderSettings::saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData)
{
	RefTarget::saveToStream(stream, excludeRecomputableData);

	stream.beginChunk(RENDER_SETTINGS_FILE_FORMAT_VERSION);
	stream << _imageInfo;
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream. 
******************************************************************************/
void RenderSettings::loadFromStream(ObjectLoadStream& stream)
{
	RefTarget::loadFromStream(stream);

	stream.expectChunk(RENDER_SETTINGS_FILE_FORMAT_VERSION);
	stream >> _imageInfo;
	stream.closeChunk();
}

/******************************************************************************
* Creates a copy of this object. 
******************************************************************************/
OORef<RefTarget> RenderSettings::clone(bool deepCopy, CloneHelper& cloneHelper)
{
	// Let the base class create an instance of this class.
	OORef<RenderSettings> clone = static_object_cast<RenderSettings>(RefTarget::clone(deepCopy, cloneHelper));
	
	/// Copy data values.
	clone->_imageInfo = this->_imageInfo;

	return clone;
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
