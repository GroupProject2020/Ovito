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
#include <ovito/gui/mainwin/MainWindow.h>
#include <ovito/core/app/PluginManager.h>
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
