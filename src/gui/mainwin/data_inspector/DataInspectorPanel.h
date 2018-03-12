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

#pragma once


#include <gui/GUI.h>
#include <gui/mainwin/MainWindow.h>
#include <core/oo/RefTargetListener.h>
#include <core/utilities/DeferredMethodInvocation.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * The data inspection panel.
 */
class OVITO_GUI_EXPORT DataInspectorPanel : public QWidget
{
	Q_OBJECT

public:

	/// \brief Constructor.
	DataInspectorPanel(MainWindow* mainWindow);

protected Q_SLOTS:

	/// This is called whenever the scene node selection has changed.
	void onSceneSelectionChanged();

	/// This is called whenever the selected scene node sends an event.
	void onSceneNodeNotificationEvent(const ReferenceEvent& event);

	/// Is emitted whenever the scene of the current dataset has been changed and is being made ready for rendering.
	void onScenePreparationBegin();

	/// Is called whenever the scene became ready for rendering.
	void onScenePreparationEnd();

	/// Updates the contents displayed in the data inpector.
	void updateInspector();

	/// Is called when the user clicked on the tab bar.
	void onTabBarClicked(int index);

	/// Is called when the user selects a new tab.
	void onCurrentTabChanged(int index);

	/// Is called whenever the user has switched to a different page of the inspector.
	void onCurrentPageChanged(int index);

protected:

	virtual void mouseReleaseEvent(QMouseEvent* event) override {
		if(event->button() == Qt::LeftButton) {
			onTabBarClicked(-1);
			event->accept();
		}
		QWidget::mouseReleaseEvent(event);
	}

	void resizeEvent(QResizeEvent* event) override;

private:

	/// Updates the list of visible tabs.
	void updateTabs(const PipelineFlowState& pipelineState);

	/// Returns the dataset container this panel is associated with.
	GuiDataSetContainer& datasetContainer() const { return _mainWindow->datasetContainer(); }

private:

	/// Pointer to the main window this inspector panel is part of. 
	MainWindow* _mainWindow;

	/// The list of all installed data inspection applets.
	std::vector<OORef<DataInspectionApplet>> _applets;

	/// This maps applet indices to active tab indices.
	std::vector<int> _appletsToTabs;

	/// This maps tab indices to applets.
	std::vector<int> _tabsToApplets;

	/// The tab display.
	QTabBar* _tabBar;

	/// The container for the applet widgets.
	QStackedWidget* _appletContainer;

	/// Listenes to messages from the currently selected object node.
	RefTargetListener<PipelineSceneNode> _selectedNodeListener;

	/// For deferred invocation of the plot repaint function.
	DeferredMethodInvocation<DataInspectorPanel, &DataInspectorPanel::updateInspector> _updateInvocation;

	/// Animation shown in the title bar to indicate process. 
	QMovie _waitingForSceneAnim;

	/// UI element indicating that we are waiting for computations to complete.
	QLabel* _waitingForSceneIndicator;

	/// The active page of the inspector.
	int _activeAppletIndex = -1;

	/// Indicates whether the inspector panel is currently open.
	bool _inspectorActive = false;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
