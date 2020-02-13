////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Alexander Stukowski
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


#include <ovito/particles/gui/ParticlesGui.h>
#include <ovito/gui/desktop/dialogs/ApplicationSettingsDialog.h>

namespace Ovito { namespace Particles {

/**
 * Page of the application settings dialog, which hosts particle-related options.
 */
class ParticleSettingsPage : public ApplicationSettingsDialogPage
{
	Q_OBJECT
	OVITO_CLASS(ParticleSettingsPage)

public:

	/// Default constructor.
	Q_INVOKABLE ParticleSettingsPage() = default;

	/// \brief Creates the widget.
	virtual void insertSettingsDialogPage(ApplicationSettingsDialog* settingsDialog, QTabWidget* tabWidget) override;

	/// \brief Lets the settings page to save all values entered by the user.
	/// \param settingsDialog The settings dialog box.
	virtual bool saveValues(ApplicationSettingsDialog* settingsDialog, QTabWidget* tabWidget) override;

	/// \brief Returns an integer value that is used to sort the dialog pages in ascending order.
	virtual int pageSortingKey() const override { return 4; }

public Q_SLOTS:

	/// Restores the built-in default particle colors and sizes.
	void restoreBuiltinParticlePresets();

private:

	QTreeWidget* _predefTypesTable;
	QTreeWidgetItem* _particleTypesItem;
	QTreeWidgetItem* _structureTypesItem;
};

}	// End of namespace
}	// End of namespace
