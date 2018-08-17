///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
//  Copyright (2014) Lars Pastewka
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


#include <plugins/particles/Particles.h>
#include <plugins/stdobj/properties/PropertyStorage.h>
#include <plugins/particles/objects/ParticleProperty.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/dataset/pipeline/Modifier.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief This modifier computes a spatial average (over splices) for a particle property.
 */
class OVITO_PARTICLES_EXPORT BinningModifier : public Modifier
{
	/// Give this modifier class its own metaclass.
	class BinningModifierClass : public ModifierClass 
	{
	public:

		/// Inherit constructor from base metaclass.
		using ModifierClass::ModifierClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const PipelineFlowState& input) const override;
	};


	Q_OBJECT
	OVITO_CLASS_META(BinningModifier, BinningModifierClass)
	Q_CLASSINFO("DisplayName", "Bin and reduce");
	Q_CLASSINFO("ModifierCategory", "Analysis");

public:

    enum ReductionOperationType { RED_MEAN, RED_SUM, RED_SUM_VOL, RED_MIN, RED_MAX };
    Q_ENUMS(ReductionOperationType);
    enum BinDirectionType { CELL_VECTOR_1 = 0, CELL_VECTOR_2 = 1, CELL_VECTOR_3 = 2,
                            CELL_VECTORS_1_2 = 0+(1<<2), CELL_VECTORS_1_3 = 0+(2<<2), CELL_VECTORS_2_3 = 1+(2<<2) };
    Q_ENUMS(BinDirectionType);

	/// Constructor.
	Q_INVOKABLE BinningModifier(DataSet* dataset);

	/// This method is called by the system after the modifier has been inserted into a data pipeline.
	virtual void initializeModifier(ModifierApplication* modApp) override;

	/// \brief Modifies the input data.
	virtual Future<PipelineFlowState> evaluate(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

	/// Set start and end value of the plotting property axis.
	void setPropertyAxisRange(FloatType start, FloatType end) { 
		setPropertyAxisRangeStart(start); 
		setPropertyAxisRangeEnd(end); 
	}

    /// Returns true if binning in a single direction only.
    bool is1D() {
        return bin1D(_binDirection);
    }

    /// Returns true if binning in a single direction only.
    static bool bin1D(BinDirectionType d) {
        return d == CELL_VECTOR_1 || d == CELL_VECTOR_2 || d == CELL_VECTOR_3;
    }

    /// Return the coordinate index to be mapped on the X-axis.
    static int binDirectionX(BinDirectionType d) {
        return d & 3;
    }

    /// Return the coordinate index to be mapped on the Y-axis.
    static int binDirectionY(BinDirectionType d) {
        return (d >> 2) & 3;
    }

private:

	/// The particle property that serves as data source to be averaged.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ParticlePropertyReference, sourceProperty, setSourceProperty);

	/// Type of reduction operation
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(ReductionOperationType, reductionOperation, setReductionOperation, PROPERTY_FIELD_MEMORIZE);

	/// Compute first derivative.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, firstDerivative, setFirstDerivative, PROPERTY_FIELD_MEMORIZE);

	/// Bin alignment
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(BinDirectionType, binDirection, setBinDirection, PROPERTY_FIELD_MEMORIZE);

	/// Controls the number of spatial bins.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(int, numberOfBinsX, setNumberOfBinsX, PROPERTY_FIELD_MEMORIZE);

	/// Controls the number of spatial bins.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(int, numberOfBinsY, setNumberOfBinsY, PROPERTY_FIELD_MEMORIZE);

	/// Controls the whether the plotting range along the y-axis should be fixed.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, fixPropertyAxisRange, setFixPropertyAxisRange);

	/// Controls the start value of the plotting y-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, propertyAxisRangeStart, setPropertyAxisRangeStart, PROPERTY_FIELD_MEMORIZE);

	/// Controls the end value of the plotting y-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, propertyAxisRangeEnd, setPropertyAxisRangeEnd, PROPERTY_FIELD_MEMORIZE);

	/// Controls whether the modifier should take into account only selected particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, onlySelected, setOnlySelected);
};

/**
 * \brief The type of ModifierApplication create for a BinningModifier 
 *        when it is inserted into in a data pipeline.
 */
class OVITO_PARTICLES_EXPORT BinningModifierApplication : public ModifierApplication
{
	OVITO_CLASS(BinningModifierApplication)
	Q_OBJECT
	
public:

	/// Constructor.
	Q_INVOKABLE BinningModifierApplication(DataSet* dataset) : ModifierApplication(dataset) {}

	using Interval = std::pair<FloatType,FloatType>;
 
private:

	/// The 1-dimensional bin values.
	DECLARE_RUNTIME_PROPERTY_FIELD_FLAGS(PropertyPtr, binData, setBinData, PROPERTY_FIELD_NO_CHANGE_MESSAGE);

	/// The interval along the first axis.
	DECLARE_RUNTIME_PROPERTY_FIELD_FLAGS(Interval, range1, setRange1, PROPERTY_FIELD_NO_CHANGE_MESSAGE);

	/// The interval along the second axis.
	DECLARE_RUNTIME_PROPERTY_FIELD_FLAGS(Interval, range2, setRange2, PROPERTY_FIELD_NO_CHANGE_MESSAGE);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::Particles::BinningModifier::ReductionOperationType);
Q_DECLARE_METATYPE(Ovito::Particles::BinningModifier::BinDirectionType);
Q_DECLARE_TYPEINFO(Ovito::Particles::BinningModifier::ReductionOperationType, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(Ovito::Particles::BinningModifier::BinDirectionType, Q_PRIMITIVE_TYPE);
