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
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/dataset/scene/PipelineSceneNode.h>
#include <core/dataset/io/FileSource.h>
#include <core/oo/CloneHelper.h>
#include "ClonePipelineDialog.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* Constructor.
******************************************************************************/
ClonePipelineDialog::ClonePipelineDialog(PipelineSceneNode* node, QWidget* parent) :
	QDialog(parent), _originalNode(node)
{
	setWindowTitle(tr("Clone pipeline"));

	initializeGraphicsScene();
	
	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	QGroupBox* pipelineBox = new QGroupBox(tr("Pipeline layout"));
	mainLayout->addWidget(pipelineBox);

	QVBoxLayout* sublayout1 = new QVBoxLayout(pipelineBox);
	_pipelineView = new QGraphicsView(&_pipelineScene, this);
	_pipelineView->setSceneRect(_pipelineView->sceneRect().marginsAdded(QMarginsF(15,15,15,15)));
	_pipelineView->setRenderHint(QPainter::Antialiasing);
	sublayout1->addWidget(_pipelineView, 1);

	QGroupBox* displacementBox = new QGroupBox(tr("Displace cloned pipeline"));
	mainLayout->addWidget(displacementBox);
	QHBoxLayout* sublayout2 = new QHBoxLayout(displacementBox);
	QToolBar* displacementToolBar = new QToolBar(displacementBox);
	displacementToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
	displacementToolBar->setIconSize(QSize(64,64));
	displacementToolBar->setStyleSheet("QToolBar { padding: 0px; margin: 0px; border: 0px none black; spacing: 8px; } QToolButton { padding: 0px; margin: 0px; }");
	sublayout2->addWidget(displacementToolBar);
	_displacementDirectionGroup = new QActionGroup(this);
	_displacementDirectionGroup->setExclusive(true);
	QAction* displacementNoneAction = displacementToolBar->addAction(QIcon(":/gui/actions/edit/clone_displace_mode_none.svg"), tr("Do not displace clone"));
	QAction* displacementXAction = displacementToolBar->addAction(QIcon(":/gui/actions/edit/clone_displace_mode_x.svg"), tr("Displace clone along X axis"));
	QAction* displacementYAction = displacementToolBar->addAction(QIcon(":/gui/actions/edit/clone_displace_mode_y.svg"), tr("Displace clone along Y axis"));
	QAction* displacementZAction = displacementToolBar->addAction(QIcon(":/gui/actions/edit/clone_displace_mode_z.svg"), tr("Displace clone along Z axis"));
	sublayout2->addStretch(1);
	displacementNoneAction->setCheckable(true);
	displacementXAction->setCheckable(true);
	displacementYAction->setCheckable(true);
	displacementZAction->setCheckable(true);
	displacementXAction->setChecked(true);
	displacementNoneAction->setData(-1);
	displacementXAction->setData(0);
	displacementYAction->setData(1);
	displacementZAction->setData(2);
	_displacementDirectionGroup->addAction(displacementNoneAction);
	_displacementDirectionGroup->addAction(displacementXAction);
	_displacementDirectionGroup->addAction(displacementYAction);
	_displacementDirectionGroup->addAction(displacementZAction);

	QGroupBox* nameBox = new QGroupBox(tr("Pipeline names"));
	mainLayout->addWidget(nameBox);
	sublayout2 = new QHBoxLayout(nameBox);
	sublayout2->setSpacing(2);
	_originalNameEdit = new QLineEdit(nameBox);
	_cloneNameEdit = new QLineEdit(nameBox);
	sublayout2->addWidget(new QLabel(tr("Original:")));
	sublayout2->addWidget(_originalNameEdit, 1);
	sublayout2->addSpacing(10);
	sublayout2->addWidget(new QLabel(tr("Clone:")));
	sublayout2->addWidget(_cloneNameEdit, 1);
	_originalNameEdit->setPlaceholderText(node->objectTitle());
	_cloneNameEdit->setPlaceholderText(node->objectTitle());

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help, Qt::Horizontal, this);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &ClonePipelineDialog::onAccept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &ClonePipelineDialog::reject);
	//connect(buttonBox, &QDialogButtonBox::helpRequested, [this]() {
	//	MainWindow::openHelpTopic("viewports.adjust_view_dialog.html");
	//});
	mainLayout->addWidget(buttonBox);
}

/******************************************************************************
* Builds the initial Qt graphics scene to visualize the pipeline layout.
******************************************************************************/
void ClonePipelineDialog::initializeGraphicsScene()
{
	// Obtain the list of objects that form the pipeline.
	PipelineObject* pobj = _originalNode->dataProvider();
	while(pobj) {
		PipelineItemStruct s;
		s.pipelineObject = pobj;
		s.modApp = dynamic_object_cast<ModifierApplication>(pobj);
		_pipelineItems.push_back(s);
		if(s.modApp)
			pobj = s.modApp->input();
		else
			break;
	}

	QPen borderPen(Qt::black);
	borderPen.setWidth(0);
	QPen thickBorderPen(Qt::black);
	thickBorderPen.setWidth(2);
	QBrush nodeBrush(QColor(200, 200, 255));
	QBrush modifierBrush(QColor(200, 255, 200));	
	QBrush sourceBrush(QColor(200, 200, 200));
	QBrush modAppBrush(QColor(255, 255, 200));
	qreal textBoxWidth = 160;
	qreal textBoxHeight = 25;
	qreal modAppRadius = 5;
	qreal objBoxIndent = textBoxWidth/2 + 20;
	qreal lineHeight = 50;
	_pipelineSeparation = 420;
	QFontMetrics fontMetrics(_pipelineScene.font());
	QFont smallFont = _pipelineScene.font();
	smallFont.setPointSizeF(smallFont.pointSizeF() * 3 / 4);

	QGraphicsSimpleTextItem* textItem;

	auto addShadowEffect = [this](QGraphicsItem* item) {
#if 0   // Shadows diabled to work around Qt bug https://bugreports.qt.io/browse/QTBUG-65035 
		QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect(this);
		effect->setOffset(3);
		effect->setBlurRadius(2);
		item->setGraphicsEffect(effect);
#endif
	};

	// Create the boxes for the two pipeline heads.
	QGraphicsRectItem* nodeItem1 = _pipelineScene.addRect(-textBoxWidth/2, -textBoxHeight/2, textBoxWidth, textBoxHeight, thickBorderPen, nodeBrush);
	nodeItem1->setZValue(1);
	textItem = _pipelineScene.addSimpleText(tr("Original pipeline"));
	textItem->setParentItem(nodeItem1);	
	textItem->setPos(-textItem->boundingRect().center());
	nodeItem1->moveBy(textBoxWidth * 0.25, 0);
	addShadowEffect(nodeItem1);
	QGraphicsRectItem* nodeItem2 = _pipelineScene.addRect(-textBoxWidth/2, -textBoxHeight/2, textBoxWidth, textBoxHeight, thickBorderPen, nodeBrush);
	nodeItem2->setZValue(1);
	nodeItem2->setPos(_pipelineSeparation, 0);
	nodeItem2->moveBy(-textBoxWidth * 0.25, 0);
	addShadowEffect(nodeItem2);
	textItem = _pipelineScene.addSimpleText(tr("Cloned pipeline"));
	textItem->setParentItem(nodeItem2);
	textItem->setPos(-textItem->boundingRect().center());

	_pipelineScene.addLine(0, 0, 0, lineHeight/2)->moveBy(0,0);
	_pipelineScene.addLine(0, 0, 0, lineHeight/2)->moveBy(_pipelineSeparation,0);
	_joinLine = _pipelineScene.addLine(0, -lineHeight/2, _pipelineSeparation, -lineHeight/2);
	textItem = _pipelineScene.addSimpleText(tr(" Pipeline branch "), smallFont);
	QGraphicsRectItem* boxItem = _pipelineScene.addRect(textItem->boundingRect(), borderPen, Qt::white);
	boxItem->setPos(-textItem->boundingRect().center());
	boxItem->moveBy(_pipelineSeparation/2, -lineHeight/2);
	boxItem->setParentItem(_joinLine);
	textItem->setParentItem(boxItem);

	QSignalMapper* unifiedMapper = new QSignalMapper(this);
	QSignalMapper* nonunifiedMapper = new QSignalMapper(this);

	int line = 1;
	for(PipelineItemStruct& s : _pipelineItems) {
		qreal y = line * lineHeight;

		// Create vertical connector lines.
		s.connector1 = _pipelineScene.addLine(0, -lineHeight/2, 0, s.modApp ? lineHeight/2 : 0);
		s.connector1->moveBy(0, y);
		s.connector2 = _pipelineScene.addLine(0, -lineHeight/2, 0, s.modApp ? lineHeight/2 : 0);
		s.connector2->moveBy(_pipelineSeparation, y);
		s.connector3 = _pipelineScene.addLine(0, -lineHeight/2, 0, s.modApp ? lineHeight/2 : 0);
		s.connector3->moveBy(_pipelineSeparation / 2 - objBoxIndent, y);

		// Create a circle for each modifier application:
		if(!s.modApp) modAppRadius = 0;
		s.modAppItem1 = _pipelineScene.addEllipse(-modAppRadius, -modAppRadius, modAppRadius*2, modAppRadius*2, borderPen, modAppBrush);
		s.modAppItem1->setParentItem(s.connector1);
		s.modAppItem2 = _pipelineScene.addEllipse(-modAppRadius, -modAppRadius, modAppRadius*2, modAppRadius*2, borderPen, modAppBrush);
		s.modAppItem2->setParentItem(s.connector2);
		s.modAppItem3 = _pipelineScene.addEllipse(-modAppRadius, -modAppRadius, modAppRadius*2, modAppRadius*2, borderPen, modAppBrush);
		s.modAppItem3->setParentItem(s.connector3);

		// Create horizontal connector lines.
		QGraphicsLineItem* horizontalConnector1 = _pipelineScene.addLine(modAppRadius, 0, (_pipelineSeparation - textBoxWidth) / 2, 0);
		horizontalConnector1->setParentItem(s.modAppItem1);
		QGraphicsLineItem* horizontalConnector2 = _pipelineScene.addLine(-modAppRadius, 0, -(_pipelineSeparation - textBoxWidth) / 2, 0);
		horizontalConnector2->setParentItem(s.modAppItem2);
		QGraphicsLineItem* horizontalConnector3 = _pipelineScene.addLine(modAppRadius, 0, objBoxIndent, 0);
		horizontalConnector3->setParentItem(s.modAppItem3);

		// Create the boxes for the pipeline objects.
		QString labelText = s.modApp ? s.modApp->modifier()->objectTitle() : (tr("Source: ") + s.pipelineObject->objectTitle());
		QString elidedText = fontMetrics.elidedText(labelText, Qt::ElideRight, (int)textBoxWidth);
		s.objItem1 = _pipelineScene.addRect(-textBoxWidth/2, -textBoxHeight/2, textBoxWidth, textBoxHeight, borderPen, s.modApp ? modifierBrush : sourceBrush);
		textItem = _pipelineScene.addSimpleText(elidedText);
		textItem->setParentItem(s.objItem1);	
		textItem->setPos(-textItem->boundingRect().center());		
		s.objItem1->setPos(objBoxIndent, y);
		addShadowEffect(s.objItem1);
		s.objItem2 = _pipelineScene.addRect(-textBoxWidth/2, -textBoxHeight/2, textBoxWidth, textBoxHeight, borderPen, s.modApp ? modifierBrush : sourceBrush);
		s.objItem2->setPos(_pipelineSeparation - objBoxIndent, y);
		addShadowEffect(s.objItem2);
		textItem = _pipelineScene.addSimpleText(elidedText);
		textItem->setParentItem(s.objItem2);
		textItem->setPos(-textItem->boundingRect().center());
		s.objItem3 = _pipelineScene.addRect(-textBoxWidth/2, -textBoxHeight/2, textBoxWidth, textBoxHeight, borderPen, s.modApp ? modifierBrush : sourceBrush);
		textItem = _pipelineScene.addSimpleText(elidedText);
		textItem->setParentItem(s.objItem3);
		textItem->setPos(-textItem->boundingRect().center());
		s.objItem3->setPos(_pipelineSeparation / 2, y);
		addShadowEffect(s.objItem3);

		QToolBar* modeSelectorBar = new QToolBar();
		modeSelectorBar->setStyleSheet(
			"QToolBar { "
			"   padding: 4px; margin: 0px; border: 0px none black; spacing: 4px; "
			"   background: none; "
			"} "
			"QToolButton { "
			"   padding: 4px; "
			"   border-radius: 2px; "
			"   border: 1px outset #8f8f91; "
			"   background-color: rgb(220,220,220); "
			"} "
			"QToolButton:pressed { "
			"   border-style: inset; "
      		"   background-color: rgb(240,240,240); "
			"} "
			"QToolButton:checked { "
			"   border-style: inset; "
      		"   background-color: rgb(180,180,220); "
			"}");
		QAction* copyAction = modeSelectorBar->addAction(tr("Copy"));
		QAction* joinAction = modeSelectorBar->addAction(tr("Join"));
		unifiedMapper->setMapping(joinAction, line-1);
		connect(joinAction, &QAction::triggered, unifiedMapper, (void (QSignalMapper::*)())&QSignalMapper::map);
		nonunifiedMapper->setMapping(copyAction, line-1);
		connect(copyAction, &QAction::triggered, nonunifiedMapper, (void (QSignalMapper::*)())&QSignalMapper::map);
		QAction* shareAction = nullptr;
		QAction* skipAction = nullptr;
		if(s.modApp) {
			shareAction = modeSelectorBar->addAction(tr("Share"));
			skipAction = modeSelectorBar->addAction(tr("Skip"));
			nonunifiedMapper->setMapping(shareAction, line-1);
			connect(shareAction, &QAction::triggered, nonunifiedMapper, (void (QSignalMapper::*)())&QSignalMapper::map);
			nonunifiedMapper->setMapping(skipAction, line-1);
			connect(skipAction, &QAction::triggered, nonunifiedMapper, (void (QSignalMapper::*)())&QSignalMapper::map);
		}
		copyAction->setCheckable(true);
		joinAction->setCheckable(true);
		if(shareAction) shareAction->setCheckable(true);
		if(skipAction) skipAction->setCheckable(true);
		s.actionGroup = new QActionGroup(this);
		s.actionGroup->setExclusive(true);
		s.actionGroup->addAction(copyAction);
		s.actionGroup->addAction(joinAction);
		if(shareAction) s.actionGroup->addAction(shareAction);
		if(skipAction) s.actionGroup->addAction(skipAction);
		copyAction->setData((int)CloneMode::Copy);
		joinAction->setData((int)CloneMode::Join);
		if(shareAction) shareAction->setData((int)CloneMode::Share);
		if(skipAction) skipAction->setData((int)CloneMode::Skip);
		connect(s.actionGroup, &QActionGroup::triggered, this, &ClonePipelineDialog::updateGraphicsScene);
		modeSelectorBar->setToolButtonStyle(Qt::ToolButtonTextOnly);
		QGraphicsProxyWidget* selectorItem = _pipelineScene.addWidget(modeSelectorBar);
		selectorItem->setPos(0, -selectorItem->boundingRect().center().y());
		selectorItem->moveBy(_pipelineSeparation + 40, y);
		if(s.modApp)
			copyAction->setChecked(true);
		else
			joinAction->setChecked(true);

		if(line == 1) {
			textItem = _pipelineScene.addSimpleText(tr("Clone mode:"));
			textItem->setPos(-textItem->boundingRect().center() + selectorItem->boundingRect().center());
			textItem->moveBy(_pipelineSeparation + 40, 0);
		}

		line++;
	}

	// When the user switches an entry to 'join', then all following entries must automatically be set to 'join' too.
	connect(unifiedMapper, (void (QSignalMapper::*)(int))&QSignalMapper::mapped, this, [this](int index) {
		for(; index < _pipelineItems.size(); index++) {
			_pipelineItems[index].setCloneMode(CloneMode::Join);
		}
	});

	// When the user switches to an entry other than 'join', then all preceding entries must automatically be set to something other too.
	connect(nonunifiedMapper, (void (QSignalMapper::*)(int))&QSignalMapper::mapped, this, [this](int index) {
		for(index--; index >= 0; index--) {
			if(_pipelineItems[index].cloneMode() == CloneMode::Join)
				_pipelineItems[index].setCloneMode(CloneMode::Copy);
		}
	});

	updateGraphicsScene();
}

/******************************************************************************
* Updates the display of the pipeline layout.
******************************************************************************/
void ClonePipelineDialog::updateGraphicsScene()
{
	_joinLine->hide();
	for(size_t i = 0; i < _pipelineItems.size(); i++) {
		const PipelineItemStruct& s = _pipelineItems[i];

		switch(s.cloneMode()) {
		case CloneMode::Copy:
			s.objItem1->show();
			s.objItem2->show();
			s.objItem3->hide();
			s.connector1->show();
			s.connector2->show();
			s.connector3->hide();
			s.modAppItem1->show();
			s.modAppItem2->show();
			s.modAppItem3->hide();
			break;
		case CloneMode::Share:
			s.objItem1->hide();
			s.objItem2->hide();
			s.objItem3->show();
			s.connector1->show();
			s.connector2->show();
			s.connector3->hide();
			s.modAppItem1->show();
			s.modAppItem2->show();
			s.modAppItem3->hide();
			break;
		case CloneMode::Join:
			s.objItem1->hide();
			s.objItem2->hide();
			s.objItem3->show();
			s.connector1->hide();
			s.connector2->hide();
			s.connector3->show();
			s.modAppItem1->hide();
			s.modAppItem2->hide();
			s.modAppItem3->show();
			if(!_joinLine->isVisible()) {
				_joinLine->setPos(0, s.objItem1->y());
				_joinLine->show();
			}
			break;
		case CloneMode::Skip:
			s.objItem1->show();
			s.objItem2->hide();
			s.objItem3->hide();
			s.connector1->show();
			s.connector2->show();
			s.connector3->hide();
			s.modAppItem1->show();
			s.modAppItem2->hide();
			s.modAppItem3->hide();
			break;
		}
	}
}

/******************************************************************************
* Is called when the user has pressed the 'Ok' button
******************************************************************************/
void ClonePipelineDialog::onAccept()
{
	UndoableTransaction::handleExceptions(_originalNode->dataset()->undoStack(), tr("Clone pipeline"), [this]() {
		if(_pipelineItems.empty()) return;

		// Do not create any animation keys during cloning.
		AnimationSuspender animSuspender(_originalNode);
		
		// Clone the scene node.
		CloneHelper cloneHelper;
		OORef<PipelineSceneNode> clonedPipeline = cloneHelper.cloneObject(_originalNode, false);
		OVITO_ASSERT(clonedPipeline->dataProvider() == _originalNode->dataProvider());

		// Clone the pipeline objects.
		OORef<PipelineObject> precedingObj;
		for(auto s = _pipelineItems.crbegin(); s != _pipelineItems.crend(); ++s) {
			if(s->cloneMode() == CloneMode::Join) {
				precedingObj = s->pipelineObject;
			}
			else if(s->cloneMode() == CloneMode::Copy) {
				OORef<PipelineObject> clonedObject = cloneHelper.cloneObject(s->pipelineObject, false);
				if(ModifierApplication* clonedModApp = dynamic_object_cast<ModifierApplication>(clonedObject)) {
					clonedModApp->setInput(precedingObj);
					clonedModApp->setModifier(cloneHelper.cloneObject(clonedModApp->modifier(), true));
				}
				else if(FileSource* clonedFileSource = dynamic_object_cast<FileSource>(clonedObject)) {
					clonedFileSource->setAdjustAnimationIntervalEnabled(false);
				}
				precedingObj = clonedObject;
			}
			else if(s->cloneMode() == CloneMode::Share) {
				OORef<ModifierApplication> clonedModApp = cloneHelper.cloneObject(s->modApp, false);
				clonedModApp->setInput(precedingObj);
				precedingObj = clonedModApp;
			}
			else if(s->cloneMode() == CloneMode::Skip) {
				continue;
			}
		}		
		clonedPipeline->setDataProvider(precedingObj);

		// Give the cloned pipeline the user-defined name.
		QString nodeName = _cloneNameEdit->text().trimmed();
		if(nodeName.isEmpty() == false)
			clonedPipeline->setNodeName(nodeName);

		// Give the original pipeline the user-defined name.
		nodeName = _originalNameEdit->text().trimmed();
		if(nodeName.isEmpty() == false)
			_originalNode->setNodeName(nodeName);
		
		// Translate cloned pipeline.
		int displacementMode = _displacementDirectionGroup->checkedAction()->data().toInt();
		if(displacementMode != -1) {
			TimePoint time = _originalNode->dataset()->animationSettings()->time();
			const Box3& bbox = _originalNode->worldBoundingBox(time);
			Vector3 translation = Vector3::Zero();
			translation[displacementMode] = bbox.size(displacementMode) + FloatType(0.2) * bbox.size().length();
			clonedPipeline->transformationController()->translate(time, translation, AffineTransformation::Identity());
		}		

		// Insert cloned pipeline into scene.
		_originalNode->parentNode()->addChildNode(clonedPipeline);

		// Select cloned pipeline.
		_originalNode->dataset()->selection()->setNode(clonedPipeline);
	});
	accept();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
