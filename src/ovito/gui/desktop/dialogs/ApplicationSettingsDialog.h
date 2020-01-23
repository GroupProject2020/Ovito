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

#pragma once


#include <ovito/gui/desktop/GUI.h>
#include <ovito/core/oo/OvitoObject.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Dialogs)

class ApplicationSettingsDialog;		// defined below.

/**
 * \brief Abstract base class for tab providers for the application's settings dialog.
 */
class OVITO_GUI_EXPORT ApplicationSettingsDialogPage : public OvitoObject
{
	Q_OBJECT
	OVITO_CLASS(ApplicationSettingsDialogPage)

protected:

	/// Base class constructor.
	ApplicationSettingsDialogPage() = default;

public:

	/// \brief Creates the tab that is inserted into the settings dialog.
	/// \param settingsDialog The settings dialog box.
	/// \param tabWidget The QTabWidget into which the method should insert the settings page.
	virtual void insertSettingsDialogPage(ApplicationSettingsDialog* settingsDialog, QTabWidget* tabWidget) = 0;

	/// \brief Lets the settings page to save all values entered by the user.
	/// \param settingsDialog The settings dialog box.
	/// \return true if the settings are valid; false if settings need to be corrected by the user and the dialog should not be closed.
	virtual bool saveValues(ApplicationSettingsDialog* settingsDialog, QTabWidget* tabWidget) { return true; }

	/// \brief Returns an integer value that is used to sort the dialog pages in ascending order.
	virtual int pageSortingKey() const { return 1000; }
};

/**
 * \brief The dialog window that lets the user change the global application settings.
 *
 * Plugins can add additional pages to this dialog by deriving new classes from
 * the ApplicationSettingsDialogPage class.
 */
class OVITO_GUI_EXPORT ApplicationSettingsDialog : public QDialog
{
	Q_OBJECT

public:

	/// \brief Constructs the dialog window.
	/// \param parent The parent window of the settings dialog.
	/// \param startPage An optional pointer to the ApplicationSettingsDialogPage derived class whose
	///                  settings page should be activated initially.
	ApplicationSettingsDialog(QWidget* parent, OvitoClassPtr startPage = nullptr);

protected Q_SLOTS:

	/// This is called when the user has pressed the OK button of the settings dialog.
	/// Validates and saves all settings made by the user and closes the dialog box.
	void onOk();

	/// This is called when the user has pressed the help button of the settings dialog.
	void onHelp();

private:

	QVector<OORef<ApplicationSettingsDialogPage>> _pages;
	QTabWidget* _tabWidget;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


