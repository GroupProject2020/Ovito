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


#include <plugins/grid/Grid.h>
#include <plugins/stdobj/properties/PropertyReference.h>
#include <plugins/stdobj/series/DataSeriesObject.h>
#include <plugins/stdobj/simcell/SimulationCell.h>
#include <core/dataset/pipeline/AsynchronousDelegatingModifier.h>
#include <core/dataset/pipeline/AsynchronousModifierApplication.h>

namespace Ovito { namespace Grid {

/**
 * \brief Base class for modifier delegates used by the SpatialBinningModifier class.
 */
class OVITO_GRID_EXPORT SpatialBinningModifierDelegate : public AsynchronousModifierDelegate
{
	Q_OBJECT
	OVITO_CLASS(SpatialBinningModifierDelegate)

protected:

	/// Constructor.
	using AsynchronousModifierDelegate::AsynchronousModifierDelegate;

	/// Asynchronous compute engine that does the actual work in a separate thread.
	class SpatialBinningEngine : public AsynchronousModifier::ComputeEngine
	{
	public:

		/// Constructor.
		SpatialBinningEngine(
				const TimeInterval& validityInterval, 
				const SimulationCell& cell,
				ConstPropertyPtr sourceProperty, 
				size_t sourceComponent, 
				ConstPropertyPtr selectionProperty,
				PropertyPtr binData,
				const Vector3I& binCount,
				const Vector3I& binDir,
				int reductionOperation,
				bool computeFirstDerivative) :
			AsynchronousModifier::ComputeEngine(validityInterval), 
			_cell(cell),
			_sourceProperty(std::move(sourceProperty)),
			_sourceComponent(sourceComponent), 
			_selectionProperty(std::move(selectionProperty)),
			_binData(std::move(binData)),
			_binCount(binCount),
			_binDir(binDir),
			_reductionOperation(reductionOperation),
			_computeFirstDerivative(computeFirstDerivative) {}
				
		/// This method is called by the system after the computation was successfully completed.
		virtual void cleanup() override {
			_sourceProperty.reset();
			_selectionProperty.reset();
			ComputeEngine::cleanup();
		}

		/// Compute first derivative using finite differences.
		void computeGradient();

		/// Returns the property storage that contains the input values.
		const ConstPropertyPtr& sourceProperty() const { return _sourceProperty; }

		/// Returns the vector component of the source property.
		size_t sourceComponent() const { return _sourceComponent; }

		/// Returns the property storage that contains the input element selection.
		const ConstPropertyPtr& selectionProperty() const { return _selectionProperty; }

		/// Returns the property storage for the per-bin data.
		const PropertyPtr& binData() const { return _binData; }

		/// Returns the simulation cell information.
		const SimulationCell& cell() const { return _cell; }

		/// Returns the number of bins in the given binning direction.
		int binCount(size_t dim) const { return _binCount[dim]; }

		/// Returns a spatial binning direction.
		int binDir(size_t dim) const { return _binDir[dim]; }

		/// Returns the type of reduction operation to perform.
		int reductionOperation() const { return _reductionOperation; }

		/// Injects the computed results into the data pipeline.
		virtual PipelineFlowState emitResults(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;
		
	private:

		const SimulationCell _cell;
		ConstPropertyPtr _sourceProperty;
		size_t _sourceComponent;
		ConstPropertyPtr _selectionProperty;
		PropertyPtr _binData;
		const Vector3I _binCount;
		const Vector3I _binDir;
		int _reductionOperation;
		bool _computeFirstDerivative;
	};

public:

	/// \brief Returns the class of data elements this delegate operates on.
	virtual const PropertyClass& propertyClass() const = 0;

	/// Creates a computation engine that will perform the actual binning of elements.
	virtual std::shared_ptr<SpatialBinningEngine> createEngine(
				TimePoint time, 
				const PipelineFlowState& input,
				const SimulationCell& cell,
				ConstPropertyPtr sourceProperty,
				size_t sourceComponent, 
				ConstPropertyPtr selectionProperty,
				PropertyPtr binData,
				const Vector3I& binCount,
				const Vector3I& binDir,
				int reductionOperation,
				bool computeFirstDerivative) = 0;
};

/**
 * \brief This modifier places elements into equal-sized spatial bins and computes a reduction, e.g. the average 
 *        of a selected input property within each bin cell.
 */
class OVITO_GRID_EXPORT SpatialBinningModifier : public AsynchronousDelegatingModifier
{
	/// Give this modifier class its own metaclass.
	class BinningModifierClass : public AsynchronousDelegatingModifier::OOMetaClass  
	{
	public:

		/// Inherit constructor from base metaclass.
		using AsynchronousDelegatingModifier::OOMetaClass::OOMetaClass;

		/// Return the metaclass of delegates for this modifier type. 
		virtual const AsynchronousModifierDelegate::OOMetaClass& delegateMetaclass() const override { return SpatialBinningModifierDelegate::OOClass(); }
	};

	Q_OBJECT
	OVITO_CLASS_META(SpatialBinningModifier, BinningModifierClass)
	Q_CLASSINFO("DisplayName", "Spatial binning");
	Q_CLASSINFO("ModifierCategory", "Analysis");

public:

    enum ReductionOperationType { 
		RED_MEAN, 
		RED_SUM, 
		RED_SUM_VOL, 
		RED_MIN, 
		RED_MAX 
	};
    Q_ENUMS(ReductionOperationType);

    enum BinDirectionType { 
		CELL_VECTOR_1 = 	0b111100, 
		CELL_VECTOR_2 = 	0b111101, 
		CELL_VECTOR_3 = 	0b111110,
        CELL_VECTORS_1_2 = 	0b110100,
		CELL_VECTORS_1_3 = 	0b111000,
		CELL_VECTORS_2_3 = 	0b111001,
		CELL_VECTORS_1_2_3 =0b100100 
	};
    Q_ENUMS(BinDirectionType);

	/// Constructor.
	Q_INVOKABLE SpatialBinningModifier(DataSet* dataset);

	/// \brief Returns the current delegate of this SpatialBinningModifier.
	SpatialBinningModifierDelegate* delegate() const { return static_object_cast<SpatialBinningModifierDelegate>(AsynchronousDelegatingModifier::delegate()); }

	/// This method is called by the system after the modifier has been inserted into a data pipeline.
	virtual void initializeModifier(ModifierApplication* modApp) override;

	/// Set start and end value of the plotting property axis.
	void setPropertyAxisRange(FloatType start, FloatType end) { 
		setPropertyAxisRangeStart(start); 
		setPropertyAxisRangeEnd(end); 
	}

    /// Returns true if binning is done in a single direction only.
    bool is1D() const {
        return bin1D(binDirection());
    }

    /// Returns true if binning is done in two spatial directions.
    bool is2D() const {
        return bin2D(binDirection());
    }	

    /// Returns true if binning is done in two spatial directions.
    bool is3D() const {
        return bin3D(binDirection());
    }	

    /// Returns true if binning is performed in a single direction only.
    static bool bin1D(BinDirectionType d) {
        return d == CELL_VECTOR_1 || d == CELL_VECTOR_2 || d == CELL_VECTOR_3;
    }

    /// Returns true if binning is performed in a two spatial directions.
    static bool bin2D(BinDirectionType d) {
        return d == CELL_VECTORS_1_2 || d == CELL_VECTORS_1_3 || d == CELL_VECTORS_2_3;
    }

    /// Returns true if binning is performed in a two spatial directions.
    static bool bin3D(BinDirectionType d) {
        return d == CELL_VECTORS_1_2_3;
    }

    /// Return the cell vector index to be mapped to the X-axis of the output grid.
    static int binDirectionX(BinDirectionType d) {
        return d & 3;
    }

    /// Return the cell vector index to be mapped to the Y-axis of the output grid.
    static int binDirectionY(BinDirectionType d) {
        return (d >> 2) & 3;
    }

    /// Return the cell vector index to be mapped to the Z-axis of the output grid.
    static int binDirectionZ(BinDirectionType d) {
        return (d >> 4) & 3;
    }

protected:

	/// \brief Is called when the value of a reference field of this RefMaker changes.
	virtual void referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget) override;

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

private:

	/// The property that serves as data source to be averaged.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(PropertyReference, sourceProperty, setSourceProperty);

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

	/// Controls the number of spatial bins.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(int, numberOfBinsZ, setNumberOfBinsZ, PROPERTY_FIELD_MEMORIZE);

	/// Controls the whether the plotting range along the y-axis should be fixed.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, fixPropertyAxisRange, setFixPropertyAxisRange);

	/// Controls the start value of the plotting y-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, propertyAxisRangeStart, setPropertyAxisRangeStart, PROPERTY_FIELD_MEMORIZE);

	/// Controls the end value of the plotting y-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, propertyAxisRangeEnd, setPropertyAxisRangeEnd, PROPERTY_FIELD_MEMORIZE);

	/// Controls whether the modifier should take into account only selected elements.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, onlySelectedElements, setOnlySelectedElements);
};

/**
 * \brief The type of ModifierApplication created for a SpatialBinningModifier 
 *        when it is inserted into in a data pipeline.
 */
class OVITO_GRID_EXPORT BinningModifierApplication : public AsynchronousModifierApplication
{
	OVITO_CLASS(BinningModifierApplication)
	Q_OBJECT
	
public:

	/// Constructor.
	Q_INVOKABLE BinningModifierApplication(DataSet* dataset) : AsynchronousModifierApplication(dataset) {}
 
private:

	/// The computed 1d histogram.
	DECLARE_RUNTIME_PROPERTY_FIELD_FLAGS(OORef<DataSeriesObject>, histogram, setHistogram, PROPERTY_FIELD_NO_CHANGE_MESSAGE);
};

}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::Grid::SpatialBinningModifier::ReductionOperationType);
Q_DECLARE_METATYPE(Ovito::Grid::SpatialBinningModifier::BinDirectionType);
Q_DECLARE_TYPEINFO(Ovito::Grid::SpatialBinningModifier::ReductionOperationType, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(Ovito::Grid::SpatialBinningModifier::BinDirectionType, Q_PRIMITIVE_TYPE);
