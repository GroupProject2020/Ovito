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
#include "ParameterUI.h"

namespace Ovito {

/**
 * \brief UI component that allows the user to select the delegate for a DelegatingModifier.
 */
class ModifierDelegateParameterUI : public ParameterUI
{
	Q_OBJECT
	OVITO_CLASS(ModifierDelegateParameterUI)
	Q_PROPERTY(QString delegateType READ delegateType WRITE setDelegateType);
	Q_PROPERTY(QStringList delegateList READ delegateList NOTIFY delegateListChanged);

public:

	/// Constructor.
	ModifierDelegateParameterUI() {
		connect(this, &ParameterUI::editObjectReplaced, this, &ModifierDelegateParameterUI::delegateListChanged);
	}

	/// Sets the class of delegates the user can choose from.
	void setDelegateType(const QString& typeName);

	/// Returns the class of delegates the user can choose from.
	QString delegateType() const { return _delegateType ? _delegateType->name() : QString(); }

	/// Obtains the current value of the parameter from the C++ object.
	virtual QVariant getCurrentValue() const override;

	/// Changes the current value of the C++ object parameter.
	virtual void setCurrentValue(const QVariant& val) override;

	/// Returns the list of available delegate types.
	QStringList delegateList();

Q_SIGNALS:

	/// This signal is emitted when the list of available delegate types changes.
	void delegateListChanged();

protected:

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

	/// The type of modifier delegates, which the user can choose from.
	OvitoClassPtr _delegateType = nullptr;

	/// The list of available delegates.
	QVector<std::pair<OvitoClassPtr, DataObjectReference>> _delegateList;
};

}	// End of namespace
