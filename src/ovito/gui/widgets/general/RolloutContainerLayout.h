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


#include <ovito/gui/GUI.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Widgets)

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

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


