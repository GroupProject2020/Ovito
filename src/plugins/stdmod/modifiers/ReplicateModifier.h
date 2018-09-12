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

#pragma once


#include <plugins/stdmod/StdMod.h>
#include <core/dataset/pipeline/DelegatingModifier.h>

namespace Ovito { namespace StdMod {

/**
 * \brief Base class for ReplicateModifier delegates that operate on different kinds of data.
 */
class OVITO_STDMOD_EXPORT ReplicateModifierDelegate : public ModifierDelegate
{
	Q_OBJECT
	OVITO_CLASS(ReplicateModifierDelegate)
	
protected:

	/// Abstract class constructor.
	ReplicateModifierDelegate(DataSet* dataset) : ModifierDelegate(dataset) {}
};

/**
 * \brief This modifier duplicates data elements (e.g. particles) multiple times and shifts them by
 *        the simulation cell vectors to visualize periodic images.
 */
class OVITO_STDMOD_EXPORT ReplicateModifier : public MultiDelegatingModifier
{
public:

	/// Give this modifier class its own metaclass.
	class OOMetalass : public MultiDelegatingModifier::OOMetaClass 
	{
	public:

		/// Inherit constructor from base class.
		using MultiDelegatingModifier::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const DataCollection& input) const override;		

		/// Return the metaclass of delegates for this modifier type. 
		virtual const ModifierDelegate::OOMetaClass& delegateMetaclass() const override { return ReplicateModifierDelegate::OOClass(); }
	};

	OVITO_CLASS_META(ReplicateModifier, OOMetalass)
	Q_CLASSINFO("DisplayName", "Replicate");
	Q_CLASSINFO("ModifierCategory", "Modification");	
	Q_OBJECT

public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE ReplicateModifier(DataSet* dataset);

	/// Modifies the input data in an immediate, preliminary way.
	virtual void evaluatePreliminary(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

	/// Helper function that returns the range of replicated boxes.
	Box3I replicaRange() const;
	
private:

	/// Controls the number of periodic images generated in the X direction.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, numImagesX, setNumImagesX);
	/// Controls the number of periodic images generated in the Y direction.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, numImagesY, setNumImagesY);
	/// Controls the number of periodic images generated in the Z direction.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, numImagesZ, setNumImagesZ);

	/// Controls whether the size of the simulation box is adjusted to the extended system.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, adjustBoxSize, setAdjustBoxSize);

	/// Controls whether the modifier assigns unique identifiers to particle copies.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, uniqueIdentifiers, setUniqueIdentifiers);
};

}	// End of namespace
}	// End of namespace
