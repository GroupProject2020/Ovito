////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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


#include <ovito/stdmod/StdMod.h>
#include <ovito/stdobj/properties/PropertyContainer.h>
#include <ovito/core/dataset/pipeline/DelegatingModifier.h>
#include <ovito/core/dataset/animation/controller/Controller.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>

namespace Ovito { namespace StdMod {

/**
 * \brief Base class for AssignColorModifier delegates that operate on different kinds of data.
 */
class OVITO_STDMOD_EXPORT AssignColorModifierDelegate : public ModifierDelegate
{
	Q_OBJECT
	OVITO_CLASS(AssignColorModifierDelegate)

public:

	/// \brief Applies the modifier operation to the data in a pipeline flow state.
	virtual PipelineStatus apply(Modifier* modifier, PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs) override;

	/// Returns the type of input property container that this delegate can process.
	PropertyContainerClassPtr inputContainerClass() const {
		return static_class_cast<PropertyContainer>(&getOOMetaClass().getApplicableObjectClass());
	}

	/// \brief Returns a reference to the property container being modified by this delegate.
	PropertyContainerReference inputContainerRef() const {
		return PropertyContainerReference(inputContainerClass(), inputDataObject().dataPath(), inputDataObject().dataTitle());
	}

protected:

	/// Abstract class constructor.
	using ModifierDelegate::ModifierDelegate;

	/// \brief returns the ID of the standard property that will receive the assigned colors.
	virtual int outputColorPropertyId() const = 0;
};


/**
 * \brief This modifier assigns a uniform color to all selected elements.
 */
class OVITO_STDMOD_EXPORT AssignColorModifier : public DelegatingModifier
{
public:

	/// Give this modifier class its own metaclass.
	class AssignColorModifierClass : public DelegatingModifier::OOMetaClass
	{
	public:

		/// Inherit constructor from base class.
		using DelegatingModifier::OOMetaClass::OOMetaClass;

		/// Return the metaclass of delegates for this modifier type.
		virtual const ModifierDelegate::OOMetaClass& delegateMetaclass() const override { return AssignColorModifierDelegate::OOClass(); }
	};

	OVITO_CLASS_META(AssignColorModifier, AssignColorModifierClass)
	Q_CLASSINFO("DisplayName", "Assign color");
	Q_CLASSINFO("ModifierCategory", "Coloring");
	Q_OBJECT

public:

	/// Constructor.
	Q_INVOKABLE AssignColorModifier(DataSet* dataset);

	/// Loads the user-defined default values of this object's parameter fields from the
	/// application's settings store.
	virtual void loadUserDefaults() override;

	/// Determines the time interval over which a computed pipeline state will remain valid.
	virtual TimeInterval validityInterval(const PipelineEvaluationRequest& request, const ModifierApplication* modApp) const override;

	/// Returns the color that is assigned to the selected elements.
	Color color() const { return colorController() ? colorController()->currentColorValue() : Color(0,0,0); }

	/// Sets the color that is assigned to the selected elements.
	void setColor(const Color& color) { if(colorController()) colorController()->setCurrentColorValue(color); }

protected:

	/// This controller stores the color to be assigned.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(Controller, colorController, setColorController, PROPERTY_FIELD_MEMORIZE);

	/// Controls whether the input selection is preserved.
	/// If false, the selection is cleared by the modifier.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, keepSelection, setKeepSelection);
};

}	// End of namespace
}	// End of namespace
