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


#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/particles/util/ParticleExpressionEvaluator.h>
#include <plugins/particles/gui/util/ParticlePickingHelper.h>
#include <plugins/stdobj/gui/properties/PropertyInspectionApplet.h>
#include <gui/viewport/input/ViewportInputMode.h>
#include <gui/viewport/input/ViewportInputManager.h>
#include <gui/viewport/input/ViewportGizmo.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * \brief Data inspector page for particles.
 */
class ParticleInspectionApplet : public PropertyInspectionApplet
{
	Q_OBJECT
	OVITO_CLASS(ParticleInspectionApplet)
	Q_CLASSINFO("DisplayName", "Particles");

public:

	/// Constructor.
	Q_INVOKABLE ParticleInspectionApplet() : PropertyInspectionApplet(ParticleProperty::OOClass()) {}

	/// Returns the key value for this applet that is used for ordering the applet tabs.
	virtual int orderingKey() const override { return 0; }

	/// Lets the applet create the UI widget that is to be placed into the data inspector panel. 
	virtual QWidget* createWidget(MainWindow* mainWindow) override;

	/// Lets the applet update the contents displayed in the inspector.
	virtual void updateDisplay(const PipelineFlowState& state, PipelineSceneNode* sceneNode) override;

	/// This is called when the applet is no longer visible.
	virtual void deactivate(MainWindow* mainWindow) override;

protected:

	/// Creates the evaluator object for filter expressions.
	virtual std::unique_ptr<PropertyExpressionEvaluator> createExpressionEvaluator() override {
		return std::make_unique<ParticleExpressionEvaluator>();
	}

	/// Determines whether the given property represents a color.
	virtual bool isColorProperty(PropertyObject* property) const override {
		return property->type() == ParticleProperty::ColorProperty 
				|| property->type() == ParticleProperty::VectorColorProperty;
	}

private Q_SLOTS:

	/// Computes the inter-particle distances for the selected particles.
	void updateDistanceTable();

private:

	/// Viewport input mode that lets the user pick particles.
	class PickingMode : public ViewportInputMode, ParticlePickingHelper, ViewportGizmo
	{
	public:

		/// Constructor.
		PickingMode(ParticleInspectionApplet* applet) : ViewportInputMode(applet), _applet(applet) {}

		/// This is called by the system after the input handler has become the active handler.
		virtual void activated(bool temporaryActivation) override {
			ViewportInputMode::activated(temporaryActivation);
			inputManager()->addViewportGizmo(this);
		}

		/// This is called by the system after the input handler is no longer the active handler.
		virtual void deactivated(bool temporary) override {
			inputManager()->removeViewportGizmo(this);
			ViewportInputMode::deactivated(temporary);
		}		

		/// Handles the mouse up events for a viewport.
		virtual void mouseReleaseEvent(ViewportWindow* vpwin, QMouseEvent* event) override;

		/// Handles the mouse move event for the given viewport.
		virtual void mouseMoveEvent(ViewportWindow* vpwin, QMouseEvent* event) override;

		/// Lets the input mode render its overlay content in a viewport.
		virtual void renderOverlay3D(Viewport* vp, ViewportSceneRenderer* renderer) override;

		/// Clears the list of picked particles.
		void resetSelection() {
			if(!_pickedElements.empty()) {
				_pickedElements.clear();
				requestViewportUpdate();
			}
		}

	private:

		/// The owner object.
		ParticleInspectionApplet* _applet;

		/// The list of picked particles.
		std::vector<PickResult> _pickedElements;
	};

private:

	/// The viewport input for picking particles.
	PickingMode* _pickingMode;

	/// UI action that controls the display of inter-particle distances.
	QAction* _measuringModeAction;

	/// The table displaying the inter-particle distances.
	QTableWidget* _distanceTable;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
