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
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/DataSetContainer.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * A combo-box widget that allows to select the current animation frame.
 */
class AnimationFramesToolButton : public QToolButton
{
	Q_OBJECT

public:

	/// Constructs the widget.
	AnimationFramesToolButton(DataSetContainer& datasetContainer, QWidget* parent = 0) : QToolButton(parent), _datasetContainer(datasetContainer) {
		setIcon(QIcon(QString(":/gui/actions/animation/named_frames.svg")));
		setToolTip(tr("Jump to animation frame"));
		setFocusPolicy(Qt::NoFocus);
		connect(this, &QToolButton::clicked, this, &AnimationFramesToolButton::onClicked);
	}

protected Q_SLOTS:

	void onClicked() {
		QMenu menu;

		AnimationSettings* animSettings = _datasetContainer.currentSet()->animationSettings();
		int currentFrame = animSettings->time() / animSettings->ticksPerFrame();
		for(auto entry = animSettings->namedFrames().cbegin(); entry != animSettings->namedFrames().cend(); ++entry) {
			QAction* action = menu.addAction(entry.value());
			action->setCheckable(true);
			action->setData(entry.key());
			if(currentFrame == entry.key()) {
				action->setChecked(true);
				menu.setActiveAction(action);
			}
		}
		if(animSettings->namedFrames().isEmpty()) {
			QAction* action = menu.addAction(tr("No animation frames loaded"));
			action->setEnabled(false);
		}

		connect(&menu, &QMenu::triggered, this, &AnimationFramesToolButton::onActionTriggered);
		menu.exec(mapToGlobal(QPoint(0, 0)));
	}

	void onActionTriggered(QAction* action) {
		if(action->data().isValid()) {
			int frameIndex = action->data().value<int>();
			AnimationSettings* animSettings = _datasetContainer.currentSet()->animationSettings();
			animSettings->setTime(animSettings->frameToTime(frameIndex));
		}
	}

private:

	DataSetContainer& _datasetContainer;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


