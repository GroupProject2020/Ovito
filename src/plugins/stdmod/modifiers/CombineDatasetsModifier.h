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


#include <plugins/stdmod/StdMod.h>
#include <core/dataset/pipeline/DelegatingModifier.h>
#include <core/dataset/pipeline/PipelineObject.h>

namespace Ovito { namespace StdMod {

/**
 * \brief Base class for CombineDatasetsModifier delegates that operate on different kinds of data.
 */
class OVITO_STDMOD_EXPORT CombineDatasetsModifierDelegate : public ModifierDelegate
{
	Q_OBJECT
	OVITO_CLASS(CombineDatasetsModifierDelegate)

protected:

	/// Abstract class constructor.
	using ModifierDelegate::ModifierDelegate;
};

/**
 * \brief Merges two separate datasets into one.
 */
class OVITO_STDMOD_EXPORT CombineDatasetsModifier : public MultiDelegatingModifier
{
	/// Give this modifier class its own metaclass.
	class CombineDatasetsModifierClass : public MultiDelegatingModifier::OOMetaClass  
	{
	public:

		/// Inherit constructor from base metaclass.
		using MultiDelegatingModifier::OOMetaClass::OOMetaClass;

		/// Return the metaclass of delegates for this modifier type. 
		virtual const ModifierDelegate::OOMetaClass& delegateMetaclass() const override { return CombineDatasetsModifierDelegate::OOClass(); }
	};

	Q_OBJECT
	OVITO_CLASS_META(CombineDatasetsModifier, CombineDatasetsModifierClass)

	Q_CLASSINFO("DisplayName", "Combine datasets");
	Q_CLASSINFO("ModifierCategory", "Modification");

public:

	/// Constructor.
	Q_INVOKABLE CombineDatasetsModifier(DataSet* dataset);

	/// Modifies the input data in an immediate, preliminary way.
	virtual void evaluatePreliminary(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

	/// Modifies the input data.
	virtual Future<PipelineFlowState> evaluate(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

private:

	/// The source for particle data to be merged into the pipeline.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(PipelineObject, secondaryDataSource, setSecondaryDataSource, PROPERTY_FIELD_NO_SUB_ANIM);
};

}	// End of namespace
}	// End of namespace
