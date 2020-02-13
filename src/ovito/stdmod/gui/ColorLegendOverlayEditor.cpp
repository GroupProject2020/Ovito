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

#include <ovito/stdmod/gui/StdModGui.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include <ovito/gui/desktop/properties/StringParameterUI.h>
#include <ovito/gui/desktop/properties/ColorParameterUI.h>
#include <ovito/gui/desktop/properties/FontParameterUI.h>
#include <ovito/gui/desktop/properties/FloatParameterUI.h>
#include <ovito/gui/desktop/properties/Vector3ParameterUI.h>
#include <ovito/gui/desktop/properties/VariantComboBoxParameterUI.h>
#include <ovito/gui/desktop/properties/CustomParameterUI.h>
#include <ovito/gui/base/viewport/ViewportInputManager.h>
#include <ovito/gui/desktop/viewport/overlays/MoveOverlayInputMode.h>
#include <ovito/gui/desktop/actions/ViewportModeAction.h>
#include <ovito/core/dataset/scene/RootSceneNode.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/stdmod/viewport/ColorLegendOverlay.h>
#include "ColorLegendOverlayEditor.h"

namespace Ovito { namespace StdMod {

IMPLEMENT_OVITO_CLASS(ColorLegendOverlayEditor);
SET_OVITO_OBJECT_EDITOR(ColorLegendOverlay, ColorLegendOverlayEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void ColorLegendOverlayEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Color legend"), rolloutParams);

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 1);
	int row = 0;

	// This widget displays the list of available ColorCodingModifiers in the current scene.
	class ModifierComboBox : public QComboBox {
	public:
		/// Initializes the widget.
		ModifierComboBox(QWidget* parent = nullptr) : QComboBox(parent), _overlay(nullptr) {}

		/// Sets the overlay being edited.
		void setOverlay(ColorLegendOverlay* overlay) { _overlay = overlay; }

		/// Is called just before the drop-down box is activated.
		virtual void showPopup() override {
			clear();
			if(_overlay) {
				// Find all ColorCodingModifiers in the scene. For this we have to visit all
				// object nodes and iterate over their modification pipelines.
				_overlay->dataset()->sceneRoot()->visitObjectNodes([this](PipelineSceneNode* node) {
					PipelineObject* obj = node->dataProvider();
					while(obj) {
						if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(obj)) {
							if(ColorCodingModifier* mod = dynamic_object_cast<ColorCodingModifier>(modApp->modifier())) {
								addItem(mod->sourceProperty().nameWithComponent(), QVariant::fromValue(mod));
							}
							obj = modApp->input();
						}
						else break;
					}
					return true;
				});
				setCurrentIndex(findData(QVariant::fromValue(_overlay->modifier())));
			}
			if(count() == 0) addItem(QIcon(":/gui/mainwin/status/status_warning.png"), tr("<none>"));
			QComboBox::showPopup();
		}

	private:
		ColorLegendOverlay* _overlay;
	};

	ModifierComboBox* modifierComboBox = new ModifierComboBox();
	CustomParameterUI* modifierPUI = new CustomParameterUI(this, "modifier", modifierComboBox,
			[modifierComboBox](const QVariant& value) {
				modifierComboBox->clear();
				ColorCodingModifier* mod = dynamic_object_cast<ColorCodingModifier>(value.value<ColorCodingModifier*>());
				if(mod) {
					modifierComboBox->addItem(mod->sourceProperty().nameWithComponent(), QVariant::fromValue(mod));
				}
				else {
					modifierComboBox->addItem(QIcon(":/gui/mainwin/status/status_warning.png"), tr("<none>"));
				}
				modifierComboBox->setCurrentIndex(0);
			},
			[modifierComboBox]() {
				return modifierComboBox->currentData();
			},
			[modifierComboBox](RefTarget* editObject) {
				modifierComboBox->setOverlay(dynamic_object_cast<ColorLegendOverlay>(editObject));
			});
	connect(modifierComboBox, (void (QComboBox::*)(int))&QComboBox::activated, modifierPUI, &CustomParameterUI::updatePropertyValue);
	layout->addWidget(new QLabel(tr("Source modifier:")), row, 0);
	layout->addWidget(modifierPUI->widget(), row++, 1);

	QGroupBox* positionBox = new QGroupBox(tr("Position"));
	layout->addWidget(positionBox, row++, 0, 1, 2);
	QGridLayout* sublayout = new QGridLayout(positionBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(1, 1);
	int subrow = 0;

	VariantComboBoxParameterUI* alignmentPUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::alignment));
	sublayout->addWidget(alignmentPUI->comboBox(), subrow, 0);
	alignmentPUI->comboBox()->addItem(tr("Top"), QVariant::fromValue((int)(Qt::AlignTop | Qt::AlignHCenter)));
	alignmentPUI->comboBox()->addItem(tr("Top left"), QVariant::fromValue((int)(Qt::AlignTop | Qt::AlignLeft)));
	alignmentPUI->comboBox()->addItem(tr("Top right"), QVariant::fromValue((int)(Qt::AlignTop | Qt::AlignRight)));
	alignmentPUI->comboBox()->addItem(tr("Bottom"), QVariant::fromValue((int)(Qt::AlignBottom | Qt::AlignHCenter)));
	alignmentPUI->comboBox()->addItem(tr("Bottom left"), QVariant::fromValue((int)(Qt::AlignBottom | Qt::AlignLeft)));
	alignmentPUI->comboBox()->addItem(tr("Bottom right"), QVariant::fromValue((int)(Qt::AlignBottom | Qt::AlignRight)));
	alignmentPUI->comboBox()->addItem(tr("Left"), QVariant::fromValue((int)(Qt::AlignVCenter | Qt::AlignLeft)));
	alignmentPUI->comboBox()->addItem(tr("Right"), QVariant::fromValue((int)(Qt::AlignVCenter | Qt::AlignRight)));

	VariantComboBoxParameterUI* orientationPUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::orientation));
	sublayout->addWidget(orientationPUI->comboBox(), subrow++, 1);
	orientationPUI->comboBox()->addItem(tr("Vertical"), QVariant::fromValue((int)Qt::Vertical));
	orientationPUI->comboBox()->addItem(tr("Horizontal"), QVariant::fromValue((int)Qt::Horizontal));

	FloatParameterUI* offsetXPUI = new FloatParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::offsetX));
	sublayout->addWidget(offsetXPUI->label(), subrow, 0);
	sublayout->addLayout(offsetXPUI->createFieldLayout(), subrow++, 1);

	FloatParameterUI* offsetYPUI = new FloatParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::offsetY));
	sublayout->addWidget(offsetYPUI->label(), subrow, 0);
	sublayout->addLayout(offsetYPUI->createFieldLayout(), subrow++, 1);

	ViewportInputMode* moveOverlayMode = new MoveOverlayInputMode(this);
	connect(this, &QObject::destroyed, moveOverlayMode, &ViewportInputMode::removeMode);
	ViewportModeAction* moveOverlayAction = new ViewportModeAction(mainWindow(), tr("Move using mouse"), this, moveOverlayMode);
	sublayout->addWidget(moveOverlayAction->createPushButton(), subrow++, 0, 1, 2);

	QGroupBox* sizeBox = new QGroupBox(tr("Size"));
	layout->addWidget(sizeBox, row++, 0, 1, 2);
	sublayout = new QGridLayout(sizeBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(1, 1);
	subrow = 0;

	FloatParameterUI* sizePUI = new FloatParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::legendSize));
	sublayout->addWidget(sizePUI->label(), subrow, 0);
	sublayout->addLayout(sizePUI->createFieldLayout(), subrow++, 1);

	FloatParameterUI* aspectRatioPUI = new FloatParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::aspectRatio));
	sublayout->addWidget(aspectRatioPUI->label(), subrow, 0);
	sublayout->addLayout(aspectRatioPUI->createFieldLayout(), subrow++, 1);

	QGroupBox* labelBox = new QGroupBox(tr("Labels"));
	layout->addWidget(labelBox, row++, 0, 1, 2);
	sublayout = new QGridLayout(labelBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(1, 3);
	sublayout->setColumnStretch(2, 1);
	subrow = 0;

	StringParameterUI* titlePUI = new StringParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::title));
	sublayout->addWidget(new QLabel(tr("Custom title:")), subrow, 0);
	sublayout->addWidget(titlePUI->textBox(), subrow++, 1, 1, 2);

	StringParameterUI* label1PUI = new StringParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::label1));
	sublayout->addWidget(new QLabel(tr("Custom label 1:")), subrow, 0);
	sublayout->addWidget(label1PUI->textBox(), subrow++, 1, 1, 2);

	StringParameterUI* label2PUI = new StringParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::label2));
	sublayout->addWidget(new QLabel(tr("Custom label 2:")), subrow, 0);
	sublayout->addWidget(label2PUI->textBox(), subrow++, 1, 1, 2);

	StringParameterUI* valueFormatStringPUI = new StringParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::valueFormatString));
	sublayout->addWidget(new QLabel(tr("Format string:")), subrow, 0);
	sublayout->addWidget(valueFormatStringPUI->textBox(), subrow++, 1, 1, 2);

	FloatParameterUI* fontSizePUI = new FloatParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::fontSize));
	sublayout->addWidget(new QLabel(tr("Text size/color:")), subrow, 0);
	sublayout->addLayout(fontSizePUI->createFieldLayout(), subrow, 1);

	ColorParameterUI* textColorPUI = new ColorParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::textColor));
	sublayout->addWidget(textColorPUI->colorPicker(), subrow++, 2);

	BooleanParameterUI* outlineEnabledPUI = new BooleanParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::outlineEnabled));
	sublayout->addWidget(outlineEnabledPUI->checkBox(), subrow, 1);

	ColorParameterUI* outlineColorPUI = new ColorParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::outlineColor));
	sublayout->addWidget(outlineColorPUI->colorPicker(), subrow++, 2);

	FontParameterUI* labelFontPUI = new FontParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::font));
	sublayout->addWidget(labelFontPUI->label(), subrow, 0);
	sublayout->addWidget(labelFontPUI->fontPicker(), subrow++, 1, 1, 2);
}

}	// End of namespace
}	// End of namespace
