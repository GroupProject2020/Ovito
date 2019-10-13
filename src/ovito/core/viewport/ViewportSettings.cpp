////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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
#include <ovito/core/viewport/ViewportSettings.h>
#include <ovito/core/viewport/Viewport.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(View)

/// The current settings record.
Q_GLOBAL_STATIC(ViewportSettings, _currentViewportSettings);

/******************************************************************************
* Assignment.
******************************************************************************/
void ViewportSettings::assign(const ViewportSettings& other)
{
	_viewportColors = other._viewportColors;
	_upDirection = other._upDirection;
	_constrainCameraRotation = other._constrainCameraRotation;
	_viewportFont = other._viewportFont;
	_defaultMaximizedViewportType = other._defaultMaximizedViewportType;

	Q_EMIT settingsChanged(this);
}

/******************************************************************************
* Returns a reference to the current global settings object.
******************************************************************************/
ViewportSettings& ViewportSettings::getSettings()
{
	/// Indicates whether the settings have already been loaded from the application's settings store.
	static bool settingsLoaded = false;

	if(!settingsLoaded) {
		QSettings settingsStore;
		settingsStore.beginGroup("core/viewport/");
		_currentViewportSettings->load(settingsStore);
		settingsStore.endGroup();
		settingsLoaded = true;
	}
	return *_currentViewportSettings;
}

/******************************************************************************
* Replaces the current global settings with new values.
******************************************************************************/
void ViewportSettings::setSettings(const ViewportSettings& settings)
{
	_currentViewportSettings->assign(settings);
	_currentViewportSettings->save();
}

/******************************************************************************
* Default constructor.
******************************************************************************/
ViewportSettings::ViewportSettings() :
	_viewportFont("Helvetica")
{
	restoreDefaultViewportColors();
}

/******************************************************************************
* Sets all viewport colors to their default values.
******************************************************************************/
void ViewportSettings::restoreDefaultViewportColors()
{
	_viewportColors[COLOR_VIEWPORT_BKG] = Color(0.0f, 0.0f, 0.0f);
	_viewportColors[COLOR_GRID] = Color(0.5f, 0.5f, 0.5f);
	_viewportColors[COLOR_GRID_INTENS] = Color(0.6f, 0.6f, 0.6f);
	_viewportColors[COLOR_GRID_AXIS] = Color(0.7f, 0.7f, 0.7f);
	_viewportColors[COLOR_VIEWPORT_CAPTION] = Color(1.0f, 1.0f, 1.0f);
	_viewportColors[COLOR_SELECTION] = Color(1.0f, 1.0f, 1.0f);
	_viewportColors[COLOR_UNSELECTED] = Color(0.6f, 0.6f, 1.0f);
	_viewportColors[COLOR_ACTIVE_VIEWPORT_BORDER] = Color(1.0f, 1.0f, 0.0f);
	_viewportColors[COLOR_ANIMATION_MODE] = Color(1.0f, 0.0f, 0.0f);
	_viewportColors[COLOR_CAMERAS] = Color(0.5f, 0.5f, 1.0f);
}

/******************************************************************************
* Returns color values for drawing in the viewports.
******************************************************************************/
const Color& ViewportSettings::viewportColor(ViewportColor which) const
{
	OVITO_ASSERT(which >= 0 && which < _viewportColors.size());
	return _viewportColors[which];
}

/******************************************************************************
* Sets color values for drawing in the viewports.
******************************************************************************/
void ViewportSettings::setViewportColor(ViewportColor which, const Color& color)
{
	OVITO_ASSERT(which >= 0 && which < _viewportColors.size());
	if(_viewportColors[which] != color) {
		_viewportColors[which] = color;
		Q_EMIT settingsChanged(this);
	}
}

/******************************************************************************
* Returns the rotation axis to be used with orbit mode.
******************************************************************************/
Vector3 ViewportSettings::upVector() const
{
	switch(_upDirection) {
	case X_AXIS: return { 1,0,0 };
	case Y_AXIS: return { 0,1,0 };
	case Z_AXIS: return { 0,0,1 };
	default: return { 0,0,1 };
	}
}

/******************************************************************************
* Returns a matrix that transforms the default coordinate system
* (with Z being the "up" direction) to the orientation given by the
* current "up" vector.
******************************************************************************/
Matrix3 ViewportSettings::coordinateSystemOrientation() const
{
	switch(_upDirection) {
	case X_AXIS: return Matrix3(Vector3(0,1,0), Vector3(0,0,1), Vector3(1,0,0));
	case Y_AXIS: return Matrix3(Vector3(-1,0,0), Vector3(0,0,1), Vector3(0,1,0));
	case Z_AXIS:
	default:
		return Matrix3::Identity();
	}
}

/******************************************************************************
* Loads the settings from the given settings store.
******************************************************************************/
void ViewportSettings::load(QSettings& store)
{
	_upDirection = (UpDirection)store.value("UpDirection", QVariant::fromValue((int)_upDirection)).toInt();
	_constrainCameraRotation = store.value("ConstrainCameraRotation", QVariant::fromValue(_constrainCameraRotation)).toBool();
	_defaultMaximizedViewportType = store.value("DefaultMaximizedViewportType", QVariant::fromValue(_defaultMaximizedViewportType)).toInt();
	store.beginGroup("Colors");
	QMetaEnum colorEnum;
	for(int i = 0; i < ViewportSettings::staticMetaObject.enumeratorCount(); i++) {
		if(qstrcmp(ViewportSettings::staticMetaObject.enumerator(i).name(), "ViewportColor") == 0) {
			colorEnum = ViewportSettings::staticMetaObject.enumerator(i);
			break;
		}
	}
	OVITO_ASSERT(colorEnum.isValid());
	for(const QString& key : store.childKeys()) {
		QColor c = store.value(key).value<QColor>();
		bool ok;
		int index = colorEnum.keyToValue(key.toUtf8().constData(), &ok);
		if(ok && index >= 0 && index < NUMBER_OF_COLORS && c.isValid()) {
			_viewportColors[index] = Color(c);
		}
	}
	store.endGroup();
}

/******************************************************************************
* Saves the settings to the default application settings store.
******************************************************************************/
void ViewportSettings::save() const
{
	QSettings settingsStore;
	settingsStore.beginGroup("core/viewport/");
	save(settingsStore);
	settingsStore.endGroup();
}

/******************************************************************************
* Saves the settings to the given settings store.
******************************************************************************/
void ViewportSettings::save(QSettings& store) const
{
	store.setValue("UpDirection", (int)_upDirection);
	store.setValue("ConstrainCameraRotation", _constrainCameraRotation);
	store.setValue("DefaultMaximizedViewportType", _defaultMaximizedViewportType);
	store.remove("Colors");
	store.beginGroup("Colors");
	QMetaEnum colorEnum;
	for(int i = 0; i < ViewportSettings::staticMetaObject.enumeratorCount(); i++) {
		if(qstrcmp(ViewportSettings::staticMetaObject.enumerator(i).name(), "ViewportColor") == 0) {
			colorEnum = ViewportSettings::staticMetaObject.enumerator(i);
			break;
		}
	}
	OVITO_ASSERT(colorEnum.isValid());
    for(size_t i = 0; i < _viewportColors.size(); i++) {
		store.setValue(colorEnum.key(i), QVariant::fromValue((QColor)_viewportColors[i]));
	}
	store.endGroup();
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
