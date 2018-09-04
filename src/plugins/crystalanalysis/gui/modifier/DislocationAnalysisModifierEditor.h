///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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


#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/crystalanalysis/modifier/dxa/DislocationAnalysisModifier.h>
#include <plugins/stdobj/series/DataSeriesObject.h>
#include <gui/properties/ModifierPropertiesEditor.h>
#include <gui/properties/RefTargetListParameterUI.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * List box that displays the dislocation types.
 */
class DislocationTypeListParameterUI : public RefTargetListParameterUI
{
	Q_OBJECT
	OVITO_CLASS(DislocationTypeListParameterUI)

public:

	/// Constructor.
	DislocationTypeListParameterUI(QObject* parent = nullptr);

	/// This method is called when a new editable object has been activated.
	virtual void resetUI() override {
		RefTargetListParameterUI::resetUI();
		// Clear initial selection by default.
		tableWidget()->selectionModel()->clear();
	}

	/// Obtains the current statistics from the pipeline. 
	void updateDislocationCounts(const PipelineFlowState& state, ModifierApplication* modApp);

protected:

	/// Returns a data item from the list data model.
	virtual QVariant getItemData(RefTarget* target, const QModelIndex& index, int role) override;

	/// Returns the number of columns for the table view.
	virtual int tableColumnCount() override { return 4; }

	/// Returns the header data under the given role for the given RefTarget.
	virtual QVariant getHorizontalHeaderData(int index, int role) override {
		if(role == Qt::DisplayRole) {
			if(index == 0)
				return QVariant();
			else if(index == 1)
				return QVariant::fromValue(tr("Dislocation type"));
			else if(index == 2)
				return QVariant::fromValue(tr("Segs"));
			else
				return QVariant::fromValue(tr("Length"));
		}
		else return RefTargetListParameterUI::getHorizontalHeaderData(index, role);
	}

	/// Do not open sub-editor for selected structure type.
	virtual void openSubEditor() override {}

protected Q_SLOTS:

	/// Is called when the user has double-clicked on one of the dislocation types in the list widget.
	void onDoubleClickDislocationType(const QModelIndex& index);

private:

	OORef<DataSeriesObject> _dislocationLengths;
	OORef<DataSeriesObject> _dislocationCounts;
};

/**
 * Properties editor for the DislocationAnalysisModifier class.
 */
class DislocationAnalysisModifierEditor : public ModifierPropertiesEditor
{
	Q_OBJECT
	OVITO_CLASS(DislocationAnalysisModifierEditor)

public:

	/// Default constructor.
	Q_INVOKABLE DislocationAnalysisModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

private:

	std::unique_ptr<DislocationTypeListParameterUI> _burgersFamilyListUI;
};

}	// End of namespace
}	// End of namespace
}	// End of namespace
