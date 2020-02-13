////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2016 Alexander Stukowski
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
#include <ovito/particles/modifier/modify/CreateBondsModifier.h>
#include <ovito/gui/desktop/properties/ModifierPropertiesEditor.h>

namespace Ovito { namespace Particles {

/**
 * \brief A properties editor for the CreateBondsModifier class.
 */
class CreateBondsModifierEditor : public ModifierPropertiesEditor
{
	Q_OBJECT
	OVITO_CLASS(CreateBondsModifierEditor)

public:

	/// Default constructor.
	Q_INVOKABLE CreateBondsModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

protected Q_SLOTS:

	/// Updates the contents of the pair-wise cutoff table.
	void updatePairCutoffList();

	/// Updates the cutoff values in the pair-wise cutoff table.
	void updatePairCutoffListValues();

private:

	class PairCutoffTableModel : public QAbstractTableModel {
	public:
		typedef std::vector<std::pair<OORef<ElementType>,OORef<ElementType>>> ContentType;

		using QAbstractTableModel::QAbstractTableModel;
		virtual int	rowCount(const QModelIndex& parent) const override { return _data.size(); }
		virtual int	columnCount(const QModelIndex& parent) const override { return 3; }
		virtual QVariant data(const QModelIndex& index, int role) const override;
		virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override {
			if(orientation != Qt::Horizontal || role != Qt::DisplayRole) return QVariant();
			switch(section) {
			case 0: return CreateBondsModifierEditor::tr("1st type");
			case 1: return CreateBondsModifierEditor::tr("2nd type");
			case 2: return CreateBondsModifierEditor::tr("Cutoff");
			default: return QVariant();
			}
		}
		virtual Qt::ItemFlags flags(const QModelIndex& index) const override {
			if(index.column() != 2)
				return Qt::ItemFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			else
				return Qt::ItemFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
		}
		virtual bool setData(const QModelIndex& index, const QVariant& value, int role) override;
		void setContent(CreateBondsModifier* modifier, ContentType&& data) {
			beginResetModel();
			_modifier = modifier;
			_data = std::move(data);
			endResetModel();
		}
		void updateContent() { Q_EMIT dataChanged(index(0,2), index(_data.size()-1,2)); }
	private:
		ContentType _data;
		OORef<CreateBondsModifier> _modifier;
	};

	QTableView* _pairCutoffTable;
	PairCutoffTableModel* _pairCutoffTableModel;
};

}	// End of namespace
}	// End of namespace
