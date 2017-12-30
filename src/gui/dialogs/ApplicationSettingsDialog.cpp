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

#include <gui/GUI.h>
#include <gui/mainwin/MainWindow.h>
#include <core/app/PluginManager.h>
#include "ApplicationSettingsDialog.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Dialogs)
	
IMPLEMENT_OVITO_CLASS(ApplicationSettingsDialogPage);

/******************************************************************************
* The constructor of the settings dialog class.
******************************************************************************/
ApplicationSettingsDialog::ApplicationSettingsDialog(QWidget* parent, OvitoClassPtr startPage) : QDialog(parent)
{
	setWindowTitle(tr("Application Settings"));
	
	QVBoxLayout* layout1 = new QVBoxLayout(this);
	
	// Create dialog contents.
	_tabWidget = new QTabWidget(this);
	layout1->addWidget(_tabWidget);

	// Instantiate all ApplicationSettingsDialogPage derived classes.
	for(OvitoClassPtr clazz : PluginManager::instance().listClasses(ApplicationSettingsDialogPage::OOClass())) {
		try {
			OORef<ApplicationSettingsDialogPage> page = static_object_cast<ApplicationSettingsDialogPage>(clazz->createInstance(nullptr));
			_pages.push_back(page);
		}
		catch(const Exception& ex) {
			ex.reportError();
		}	
	}
	
	// Sort pages.
	std::sort(_pages.begin(), _pages.end(), [](const auto& page1, const auto& page2) { return page1->pageSortingKey() < page2->pageSortingKey(); });

	// Show pages in dialog.
	int defaultPage = 0;
	for(const auto& page : _pages) {
		if(startPage && startPage->isMember(page)) defaultPage = _tabWidget->count();
		page->insertSettingsDialogPage(this, _tabWidget);
	}
	_tabWidget->setCurrentIndex(defaultPage);

	// Add a label that displays the location of the application settings store on the computer.
	QLabel* configLocationLabel = new QLabel();
	configLocationLabel->setText(tr("<p style=\"font-size: small; color: #686868;\">Program settings are stored in %1</p>").arg(QSettings().fileName()));
	configLocationLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	layout1->addWidget(configLocationLabel);

	// Ok and Cancel buttons
	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help, Qt::Horizontal, this);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &ApplicationSettingsDialog::onOk);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &ApplicationSettingsDialog::reject);
	connect(buttonBox, &QDialogButtonBox::helpRequested, this, &ApplicationSettingsDialog::onHelp);
	layout1->addWidget(buttonBox);
}

/******************************************************************************
* This is called when the user has pressed the OK button of the settings dialog.
* Validates and saves all settings made by the user and closes the dialog box.
******************************************************************************/
void ApplicationSettingsDialog::onOk()
{
	try {
		// Let all pages save their settings.
		for(const OORef<ApplicationSettingsDialogPage>& page : _pages) {
			if(!page->saveValues(this, _tabWidget)) {
				return;
			}
		}
		
		// Close dialog box.
		accept();
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
}

/******************************************************************************
* This is called when the user has pressed the help button of the settings dialog.
******************************************************************************/
void ApplicationSettingsDialog::onHelp()
{
	MainWindow::openHelpTopic(QStringLiteral("application_settings.html"));
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
