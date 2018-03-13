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
#include <gui/mainwin/MainWindow.h>
#include <core/app/PluginManager.h>
#include "DataInspectorPanel.h"
#include "DataInspectionApplet.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* Constructor.
******************************************************************************/
DataInspectorPanel::DataInspectorPanel(MainWindow* mainWindow) : 
	_mainWindow(mainWindow),
	_waitingForSceneAnim(":/gui/mainwin/inspector/waiting.gif")
{
	// Create data inspection applets.
	for(OvitoClassPtr clazz : PluginManager::instance().listClasses(DataInspectionApplet::OOClass())) {
		OORef<DataInspectionApplet> applet = static_object_cast<DataInspectionApplet>(clazz->createInstance(nullptr));
		_applets.push_back(std::move(applet));
	}
	// Give applets a fixed ordering.
	std::sort(_applets.begin(), _applets.end(), [](DataInspectionApplet* a, DataInspectionApplet* b) { return a->orderingKey() < b->orderingKey(); });
	_appletsToTabs.resize(_applets.size(), -1);

	QGridLayout* layout = new QGridLayout(this);
	layout->setContentsMargins(0,0,0,0);
	layout->setSpacing(0);
	layout->setRowStretch(1, 1);
	layout->setColumnStretch(0, 1);
	layout->setColumnStretch(3, 1);

	_tabBar = new QTabBar();
	_tabBar->setShape(QTabBar::RoundedNorth);
	_tabBar->setDrawBase(false);
	_tabBar->setExpanding(false);
	_tabBar->setDocumentMode(false);
	layout->addWidget(_tabBar, 0, 1);

	_waitingForSceneIndicator = new QLabel();
	_waitingForSceneAnim.setCacheMode(QMovie::CacheAll);
	_waitingForSceneIndicator->setMovie(&_waitingForSceneAnim);
	_waitingForSceneIndicator->hide();
	layout->addWidget(_waitingForSceneIndicator, 0, 2);
	_waitingForSceneAnim.jumpToNextFrame();
	QSize indicatorSize = _waitingForSceneAnim.currentImage().size();
	layout->setRowMinimumHeight(0, indicatorSize.height());
	layout->setColumnMinimumWidth(2, indicatorSize.width());

	_appletContainer = new QStackedWidget();
	_appletContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
	QLabel* label = new QLabel(tr("There is no data to be displayed"));
	label->setAlignment(Qt::AlignCenter);
	_appletContainer->addWidget(label);
	for(DataInspectionApplet* applet : _applets)
		_appletContainer->insertWidget(_appletContainer->count() - 1, applet->createWidget(_mainWindow));
	layout->addWidget(_appletContainer, 1, 0, 1, 4);

	connect(_tabBar, &QTabBar::tabBarClicked, this, &DataInspectorPanel::onTabBarClicked);
	connect(_tabBar, &QTabBar::currentChanged, this, &DataInspectorPanel::onCurrentTabChanged);
	connect(_appletContainer, &QStackedWidget::currentChanged, this, &DataInspectorPanel::onCurrentPageChanged);
	connect(&datasetContainer(), &DataSetContainer::selectionChangeComplete, this, &DataInspectorPanel::onSceneSelectionChanged);	
	connect(&datasetContainer(), &GuiDataSetContainer::scenePreparationBegin, this, &DataInspectorPanel::onScenePreparationBegin);	
	connect(&datasetContainer(), &GuiDataSetContainer::scenePreparationEnd, this, &DataInspectorPanel::onScenePreparationEnd, Qt::QueuedConnection);
	connect(&datasetContainer(), &DataSetContainer::timeChanged, this, &DataInspectorPanel::onScenePreparationBegin);
	connect(&datasetContainer(), &DataSetContainer::timeChangeComplete, this, &DataInspectorPanel::onScenePreparationEnd, Qt::QueuedConnection);	
	connect(&_selectedNodeListener, &RefTargetListenerBase::notificationEvent, this, &DataInspectorPanel::onSceneNodeNotificationEvent);	
}

/******************************************************************************
* Is called when the user clicked on the tab bar.
******************************************************************************/
void DataInspectorPanel::onTabBarClicked(int index)
{
	if(index == -1 || index == _tabBar->currentIndex() || _appletContainer->height() == 0) {
		_tabBar->setCurrentIndex(index);
		if(_appletContainer->height() == 0)
			parentWidget()->setMaximumHeight(16777215);
		if(_appletContainer->height() != 0) {
			parentWidget()->setMaximumHeight(parentWidget()->minimumSizeHint().height());
			parentWidget()->parentWidget()->updateGeometry();
			QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
			parentWidget()->setMaximumHeight(16777215);
		}
		else {
			parentWidget()->setMinimumHeight(parentWidget()->minimumSizeHint().height() + _mainWindow->centralWidget()->height() / 3);
			parentWidget()->parentWidget()->updateGeometry();
			QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
			parentWidget()->setMinimumHeight(0);
		}
	}
}

/******************************************************************************
* This is called whenever the scene node selection has changed.
******************************************************************************/
void DataInspectorPanel::onSceneSelectionChanged()
{
	// Find the first selected PipelineSceneNode and make it the active node:
	PipelineSceneNode* selectedNode = nullptr;
	SelectionSet* selection = datasetContainer().currentSet() ? datasetContainer().currentSet()->selection() : nullptr;	
	if(selection) {
		for(SceneNode* node : selection->nodes()) {
			selectedNode = dynamic_object_cast<PipelineSceneNode>(node);
			if(selectedNode) break;
		}
	}
	if(selectedNode != _selectedNodeListener.target()) {
		_selectedNodeListener.setTarget(selectedNode);
		_updateInvocation(this);
	}
}

/******************************************************************************
* This is called whenever the selected scene node sends an event.
******************************************************************************/
void DataInspectorPanel::onSceneNodeNotificationEvent(const ReferenceEvent& event)
{
}

/******************************************************************************
* Is emitted whenever the scene of the current dataset has been changed and 
* is being made ready for rendering.
******************************************************************************/
void DataInspectorPanel::onScenePreparationBegin()
{
	_waitingForSceneAnim.start();
	QTimer::singleShot(400, this, [this]() {
		if(_waitingForSceneAnim.state() == QMovie::Running)
			_waitingForSceneIndicator->show();
	});
}

/******************************************************************************
*  Is called whenever the scene became ready for rendering.
******************************************************************************/
void DataInspectorPanel::onScenePreparationEnd()
{
	_waitingForSceneIndicator->hide();
	_waitingForSceneAnim.stop();
	_updateInvocation(this);
}

/******************************************************************************
* Is called whenever the inspector panel was resized.
******************************************************************************/
void DataInspectorPanel::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	bool isActive = (_appletContainer->height() > 0);
	if(!_inspectorActive && isActive) {
		_inspectorActive = true;
		if(_activeAppletIndex >= 0 && _activeAppletIndex < _applets.size()) {
			const PipelineFlowState& pipelineState = _selectedNodeListener.target() ? 
				_selectedNodeListener.target()->evaluatePipelinePreliminary(true) :
				PipelineFlowState();
			_applets[_activeAppletIndex]->updateDisplay(pipelineState, _selectedNodeListener.target());
		}
	}
	else if(_inspectorActive && !isActive) {
		_inspectorActive = false;
		if(_activeAppletIndex >= 0 && _activeAppletIndex < _applets.size()) {
			_applets[_activeAppletIndex]->deactivate(_mainWindow);
		}
	}
}

/******************************************************************************
* Updates the contents displayed in the data inpector.
******************************************************************************/
void DataInspectorPanel::updateInspector()
{
	// Obtain the pipeline output of the currently selected scene node.
	const PipelineFlowState& pipelineState = _selectedNodeListener.target() ? 
		_selectedNodeListener.target()->evaluatePipelinePreliminary(true) :
		PipelineFlowState();

	// Update the list of displayed tabs.
	updateTabs(pipelineState);

	// Update displayed content.
	if(_inspectorActive) {
		if(_activeAppletIndex >= 0 && _activeAppletIndex < _applets.size()) {
			_applets[_activeAppletIndex]->updateDisplay(pipelineState, _selectedNodeListener.target());
		}
	}
}

/******************************************************************************
* Updates the list of visible tabs.
******************************************************************************/
void DataInspectorPanel::updateTabs(const PipelineFlowState& pipelineState)
{
	OVITO_ASSERT(_appletsToTabs.size() == _applets.size());
	size_t numActiveApplets = 0;

	// Remove tabs that have become inactive.
	for(int appletIndex = _applets.size()-1; appletIndex >= 0; appletIndex--) {
		if(_appletsToTabs[appletIndex] != -1) {
			DataInspectionApplet* applet = _applets[appletIndex];
			if(!applet->appliesTo(pipelineState)) {
				int tabIndex = _appletsToTabs[appletIndex];
				_appletsToTabs[appletIndex] = -1;
				// Shift tab indices.
				for(int i = appletIndex + 1; i < _applets.size(); i++)
					if(_appletsToTabs[i] != -1) _appletsToTabs[i]--;
				// Remove the tab for this applet.
				_tabBar->removeTab(tabIndex);
			}
			else numActiveApplets++;
		}
	}

	// Create tabs for applets that became active.
	int tabIndex = 0;
	for(int appletIndex = 0; appletIndex < _applets.size(); appletIndex++) {
		if(_appletsToTabs[appletIndex] == -1) {
			DataInspectionApplet* applet = _applets[appletIndex];
			if(applet->appliesTo(pipelineState)) {
				_appletsToTabs[appletIndex] = tabIndex;
				// Shift tab indices.
				for(int i = appletIndex + 1; i < _applets.size(); i++)
					if(_appletsToTabs[i] != -1) _appletsToTabs[i]++;
				// Create a new tab for the applet.
				_tabBar->insertTab(tabIndex, applet->getOOClass().displayName());
				tabIndex++;
				numActiveApplets++;
			}
		}
		else tabIndex = _appletsToTabs[appletIndex]+1;
	}

	// Show the "Data Inspector" default tab if there are no active applets.
	if(numActiveApplets == 0 && _tabBar->count() == 0) {
		_tabBar->addTab(tr("Data Inspector"));
	}
	else if(numActiveApplets != 0 && _tabBar->count() != numActiveApplets) {
		if(_tabBar->currentIndex() == _tabBar->count() - 1)
			_tabBar->setCurrentIndex(0);
		_tabBar->removeTab(_tabBar->count() - 1);
	}
}

/******************************************************************************
* Is called when the user selects a new tab.
******************************************************************************/
void DataInspectorPanel::onCurrentTabChanged(int tabIndex)
{
	int appletIndex = _applets.size();
	if(tabIndex >= 0)
		appletIndex = std::find(_appletsToTabs.begin(), _appletsToTabs.end(), tabIndex) - _appletsToTabs.begin();
	OVITO_ASSERT(appletIndex >= 0 && appletIndex < _appletContainer->count());
	_appletContainer->setCurrentIndex(appletIndex);
}

/******************************************************************************
* Is called whenever the user has switched to a different page of the inspector.
******************************************************************************/
void DataInspectorPanel::onCurrentPageChanged(int index)
{
	if(_activeAppletIndex >= 0 && _activeAppletIndex < _applets.size()) {
		_applets[_activeAppletIndex]->deactivate(_mainWindow);
	}
	
	_activeAppletIndex = index;

	if(_activeAppletIndex >= 0 && _activeAppletIndex < _applets.size()) {
		// Obtain the pipeline output of the currently selected scene node.
		const PipelineFlowState& pipelineState = _selectedNodeListener.target() ? 
			_selectedNodeListener.target()->evaluatePipelinePreliminary(true) :
			PipelineFlowState();
		_applets[_activeAppletIndex]->updateDisplay(pipelineState, _selectedNodeListener.target());
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
