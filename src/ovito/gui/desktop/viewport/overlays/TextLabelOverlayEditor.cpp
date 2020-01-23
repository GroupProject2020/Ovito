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

#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/properties/StringParameterUI.h>
#include <ovito/gui/desktop/properties/ColorParameterUI.h>
#include <ovito/gui/desktop/properties/FontParameterUI.h>
#include <ovito/gui/desktop/properties/FloatParameterUI.h>
#include <ovito/gui/desktop/properties/VariantComboBoxParameterUI.h>
#include <ovito/gui/desktop/properties/CustomParameterUI.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include <ovito/gui/desktop/widgets/general/AutocompleteTextEdit.h>
#include <ovito/gui/desktop/actions/ViewportModeAction.h>
#include <ovito/gui/viewport/overlays/MoveOverlayInputMode.h>
#include <ovito/core/viewport/overlays/TextLabelOverlay.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/scene/RootSceneNode.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include "TextLabelOverlayEditor.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(View) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(TextLabelOverlayEditor);
SET_OVITO_OBJECT_EDITOR(TextLabelOverlay, TextLabelOverlayEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void TextLabelOverlayEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Text label"), rolloutParams, "viewport_layers.text_label.html");

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 3);
	layout->setColumnStretch(2, 1);
	int row = 0;

	// This widget displays the list of available pipeline nodes in the current scene.
	class PipelineSceneNodeComboBox : public QComboBox {
	public:
		/// Initializes the widget.
		PipelineSceneNodeComboBox(QWidget* parent = nullptr) : QComboBox(parent), _overlay(nullptr) {}

		/// Sets the overlay being edited.
		void setOverlay(TextLabelOverlay* overlay) { _overlay = overlay; }

		/// Is called just before the drop-down box is activated.
		virtual void showPopup() override {
			clear();
			if(_overlay) {
				// Find all ObjectNodes in the scene.
				_overlay->dataset()->sceneRoot()->visitObjectNodes([this](PipelineSceneNode* node) {
					addItem(node->objectTitle(), QVariant::fromValue(node));
					return true;
				});
				setCurrentIndex(findData(QVariant::fromValue(_overlay->sourceNode())));
			}
			if(count() == 0) addItem(tr("<none>"));
			QComboBox::showPopup();
		}

	private:
		TextLabelOverlay* _overlay;
	};

	PipelineSceneNodeComboBox* nodeComboBox = new PipelineSceneNodeComboBox();
	CustomParameterUI* sourcePUI = new CustomParameterUI(this, "sourceNode", nodeComboBox,
			[nodeComboBox](const QVariant& value) {
				nodeComboBox->clear();
				PipelineSceneNode* node = dynamic_object_cast<PipelineSceneNode>(value.value<PipelineSceneNode*>());
				if(node) {
					nodeComboBox->addItem(node->objectTitle(), QVariant::fromValue(node));
				}
				else {
					nodeComboBox->addItem(tr("<none>"));
				}
				nodeComboBox->setCurrentIndex(0);
			},
			[nodeComboBox]() {
				return nodeComboBox->currentData();
			},
			[nodeComboBox](RefTarget* editObject) {
				nodeComboBox->setOverlay(dynamic_object_cast<TextLabelOverlay>(editObject));
			});
	connect(nodeComboBox, (void (QComboBox::*)(int))&QComboBox::activated, sourcePUI, &CustomParameterUI::updatePropertyValue);
	layout->addWidget(new QLabel(tr("Data source:")), row, 0);
	layout->addWidget(sourcePUI->widget(), row++, 1, 1, 2);

	// Label text.
	StringParameterUI* labelTextPUI = new StringParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::labelText));
	layout->addWidget(new QLabel(tr("Text:")), row, 0);
	_textEdit = new AutocompleteTextEdit();
	labelTextPUI->setTextBox(_textEdit);
	layout->addWidget(labelTextPUI->textBox(), row++, 1, 1, 2);

	VariantComboBoxParameterUI* alignmentPUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::alignment));
	layout->addWidget(new QLabel(tr("Position:")), row, 0);
	layout->addWidget(alignmentPUI->comboBox(), row++, 1);
	alignmentPUI->comboBox()->addItem(tr("Top"), QVariant::fromValue((int)(Qt::AlignTop | Qt::AlignHCenter)));
	alignmentPUI->comboBox()->addItem(tr("Top left"), QVariant::fromValue((int)(Qt::AlignTop | Qt::AlignLeft)));
	alignmentPUI->comboBox()->addItem(tr("Top right"), QVariant::fromValue((int)(Qt::AlignTop | Qt::AlignRight)));
	alignmentPUI->comboBox()->addItem(tr("Bottom"), QVariant::fromValue((int)(Qt::AlignBottom | Qt::AlignHCenter)));
	alignmentPUI->comboBox()->addItem(tr("Bottom left"), QVariant::fromValue((int)(Qt::AlignBottom | Qt::AlignLeft)));
	alignmentPUI->comboBox()->addItem(tr("Bottom right"), QVariant::fromValue((int)(Qt::AlignBottom | Qt::AlignRight)));
	alignmentPUI->comboBox()->addItem(tr("Left"), QVariant::fromValue((int)(Qt::AlignVCenter | Qt::AlignLeft)));
	alignmentPUI->comboBox()->addItem(tr("Right"), QVariant::fromValue((int)(Qt::AlignVCenter | Qt::AlignRight)));

	FloatParameterUI* offsetXPUI = new FloatParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::offsetX));
	layout->addWidget(offsetXPUI->label(), row, 0);
	layout->addLayout(offsetXPUI->createFieldLayout(), row++, 1, 1, 2);

	FloatParameterUI* offsetYPUI = new FloatParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::offsetY));
	layout->addWidget(offsetYPUI->label(), row, 0);
	layout->addLayout(offsetYPUI->createFieldLayout(), row++, 1, 1, 2);

	ViewportInputMode* moveOverlayMode = new MoveOverlayInputMode(this);
	connect(this, &QObject::destroyed, moveOverlayMode, &ViewportInputMode::removeMode);
	ViewportModeAction* moveOverlayAction = new ViewportModeAction(mainWindow(), tr("Move using mouse"), this, moveOverlayMode);
	layout->addWidget(moveOverlayAction->createPushButton(), row++, 1, 1, 2);

	FloatParameterUI* fontSizePUI = new FloatParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::fontSize));
	layout->addWidget(new QLabel(tr("Text size/color:")), row, 0);
	layout->addLayout(fontSizePUI->createFieldLayout(), row, 1);

	// Text color.
	ColorParameterUI* textColorPUI = new ColorParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::textColor));
	layout->addWidget(textColorPUI->colorPicker(), row++, 2);

	BooleanParameterUI* outlineEnabledPUI = new BooleanParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::outlineEnabled));
	layout->addWidget(outlineEnabledPUI->checkBox(), row, 1);

	ColorParameterUI* outlineColorPUI = new ColorParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::outlineColor));
	layout->addWidget(outlineColorPUI->colorPicker(), row++, 2);

	FontParameterUI* labelFontPUI = new FontParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::font));
	layout->addWidget(labelFontPUI->label(), row, 0);
	layout->addWidget(labelFontPUI->fontPicker(), row++, 1, 1, 2);

	QWidget* variablesRollout = createRollout(tr("Variables"), rolloutParams.after(rollout), "viewport_layers.text_label.html");
    QVBoxLayout* variablesLayout = new QVBoxLayout(variablesRollout);
    variablesLayout->setContentsMargins(4,4,4,4);
    _attributeNamesList = new QLabel();
    _attributeNamesList->setWordWrap(true);
    _attributeNamesList->setTextInteractionFlags(Qt::TextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard));
	variablesLayout->addWidget(_attributeNamesList);

	// Update input variables list if another modifier has been loaded into the editor.
	connect(this, &TextLabelOverlayEditor::contentsReplaced, this, &TextLabelOverlayEditor::updateEditorFields);
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool TextLabelOverlayEditor::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(source == editObject() && (event.type() == ReferenceEvent::TargetChanged || event.type() == ReferenceEvent::PreliminaryStateAvailable)) {
		updateEditorFieldsLater(this);
	}
	return PropertiesEditor::referenceEvent(source, event);
}

/******************************************************************************
* Updates the UI.
******************************************************************************/
void TextLabelOverlayEditor::updateEditorFields()
{
	QString str;
	QStringList variableNames;
	if(TextLabelOverlay* overlay = static_object_cast<TextLabelOverlay>(editObject())) {
		if(PipelineSceneNode* node = overlay->sourceNode()) {
			const PipelineFlowState& flowState = node->evaluatePipelinePreliminary(false);
			str.append(tr("<p>Dynamic attributes that can be referenced in the label text:</b><ul>"));
			if(flowState.data()) {
				for(const QString& attrName : flowState.data()->buildAttributesMap().keys()) {
					str.append(QStringLiteral("<li>[%1]</li>").arg(attrName.toHtmlEscaped()));
					variableNames.push_back(QStringLiteral("[") + attrName + QStringLiteral("]"));
				}
			}
			str.append(QStringLiteral("</ul></p><p></p>"));
		}
	}

	_attributeNamesList->setText(str);
	_attributeNamesList->updateGeometry();
	_textEdit->setWordList(variableNames);
	container()->updateRolloutsLater();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
