////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2014 Alexander Stukowski
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


#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/dialogs/ApplicationSettingsDialog.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * Page of the application settings dialog, which hosts general program options.
 */
class OVITO_GUI_EXPORT GeneralSettingsPage : public ApplicationSettingsDialogPage
{
	Q_OBJECT
	OVITO_CLASS(GeneralSettingsPage)

public:

	/// Default constructor.
	Q_INVOKABLE GeneralSettingsPage() = default;

	/// \brief Creates the widget.
	virtual void insertSettingsDialogPage(ApplicationSettingsDialog* settingsDialog, QTabWidget* tabWidget) override;

	/// \brief Lets the settings page to save all values entered by the user.
	/// \param settingsDialog The settings dialog box.
	virtual bool saveValues(ApplicationSettingsDialog* settingsDialog, QTabWidget* tabWidget) override;

	/// \brief Returns an integer value that is used to sort the dialog pages in ascending order.
	virtual int pageSortingKey() const override { return 1; }

private:

	QCheckBox* _useQtFileDialog;
	QCheckBox* _enableMRUModifierList;
	QCheckBox* _overrideGLContextSharing;
	QComboBox* _contextSharingMode;
	QCheckBox* _overrideUseOfPointSprites;
	QComboBox* _pointSpriteMode;
	QCheckBox* _overrideUseOfGeometryShaders;
	QComboBox* _geometryShaderMode;
#if !defined(OVITO_BUILD_APPSTORE_VERSION)
	QCheckBox* _enableUpdateChecks;
	QCheckBox* _enableUsageStatistics;
#endif
	QLabel* _restartRequiredLabel;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


