////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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
#include "DataInspectionApplet.h"

namespace Ovito {

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

}	// End of namespace
