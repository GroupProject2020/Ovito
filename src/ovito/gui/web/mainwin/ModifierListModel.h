////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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


#include <ovito/gui/web/GUIWeb.h>
#include "PipelineListModel.h"

namespace Ovito {

/**
 * A Qt list model that list all available modifier types that are applicable to the current data pipeline.
 */
class ModifierListModel : public QAbstractListModel
{
	Q_OBJECT

public:

	/// Constructor.
	ModifierListModel(QObject* parent);

	/// Returns the number of rows in the model.
	virtual int rowCount(const QModelIndex& parent) const override;

	/// Returns the data associated with a list item.
	virtual QVariant data(const QModelIndex& index, int role) const override;

	/// Returns the flags for an item.
	virtual Qt::ItemFlags flags(const QModelIndex& index) const override;

public Q_SLOTS:

	/// Instantiates a modifier and inserts into the current data pipeline.
	void insertModifier(int index, PipelineListModel* pipelineModel);

private:

	/// The list of modifier class types.
	QVector<ModifierClassPtr> _modifierClasses;
};

}	// End of namespace
