////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 Alexander Stukowski
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


#include <ovito/gui/GUI.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * A combo-box widget that displays the current scene node selection
 * and allows to select scene nodes.
 */
class SceneNodeSelectionBox : public QComboBox
{
	Q_OBJECT

public:

	/// Constructs the widget.
	SceneNodeSelectionBox(DataSetContainer& datasetContainer, QWidget* parent = 0);

protected Q_SLOTS:

	/// This is called whenever the node selection has changed.
	void onSceneSelectionChanged();

	/// Is called when the user selected an item in the list box.
	void onItemActivated(int index);

	/// This is called whenever the number of nodes changes.
	void onNodeCountChanged();

private:

	/// The container of the dataset.
	DataSetContainer& _datasetContainer;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


