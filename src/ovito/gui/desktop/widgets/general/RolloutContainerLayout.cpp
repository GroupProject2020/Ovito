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

#include <ovito/gui/desktop/GUI.h>
#include "RolloutContainerLayout.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Widgets)

RolloutContainerLayout::~RolloutContainerLayout()
{
	while(!list.isEmpty())
		delete list.takeFirst();
}

void RolloutContainerLayout::addItem(QLayoutItem* item)
{
	list.push_back(item);
}

void RolloutContainerLayout::insertWidget(int index, QWidget* widget)
{
	addChildWidget(widget);
	list.insert(index, new QWidgetItem(widget));
}

void RolloutContainerLayout::setGeometry(const QRect& r)
{
	int y = 0;
	for(QLayoutItem* item : list) {
		QSize itemSize = item->sizeHint();
		item->setGeometry(QRect(r.left(), r.top() + y, r.width(), itemSize.height()));
		y += itemSize.height() + spacing();
	}
}

QSize RolloutContainerLayout::sizeHint() const
{
	QSize size(0,0);
	for(QLayoutItem* item : list) {
		QSize itemSize = item->sizeHint();
		size.rwidth() = std::max(size.width(), itemSize.width());
		size.rheight() += itemSize.height();
	}
	size.rheight() += spacing() * std::max(0, list.size() - 1);
	return size;
}

QSize RolloutContainerLayout::minimumSize() const
{
	QSize size(0,0);
	for(QLayoutItem* item : list) {
		QSize itemSize = item->minimumSize();
		size.rwidth() = std::max(size.width(), itemSize.width());
		size.rheight() += itemSize.height();
	}
	size.rheight() += spacing() * std::max(0, list.size() - 1);
	return size;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
