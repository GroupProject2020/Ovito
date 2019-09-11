///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2019) Alexander Stukowski
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


#include <ovito/core/Core.h>

#include <QFont>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(View)

/**
 * \brief Stores general settings related to the viewports.
 */
class OVITO_CORE_EXPORT ViewportSettings : public QObject
{
public:

	/// Standard colors for drawing various things in the viewports.
	enum ViewportColor {
		COLOR_VIEWPORT_BKG,				///< Viewport background
		COLOR_GRID,						///< Minor construction grid lines
		COLOR_GRID_INTENS,				///< Major construction grid lines
		COLOR_GRID_AXIS,				///< Construction grid axis lines
		COLOR_VIEWPORT_CAPTION,			///< Viewport caption text
		COLOR_SELECTION,				///< Selected objects in wireframe mode
		COLOR_UNSELECTED,				///< Unselected objects in wireframe mode
		COLOR_ACTIVE_VIEWPORT_BORDER,	///< Border of the active viewport
		COLOR_ANIMATION_MODE,			///< Border color when animation mode is active
		COLOR_CAMERAS,					///< Camera icons

		NUMBER_OF_COLORS
	};
	Q_ENUMS(ViewportColor);

	/// Selects the "up" direction in the viewports.
	enum UpDirection {
		X_AXIS, ///< Makes the X axis the vertical axis
		Y_AXIS, ///< Makes the Y axis the vertical axis
		Z_AXIS, ///< Makes the Z axis the vertical axis (the default)
	};
	Q_ENUMS(UpDirection);

public:

	/// Default constructor. Initializes all settings to their default values.
	ViewportSettings();

	/// \brief Returns a color value for drawing something in the viewports.
	/// \param which The constant specifying what type of element to draw.
	/// \return The color that should be used for the given type of element.
	const Color& viewportColor(ViewportColor which) const;

	/// \brief Sets the color for drawing something in the viewports.
	/// \param which The constant specifying what type of element to draw.
	/// \return The color that should be used for the given type of element.
	void setViewportColor(ViewportColor which, const Color& color);

	/// \brief Sets all viewport colors to their default values.
	void restoreDefaultViewportColors();

	/// Returns the rotation axis to be used with orbit mode.
	Vector3 upVector() const;

	/// Returns a matrix that transforms the default coordinate system (with Z being the "up" direction)
	/// to the orientation given by the current "up" vector.
	Matrix3 coordinateSystemOrientation() const;

	/// Returns the selected rotation axis type.
	UpDirection upDirection() const { return _upDirection; }

	/// Sets the "up" direction.
	void setUpDirection(UpDirection t) {
		if(_upDirection != t) {
			_upDirection = t;
			Q_EMIT settingsChanged(this);
		}
	}

	/// Returns whether the camera rotation is restricted such that the selected axis always points upward.
	bool constrainCameraRotation() const { return _constrainCameraRotation; }

	/// Sets whether the camera rotation should be restricted such that the selected axis always points upward.
	void setConstrainCameraRotation(bool active) {
		if(_constrainCameraRotation != active) {
			_constrainCameraRotation = active;
			Q_EMIT settingsChanged(this);
		}
	}

	/// Returns the font to be used for rendering text in the viewports.
	const QFont& viewportFont() const { return _viewportFont; }

	/// Returns the type of viewport that should initially be in the maximized state.
	/// Or 0 if no viewport is initially maximized.
	int defaultMaximizedViewportType() const { return _defaultMaximizedViewportType; }

	/// Sets the type of viewport that will be initially in the maximized state.
	/// Or 0 if no viewport is initially maximized.
	void setDefaultMaximizedViewportType(int viewType) {
		if(_defaultMaximizedViewportType != viewType) {
			_defaultMaximizedViewportType = viewType;
			Q_EMIT settingsChanged(this);
		}
	}

	/// Loads the settings from the given settings store.
	void load(QSettings& store);

	/// Saves the settings to the given settings store.
	void save(QSettings& store) const;

	/// Saves the settings to the default application settings store.
	void save() const;

	/// Assignment.
	void assign(const ViewportSettings& other);

	/// Returns a (read-only) reference to the current global settings object.
	static ViewportSettings& getSettings();

	/// Replaces the current global settings with new values.
	static void setSettings(const ViewportSettings& settings);

Q_SIGNALS:

	/// This signal is emitted when the active viewport settings have changed.
	void settingsChanged(ViewportSettings* newSettings);

private:

	/// The colors for viewport drawing.
	std::array<Color, NUMBER_OF_COLORS> _viewportColors;

	/// The selected rotation axis type for orbit mode.
	UpDirection _upDirection = Z_AXIS;

	/// Restricts the camera rotation such that the selected axis always points upward.
	bool _constrainCameraRotation = true;

	/// The font used for rendering text in the viewports.
	QFont _viewportFont;

	/// The type of viewport that is initially in the maximized state.
	/// Or -1 if no viewport is initially maximized.
	int _defaultMaximizedViewportType = 0;

	Q_OBJECT
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::ViewportSettings::ViewportColor);
Q_DECLARE_METATYPE(Ovito::ViewportSettings::UpDirection);
Q_DECLARE_TYPEINFO(Ovito::ViewportSettings::ViewportColor, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(Ovito::ViewportSettings::UpDirection, Q_PRIMITIVE_TYPE);
