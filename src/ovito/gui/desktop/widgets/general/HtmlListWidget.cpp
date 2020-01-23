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

#include <ovito/gui/desktop/GUI.h>
#include "HtmlListWidget.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Widgets)

/******************************************************************************
* Constructs a list widget.
******************************************************************************/
HtmlListWidget::HtmlListWidget(QWidget* parent) : QListWidget(parent)
{
	class HtmlItemDelegate : public QStyledItemDelegate {
	protected:
		virtual void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
			QStyleOptionViewItem options = option;
			initStyleOption(&options, index);
			painter->save();
			QTextDocument doc;
			doc.setHtml(options.text);
			options.text.clear();
			options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter);
			// shift text right to make icon visible
			painter->translate(options.rect.left(), options.rect.top());
			QRect clip(0, 0, options.rect.width(), options.rect.height());
			doc.setTextWidth(clip.width());
			QAbstractTextDocumentLayout::PaintContext ctx;
			// set text color to red for selected item
			if(option.state & QStyle::State_Selected)
				ctx.palette.setColor(QPalette::Text, options.palette.color(QPalette::Active, QPalette::HighlightedText));
			ctx.clip = clip;
			doc.documentLayout()->draw(painter, ctx);
			painter->restore();
		}

		virtual QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override {
			QStyleOptionViewItem options = option;
			initStyleOption(&options, index);
			QTextDocument doc;
			doc.setHtml(options.text);
			doc.setTextWidth(options.rect.width());
			return QSize(doc.idealWidth(), doc.size().height());
		}
	};
	setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	setWordWrap(true);
	setItemDelegate(new HtmlItemDelegate());
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
