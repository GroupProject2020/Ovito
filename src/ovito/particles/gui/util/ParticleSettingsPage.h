///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2015) Alexander Stukowski
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


#include <ovito/particles/gui/ParticlesGui.h>
#include <ovito/gui/dialogs/ApplicationSettingsDialog.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

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

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
