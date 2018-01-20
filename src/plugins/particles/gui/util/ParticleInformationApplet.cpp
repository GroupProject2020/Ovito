///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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

#include <plugins/particles/gui/ParticlesGui.h>
#include <core/dataset/animation/AnimationSettings.h>
#include <core/viewport/Viewport.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/dataset/scene/SelectionSet.h>
#include <gui/rendering/ViewportSceneRenderer.h>
#include <gui/actions/ViewportModeAction.h>
#include <gui/mainwin/MainWindow.h>
#include <gui/viewport/ViewportWindow.h>
#include <gui/widgets/general/AutocompleteTextEdit.h>
#include <plugins/particles/objects/ParticleProperty.h>
#include <plugins/particles/util/ParticleExpressionEvaluator.h>
#include "ParticleInformationApplet.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(ParticleInformationApplet);

/******************************************************************************
* Shows the UI of the utility in the given RolloutContainer.
******************************************************************************/
void ParticleInformationApplet::openUtility(MainWindow* mainWindow, RolloutContainer* container, const RolloutInsertionParameters& rolloutParams)
{
	OVITO_ASSERT(_panel == nullptr);
	_mainWindow = mainWindow;

	// Create a rollout.
	_panel = new QWidget();
	container->addRollout(_panel, tr("Particle information"), rolloutParams.useAvailableSpace(), "utilities.particle_inspection.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(_panel);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(2);

	_inputMode = new ParticleInformationInputMode(this);
	ViewportModeAction* pickModeAction = new ViewportModeAction(_mainWindow, tr("Selection mode"), this, _inputMode);

	layout->addWidget(new QLabel(tr("Particle selection expression:")));
	_expressionEdit = new AutocompleteTextEdit();
	layout->addWidget(_expressionEdit);
	connect(_expressionEdit, &AutocompleteTextEdit::editingFinished, this, [this]() {
		_userSelectionExpression = _expressionEdit->toPlainText();
		updateInformationDisplay();
	});

	layout->addSpacing(2);
	_displayHeader = new QLabel(tr("Particle information:"));
	layout->addWidget(_displayHeader);
	_infoDisplay = new QTextEdit(_panel);
	_infoDisplay->setReadOnly(true);
	_infoDisplay->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
#ifndef Q_OS_MACX
	_infoDisplay->setText(tr("Pick a particle in the viewports. Hold down the CONTROL key to select multiple particles."));
#else
	_infoDisplay->setText(tr("Pick a particle in the viewports. Hold down the COMMAND key to select multiple particles."));
#endif
	layout->addWidget(_infoDisplay, 1);

	// Install signal handlers to automatically update displayed information upon animation time change.
	connect(&_mainWindow->datasetContainer(), &DataSetContainer::animationSettingsReplaced, this, &ParticleInformationApplet::onAnimationSettingsReplaced);
	_timeChangeCompleteConnection = connect(_mainWindow->datasetContainer().currentSet()->animationSettings(), &AnimationSettings::timeChangeComplete, this, &ParticleInformationApplet::updateInformationDisplay);

	// Also update displayed information whenever scene selection changes.
	connect(&_mainWindow->datasetContainer(), &DataSetContainer::selectionChangeComplete, this, &ParticleInformationApplet::updateInformationDisplay);

	// Activate the viewport input mode which allows picking particles with the mouse.
	_mainWindow->viewportInputManager()->pushInputMode(_inputMode);

	// Update the list of variables that can be referenced in the selection expression.
	try {
		if(DataSet* dataset = _mainWindow->datasetContainer().currentSet()) {
			if(ObjectNode* node = dynamic_object_cast<ObjectNode>(dataset->selection()->firstNode())) {
				const PipelineFlowState& state = node->evaluatePipelinePreliminary(false);
				ParticleExpressionEvaluator evaluator;
				evaluator.initialize(QStringList(), state, 0);
				_expressionEdit->setWordList(evaluator.inputVariableNames());
			}
		}
	}
	catch(const Exception&) {}
}

/******************************************************************************
* Removes the UI of the utility from the rollout container.
******************************************************************************/
void ParticleInformationApplet::closeUtility(RolloutContainer* container)
{
	delete _panel;
}

/******************************************************************************
* This is called when new animation settings have been loaded.
******************************************************************************/
void ParticleInformationApplet::onAnimationSettingsReplaced(AnimationSettings* newAnimationSettings)
{
	disconnect(_timeChangeCompleteConnection);
	if(newAnimationSettings) {
		_timeChangeCompleteConnection = connect(newAnimationSettings, &AnimationSettings::timeChangeComplete, this, &ParticleInformationApplet::updateInformationDisplay);
	}
	updateInformationDisplay();
}

/******************************************************************************
* Updates the display of atom properties.
******************************************************************************/
void ParticleInformationApplet::updateInformationDisplay()
{
	DataSet* dataset = _mainWindow->datasetContainer().currentSet();
	if(!dataset) return;

	QString infoText;
	size_t nselected = 0;
	QTextStream stream(&infoText, QIODevice::WriteOnly);

	// In case a user-defined selection expression has been entered, 
	// apply it the current particle system to generate a new selection set.
	if(!_userSelectionExpression.isEmpty()) {
		_inputMode->_pickedParticles.clear();
		try {
			// Check if expression contain an assignment ('=' operator).
			// This should be considered an error, because the user is probably referring the comparison operator '=='.
			if(_userSelectionExpression.contains(QRegExp("[^=!><]=(?!=)")))
				throwException(tr("The entered expression contains the assignment operator '='. Please use the comparison operator '==' instead."));
			
			// Get the currently selected scene node and obtains its pipeline results.
			ObjectNode* node = dynamic_object_cast<ObjectNode>(dataset->selection()->firstNode());
			if(!node) throwException(tr("No scene object is currently selected."));
			const PipelineFlowState& state = node->evaluatePipelinePreliminary(false);
			TimeInterval iv;
			const AffineTransformation nodeTM = node->getWorldTransform(dataset->animationSettings()->time(), iv);
			ParticleProperty* posProperty = ParticleProperty::findInState(state, ParticleProperty::PositionProperty);
			ParticleProperty* identifierProperty = ParticleProperty::findInState(state, ParticleProperty::IdentifierProperty);

			// Generate particle selection set.
			ParticleExpressionEvaluator evaluator;
			evaluator.setMaxThreadCount(1);	// Disable multi-threading to make the selection order deterministic.
			evaluator.initialize(QStringList(_userSelectionExpression), state, dataset->animationSettings()->currentFrame());
			evaluator.evaluate([this, node, &nodeTM, posProperty, identifierProperty, &nselected](size_t particleIndex, size_t componentIndex, double value) {
				if(value) {
					ParticlePickingHelper::PickResult pickRes;
					pickRes.objNode = node;
					pickRes.particleIndex = particleIndex;
					pickRes.localPos = posProperty ? posProperty->getPoint3(particleIndex) : Point3::Origin();
					pickRes.worldPos = nodeTM * pickRes.localPos;
					pickRes.particleId = identifierProperty ? identifierProperty->getInt64(particleIndex) : -1;
					++nselected;
					if(_inputMode->_pickedParticles.size() < _maxSelectionSize)
						_inputMode->_pickedParticles.push_back(pickRes);
				}
			});
		}
		catch(const Exception& ex) {
			stream << QStringLiteral("<p><b>Evaluation error: ") << ex.messages().join("<br>") << QStringLiteral("</b></p>");
		}
		// Update the displayed particle markers to reflect the new selection set.
		dataset->viewportConfig()->updateViewports();
	}
	
	QString autoExpressionText;
	for(auto pickedParticle = _inputMode->_pickedParticles.begin(); pickedParticle != _inputMode->_pickedParticles.end(); ) {
		OVITO_ASSERT(pickedParticle->objNode);

		// Check if the scene node to which the selected particle belongs still exists.
		if(pickedParticle->objNode->isInScene()) {

			const PipelineFlowState& flowState = pickedParticle->objNode->evaluatePipelinePreliminary(false);

			// If selection is based on particle ID, update the stored particle index in case order has changed.
			if(pickedParticle->particleId >= 0) {
				for(DataObject* dataObj : flowState.objects()) {
					ParticleProperty* property = dynamic_object_cast<ParticleProperty>(dataObj);
					if(property && property->type() == ParticleProperty::IdentifierProperty) {
						auto begin = property->constDataInt64();
						auto end = begin + property->size();
						auto iter = std::find(begin, end, pickedParticle->particleId);
						if(iter != end) {
							pickedParticle->particleIndex = (iter - begin);
						}
						else {
							pickedParticle->particleIndex = std::numeric_limits<size_t>::max();
						}
					}
				}
			}

			// Generate an automatic selection expression.
			if(_userSelectionExpression.isEmpty()) {
				if(!autoExpressionText.isEmpty()) autoExpressionText += QStringLiteral(" ||\n");
				if(pickedParticle->particleId >= 0)
					autoExpressionText += QStringLiteral("ParticleIdentifier==%1").arg(pickedParticle->particleId);
				else
					autoExpressionText += QStringLiteral("ParticleIndex==%1").arg(pickedParticle->particleIndex);
			}

			ParticleProperty* posProperty = ParticleProperty::findInState(flowState, ParticleProperty::PositionProperty);
			if(posProperty && posProperty->size() > pickedParticle->particleIndex) {

				stream << QStringLiteral("<b>") << tr("Particle index") << QStringLiteral(" ") << pickedParticle->particleIndex << QStringLiteral(":</b>");
				stream << QStringLiteral("<table border=\"0\">");

				for(DataObject* dataObj : flowState.objects()) {
					ParticleProperty* property = dynamic_object_cast<ParticleProperty>(dataObj);
					if(!property || property->size() <= pickedParticle->particleIndex) continue;

					// Update saved particle position in case it has changed.
					if(property->type() == ParticleProperty::PositionProperty)
						pickedParticle->localPos = property->getPoint3(pickedParticle->particleIndex);

					if(property->dataType() != PropertyStorage::Int && property->dataType() != PropertyStorage::Int64 && property->dataType() != PropertyStorage::Float) continue;
					for(size_t component = 0; component < property->componentCount(); component++) {
						QString propertyName = property->name();
						if(property->componentNames().empty() == false) {
							propertyName.append(".");
							propertyName.append(property->componentNames()[component]);
						}
						QString valueString;
						if(property->dataType() == PropertyStorage::Int) {
							valueString = QString::number(property->getIntComponent(pickedParticle->particleIndex, component));
							if(property->elementTypes().empty() == false) {
								ElementType* ptype = property->elementType(property->getIntComponent(pickedParticle->particleIndex, component));
								if(ptype) {
									valueString.append(" (" + ptype->name() + ")");
								}
							}
						}
						else if(property->dataType() == PropertyStorage::Int64)
							valueString = QString::number(property->getInt64Component(pickedParticle->particleIndex, component));
						else if(property->dataType() == PropertyStorage::Float)
							valueString = QString::number(property->getFloatComponent(pickedParticle->particleIndex, component));

						stream << QStringLiteral("<tr><td>") << propertyName << QStringLiteral(":</td><td>") << valueString << QStringLiteral("</td></tr>");
					}
				}
				stream << QStringLiteral("</table><hr>");
				++pickedParticle;
				continue;
			}
		}
		// Remove non-existent particle from the list.
		pickedParticle = _inputMode->_pickedParticles.erase(pickedParticle);
	}

	if(_userSelectionExpression.isEmpty())
		_expressionEdit->setPlainText(autoExpressionText);

	if(_inputMode->_pickedParticles.empty()) {
		stream << tr("No particles selected.");
		_displayHeader->setText(tr("Particle information:"));
	}
	else if(_inputMode->_pickedParticles.size() >= nselected) {
		_displayHeader->setText(tr("Particle information (%1):").arg(_inputMode->_pickedParticles.size()));
	}
	else {
		_displayHeader->setText(tr("Particle information (%1 out of %2):").arg(_inputMode->_pickedParticles.size()).arg(nselected));
	}

	if(_inputMode->_pickedParticles.size() >= 2) {
		stream << QStringLiteral("<b>") << tr("Pair vectors:") << QStringLiteral("</b>");
		stream << QStringLiteral("<table border=\"0\">");
		for(size_t i = 0; i < _inputMode->_pickedParticles.size(); i++) {
			const auto& p1 = _inputMode->_pickedParticles[i];
			for(size_t j = i + 1; j < _inputMode->_pickedParticles.size(); j++) {
				const auto& p2 = _inputMode->_pickedParticles[j];
				Vector3 delta = p2.localPos - p1.localPos;
				stream << QStringLiteral("<tr><td width=\"50%%\">(") <<
						p1.particleIndex << QStringLiteral(" - ") << p2.particleIndex <<
						QStringLiteral("):</td><td width=\"50%%\">Distance = ") << delta.length() << QStringLiteral("</td></tr>");
				stream << QStringLiteral("<tr><td colspan=\"2\">&nbsp;&nbsp;&nbsp;&nbsp;[") <<
						delta.x() << QStringLiteral(", ") << delta.y() << QStringLiteral(", ") << delta.z() << QStringLiteral("]</td></tr>");
			}
		}
		stream << QStringLiteral("</table><hr>");
	}
	if(_inputMode->_pickedParticles.size() >= 3) {
		stream << QStringLiteral("<b>") << tr("Angles:") << QStringLiteral("</b>");
		stream << QStringLiteral("<table border=\"0\">");
		for(size_t i = 0; i < _inputMode->_pickedParticles.size(); i++) {
			const auto& p1 = _inputMode->_pickedParticles[i];
			for(size_t j = 0; j < _inputMode->_pickedParticles.size(); j++) {
				if(j == i) continue;
				const auto& p2 = _inputMode->_pickedParticles[j];
				for(size_t k = j + 1; k < _inputMode->_pickedParticles.size(); k++) {
					if(k == i) continue;
					const auto& p3 = _inputMode->_pickedParticles[k];
					Vector3 v1 = p2.localPos - p1.localPos;
					Vector3 v2 = p3.localPos - p1.localPos;
					v1.normalizeSafely();
					v2.normalizeSafely();
					FloatType angle = acos(v1.dot(v2));
					stream << QStringLiteral("<tr><td>(") <<
							p2.particleIndex << QStringLiteral(" - ") << p1.particleIndex << QStringLiteral(" - ") << p3.particleIndex <<
							QStringLiteral("):</td><td>") << qRadiansToDegrees(angle) << QStringLiteral("</td></tr>");
				}
			}
		}
		stream << QStringLiteral("</table><hr>");
	}
	_infoDisplay->setText(infoText);
}

/******************************************************************************
* Handles the mouse up events for a Viewport.
******************************************************************************/
void ParticleInformationInputMode::mouseReleaseEvent(ViewportWindow* vpwin, QMouseEvent* event)
{
	if(event->button() == Qt::LeftButton) {
		PickResult pickResult;
		pickParticle(vpwin, event->pos(), pickResult);
		if(!event->modifiers().testFlag(Qt::ControlModifier))
			_pickedParticles.clear();
		if(pickResult.objNode) {
			// Don't select the same particle twice. Instead, toggle selection.
			bool alreadySelected = false;
			for(auto p = _pickedParticles.begin(); p != _pickedParticles.end(); ++p) {
				if(p->objNode == pickResult.objNode && p->particleIndex == pickResult.particleIndex) {
					alreadySelected = true;
					_pickedParticles.erase(p);
					break;
				}
			}
			if(!alreadySelected)
				_pickedParticles.push_back(pickResult);
		}
		_applet->resetUserExpression();
		_applet->updateInformationDisplay();
		vpwin->viewport()->dataset()->viewportConfig()->updateViewports();
	}
	ViewportInputMode::mouseReleaseEvent(vpwin, event);
}

/******************************************************************************
* Handles the mouse move event for the given viewport.
******************************************************************************/
void ParticleInformationInputMode::mouseMoveEvent(ViewportWindow* vpwin, QMouseEvent* event)
{
	// Change mouse cursor while hovering over a particle.
	PickResult pickResult;
	if(pickParticle(vpwin, event->pos(), pickResult))
		setCursor(SelectionMode::selectionCursor());
	else
		setCursor(QCursor());

	ViewportInputMode::mouseMoveEvent(vpwin, event);
}

/******************************************************************************
* Lets the input mode render its overlay content in a viewport.
******************************************************************************/
void ParticleInformationInputMode::renderOverlay3D(Viewport* vp, ViewportSceneRenderer* renderer)
{
	ViewportInputMode::renderOverlay3D(vp, renderer);
	for(const auto& pickedParticle : _pickedParticles)
		renderSelectionMarker(vp, renderer, pickedParticle);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
