////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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

namespace Ovito {

/******************************************************************************
* A Qt layout implementation used by the RolloutContainer widget.
******************************************************************************/
class OVITO_GUI_EXPORT RolloutContainerLayout : public QLayout
{
	Q_OBJECT

public:

	RolloutContainerLayout(QWidget* parent) : QLayout(parent) {}
	virtual ~RolloutContainerLayout();

	void addItem(QLayoutItem* item) override;
	int count() const override { return list.size(); }

	void insertWidget(int index, QWidget* widgets);

	void setGeometry(const QRect& r) override;

	QSize sizeHint() const override;
    QSize minimumSize() const override;

	QLayoutItem* itemAt(int index) const override {
		if(index < 0 || index >= list.size()) return nullptr;
		return list[index];
	}
	QLayoutItem* takeAt(int index) override {
		if(index < 0 || index >= list.size()) return nullptr;
		return list.takeAt(index);
	}

private:

	QList<QLayoutItem*> list;
};

}	// End of namespace


