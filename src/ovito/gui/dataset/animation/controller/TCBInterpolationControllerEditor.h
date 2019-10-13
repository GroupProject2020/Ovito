////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2016 Alexander Stukowski
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
#include <ovito/gui/properties/PropertiesEditor.h>
#include <ovito/gui/properties/FloatParameterUI.h>
#include <ovito/core/dataset/animation/controller/TCBInterpolationControllers.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Anim) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * A properties editor template for the TCBAnimationKey class template.
 */
template<class TCBAnimationKeyType>
class TCBAnimationKeyEditor : public PropertiesEditor
{
protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override {
		// Create the rollout.
		QWidget* rollout = createRollout(tr("TCB Animation Key"), rolloutParams);

		QVBoxLayout* layout = new QVBoxLayout(rollout);
		layout->setContentsMargins(4,4,4,4);
		layout->setSpacing(2);

		QGridLayout* sublayout = new QGridLayout();
		sublayout->setContentsMargins(0,0,0,0);
		sublayout->setColumnStretch(2, 1);
		layout->addLayout(sublayout);

		// Ease to parameter.
		FloatParameterUI* easeInPUI = new FloatParameterUI(this, PROPERTY_FIELD(TCBAnimationKeyType::easeTo));
		sublayout->addWidget(easeInPUI->label(), 0, 0);
		sublayout->addLayout(easeInPUI->createFieldLayout(), 0, 1);

		// Ease from parameter.
		FloatParameterUI* easeFromPUI = new FloatParameterUI(this, PROPERTY_FIELD(TCBAnimationKeyType::easeFrom));
		sublayout->addWidget(easeFromPUI->label(), 1, 0);
		sublayout->addLayout(easeFromPUI->createFieldLayout(), 1, 1);

		// Tension parameter.
		FloatParameterUI* tensionPUI = new FloatParameterUI(this, PROPERTY_FIELD(TCBAnimationKeyType::tension));
		sublayout->addWidget(tensionPUI->label(), 2, 0);
		sublayout->addLayout(tensionPUI->createFieldLayout(), 2, 1);

		// Continuity parameter.
		FloatParameterUI* continuityPUI = new FloatParameterUI(this, PROPERTY_FIELD(TCBAnimationKeyType::continuity));
		sublayout->addWidget(continuityPUI->label(), 3, 0);
		sublayout->addLayout(continuityPUI->createFieldLayout(), 3, 1);

		// Continuity parameter.
		FloatParameterUI* biasPUI = new FloatParameterUI(this, PROPERTY_FIELD(TCBAnimationKeyType::bias));
		sublayout->addWidget(biasPUI->label(), 4, 0);
		sublayout->addLayout(biasPUI->createFieldLayout(), 4, 1);
	}
};

/**
 * A properties editor for the PositionTCBAnimationKey class.
 */
class PositionTCBAnimationKeyEditor : public TCBAnimationKeyEditor<PositionTCBAnimationKey>
{
	Q_OBJECT
	OVITO_CLASS(PositionTCBAnimationKeyEditor)

public:

	/// Constructor.
	Q_INVOKABLE PositionTCBAnimationKeyEditor() {}
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


