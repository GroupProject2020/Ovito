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
#include <plugins/particles/gui/util/BondPickingHelper.h>
#include <plugins/particles/objects/BondsObject.h>
#include <plugins/particles/util/ParticleExpressionEvaluator.h>
#include <plugins/stdobj/gui/properties/PropertyInspectionApplet.h>
#include <gui/viewport/input/ViewportInputMode.h>
#include <gui/viewport/input/ViewportInputManager.h>
#include <gui/viewport/input/ViewportGizmo.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Internal)


/**
 * \brief Data inspector page for bonds.
 */
class BondInspectionApplet : public PropertyInspectionApplet
{
	Q_OBJECT
	OVITO_CLASS(BondInspectionApplet)
	Q_CLASSINFO("DisplayName", "Bonds");

public:

	/// Constructor.
	Q_INVOKABLE BondInspectionApplet() : PropertyInspectionApplet(BondsObject::OOClass()) {}

	/// Returns the key value for this applet that is used for ordering the applet tabs.
	virtual int orderingKey() const override { return 10; }

	/// Lets the applet create the UI widget that is to be placed into the data inspector panel. 
	virtual QWidget* createWidget(MainWindow* mainWindow) override;

	/// Lets the applet update the contents displayed in the inspector.
	virtual void updateDisplay(const PipelineFlowState& state, PipelineSceneNode* sceneNode) override;

	/// This is called when the applet is no longer visible.
	virtual void deactivate(MainWindow* mainWindow) override;

protected:

	/// Creates the evaluator object for filter expressions.
	virtual std::unique_ptr<PropertyExpressionEvaluator> createExpressionEvaluator() override {
		return std::make_unique<BondExpressionEvaluator>();
	}

	/// Determines whether the given property represents a color.
	virtual bool isColorProperty(PropertyObject* property) const override {
		return property->type() == BondsObject::ColorProperty;
	}

private:

	/// Viewport input mode that lets the user pick bonds.
	class PickingMode : public ViewportInputMode, BondPickingHelper
	{
	public:

		/// Constructor.
		PickingMode(BondInspectionApplet* applet) : ViewportInputMode(applet), _applet(applet) {}

		/// Handles the mouse up events for a viewport.
		virtual void mouseReleaseEvent(ViewportWindow* vpwin, QMouseEvent* event) override;

		/// Handles the mouse move event for the given viewport.
		virtual void mouseMoveEvent(ViewportWindow* vpwin, QMouseEvent* event) override;

		/// Clears the list of picked bonds.
		void resetSelection() {
			if(!_pickedElements.empty()) {
				_pickedElements.clear();
				requestViewportUpdate();
			}
		}

	private:

		/// The owner object.
		BondInspectionApplet* _applet;

		/// The list of picked bonds.
		std::vector<PickResult> _pickedElements;
	};

private:

	/// The viewport input for picking bonds.
	PickingMode* _pickingMode;	
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
