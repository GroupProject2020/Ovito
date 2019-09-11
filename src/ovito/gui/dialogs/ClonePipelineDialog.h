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


#include <ovito/gui/GUI.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * This dialog box lets the user make a copy of a pipeline scene node.
 */
class ClonePipelineDialog : public QDialog
{
	Q_OBJECT

public:

	enum CloneMode {
		Copy,
		Join,
		Share,
		Skip
	};

	/// Constructor.
	ClonePipelineDialog(PipelineSceneNode* node, QWidget* parentWindow = nullptr);

private Q_SLOTS:

	/// Is called when the user has pressed the 'Ok' button.
	void onAccept();

	/// Updates the display of the pipeline layout.
	void updateGraphicsScene();

private:

	/// Builds the initial Qt graphics scene to visualize the pipeline layout.
	void initializeGraphicsScene();

	/// Data structure that is created for every pipeline object.
	struct PipelineItemStruct {
		OORef<PipelineObject> pipelineObject;
		ModifierApplication* modApp;
		QGraphicsItem* connector1;
		QGraphicsItem* connector2;
		QGraphicsItem* connector3;
		QGraphicsItem* modAppItem1;
		QGraphicsItem* modAppItem2;
		QGraphicsItem* modAppItem3;
		QGraphicsItem* objItem1;
		QGraphicsItem* objItem2;
		QGraphicsItem* objItem3;
		QActionGroup* actionGroup;
		CloneMode cloneMode() const { return (CloneMode)actionGroup->checkedAction()->data().toInt(); }
		void setCloneMode(CloneMode mode) { return actionGroup->actions()[mode]->setChecked(true); }
	};

	/// The graphics scene for the pipeline layout.
	QGraphicsScene _pipelineScene;

	/// Widget that displays the current pipeline layout.
	QGraphicsView* _pipelineView;

	/// The original scene node to be cloned.
	OORef<PipelineSceneNode> _originalNode;

	/// One structure for each pipeline object.
	std::vector<PipelineItemStruct> _pipelineItems;

	/// Distance between the two pipelines.
	qreal _pipelineSeparation;

	QGraphicsItem* _joinLine;

	QActionGroup* _displacementDirectionGroup;
	QLineEdit* _originalNameEdit;
	QLineEdit* _cloneNameEdit;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


