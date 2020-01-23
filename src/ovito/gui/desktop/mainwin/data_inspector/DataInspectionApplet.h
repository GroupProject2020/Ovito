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

#pragma once


#include <ovito/gui/desktop/GUI.h>
#include <ovito/core/oo/OvitoObject.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui)

/**
 * \brief Abstract base class for applets shown in the data inspector.
 */
class OVITO_GUI_EXPORT DataInspectionApplet : public OvitoObject
{
	Q_OBJECT
	OVITO_CLASS(DataInspectionApplet)

public:

	/// Returns the key value for this applet that is used for ordering the applet tabs.
	virtual int orderingKey() const { return std::numeric_limits<int>::max(); }

	/// Determines whether the given pipeline data contains data that can be displayed by this applet.
	virtual bool appliesTo(const DataCollection& data) = 0;

	/// Lets the applet create the UI widget that is to be placed into the data inspector panel.
	virtual QWidget* createWidget(MainWindow* mainWindow) = 0;

	/// Lets the applet update the contents displayed in the inspector.
	virtual void updateDisplay(const PipelineFlowState& state, PipelineSceneNode* sceneNode) = 0;

	/// This is called when the applet is no longer visible.
	virtual void deactivate(MainWindow* mainWindow) {}

	/// Selects a specific data object in this applet.
	virtual bool selectDataObject(PipelineObject* dataSource, const QString& objectIdentifierHint, const QVariant& modeHint) { return false; }

public:

	/// A specialized QTableView widget, which allows copying the selected contents of the
	/// table to the clipboard.
	class TableView : public QTableView
	{
	public:

		/// Constructor.
		TableView(QWidget* parent = nullptr) : QTableView(parent) {
			setWordWrap(false);
		}

	protected:

		/// Handles key press events for this widget.
		virtual void keyPressEvent(QKeyEvent* event) override;
	};
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
