///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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


#include <plugins/stdmod/StdMod.h>
#include <core/dataset/pipeline/DelegatingModifier.h>
#include <core/dataset/animation/controller/Controller.h>
#include <core/dataset/animation/AnimationSettings.h>

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
	virtual PipelineStatus apply(Modifier* modifier, const PipelineFlowState& input, PipelineFlowState& output, TimePoint time, ModifierApplication* modApp) override;

	/// \brief Returns the class of properties that can serve as input for the modifier.
	virtual const PropertyClass& propertyClass() const = 0;
	
protected:

	/// Abstract class constructor.
	using ModifierDelegate::ModifierDelegate;
	
	/// \brief Creates the property object that will receive the assigned color.
	virtual PropertyObject* createOutputColorProperty(TimePoint time, InputHelper& ih, OutputHelper& oh, bool initializeWithExistingColors) = 0;
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
	
	/// Asks the modifier for its validity interval at the given time.
	virtual TimeInterval modifierValidity(TimePoint time) override;

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
