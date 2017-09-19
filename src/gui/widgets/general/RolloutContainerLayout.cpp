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

#include <gui/GUI.h>
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
