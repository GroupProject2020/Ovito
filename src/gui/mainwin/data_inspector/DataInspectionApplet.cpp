///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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
#include "DataInspectionApplet.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui)

IMPLEMENT_OVITO_CLASS(DataInspectionApplet);

/******************************************************************************
* Handles key press events for this widget.
******************************************************************************/
void DataInspectionApplet::TableView::keyPressEvent(QKeyEvent* event)
{
    if(event->matches(QKeySequence::Copy)) {

        QItemSelectionModel* selection = selectionModel();
        QModelIndexList indices = selection->selectedIndexes();

        if(indices.isEmpty())
            return;

        // QModelIndex::operator < sorts first by row, then by column. This is what we need.
        qSort(indices);

        int lastRow = indices.first().row();
        int lastColumn = indices.first().column();

        QString selectedText;
        for(const QModelIndex& current : indices) {

            if(current.row() != lastRow) {
                selectedText += QLatin1Char('\n');
                lastColumn = indices.first().column();
                lastRow = current.row();
            }

            if(current.column() != lastColumn) {
                for(int i = 0; i < current.column() - lastColumn; ++i)
                    selectedText += QLatin1Char('\t');
                lastColumn = current.column();
            }

            selectedText += model()->data(current).toString();
        }

        selectedText += QLatin1Char('\n');

        QApplication::clipboard()->setText(selectedText);        
        event->accept();
    }
    else {
        QTableView::keyPressEvent(event);
    }
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
