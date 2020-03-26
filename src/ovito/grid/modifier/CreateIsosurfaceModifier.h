////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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


#include <ovito/grid/Grid.h>
#include <ovito/grid/objects/VoxelGrid.h>
#include <ovito/mesh/surface/SurfaceMeshData.h>
#include <ovito/mesh/surface/SurfaceMeshVis.h>
#include <ovito/stdobj/table/DataTable.h>
#include <ovito/core/dataset/pipeline/AsynchronousModifier.h>

namespace Ovito { namespace Grid {

/*
 * Constructs an isosurface from a data grid.
 */
class OVITO_GRID_EXPORT CreateIsosurfaceModifier : public AsynchronousModifier
{
	/// Give this modifier class its own metaclass.
	class CreateIsosurfaceModifierClass : public ModifierClass
	{
	public:

		/// Inherit constructor from base class.
		using ModifierClass::ModifierClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const DataCollection& input) const override;
	};

	Q_OBJECT
	OVITO_CLASS_META(CreateIsosurfaceModifier, CreateIsosurfaceModifierClass)

	Q_CLASSINFO("DisplayName", "Create isosurface");
	Q_CLASSINFO("ModifierCategory", "Visualization");

public:

	/// Constructor.
	Q_INVOKABLE CreateIsosurfaceModifier(DataSet* dataset);

	/// This method is called by the system after the modifier has been inserted into a data pipeline.
	virtual void initializeModifier(ModifierApplication* modApp) override;

	/// Determines the time interval over which a computed pipeline state will remain valid.
	virtual TimeInterval validityInterval(const PipelineEvaluationRequest& request, const ModifierApplication* modApp) const override;

	/// Decides whether a preliminary viewport update is performed after the modifier has been
	/// evaluated but before the entire pipeline evaluation is complete.
	/// We suppress such preliminary updates for this modifier, because it produces a surface mesh,
	/// which requires further asynchronous processing before a viewport update makes sense.
	virtual bool performPreliminaryUpdateAfterEvaluation() override { return false; }

	/// Returns the level at which to create the isosurface.
	FloatType isolevel() const { return isolevelController() ? isolevelController()->currentFloatValue() : 0; }

	/// Sets the level at which to create the isosurface.
	void setIsolevel(FloatType value) { if(isolevelController()) isolevelController()->setCurrentFloatValue(value); }

protected:

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input) override;

private:

	/// Computation engine that builds the isosurface mesh.
	class ComputeIsosurfaceEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		ComputeIsosurfaceEngine(const TimeInterval& validityInterval, const VoxelGrid::GridDimensions& gridShape, ConstPropertyPtr property, int vectorComponent, const SimulationCell& simCell, FloatType isolevel) :
			ComputeEngine(validityInterval),
			_gridShape(gridShape),
			_property(std::move(property)),
			_vectorComponent(std::max(vectorComponent, 0)),
			_mesh(simCell),
			_isolevel(isolevel) {}
			
		/// Computes the modifier's results.
		virtual void perform() override;

		/// Injects the computed results into the data pipeline.
		virtual void emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

		/// Returns the minimum field value that was encountered.
		FloatType minValue() const { return _minValue; }

		/// Returns the maximum field value that was encountered.
		FloatType maxValue() const { return _maxValue; }

		/// Returns the generated mesh.
		const SurfaceMeshData& mesh() { return _mesh; }

		/// Returns the simulation cell geometry.
		const SimulationCell& cell() { return _mesh.cell(); }

		/// Adjust the min/max values to include the given value.
		void updateMinMax(FloatType val) {
			if(val < _minValue) _minValue = val;
			if(val > _maxValue) _maxValue = val;
		}

		/// Returns the input voxel property.
		const ConstPropertyPtr& property() const { return _property; }

		/// Returns the computed histogram of the input field values.
		const PropertyPtr& histogram() const { return _histogram; }

	private:

		const VoxelGrid::GridDimensions _gridShape;
		const FloatType _isolevel;
		const int _vectorComponent;
		ConstPropertyPtr _property;

		/// The surface mesh produced by the modifier.
		SurfaceMeshData _mesh;

		/// The minimum field value that was encountered.
		FloatType _minValue =  FLOATTYPE_MAX;

		/// The maximum field value that was encountered.
		FloatType _maxValue = -FLOATTYPE_MAX;

		/// The computed histogram of the input field values.
		PropertyPtr _histogram = std::make_shared<PropertyStorage>(64, PropertyStorage::Int64, 1, 0, tr("Count"), true, DataTable::YProperty);
	};

	/// Specifies the voxel grid this modifier should operate on.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(PropertyContainerReference, subject, setSubject);

	/// The voxel property that serves as input.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(VoxelPropertyReference, sourceProperty, setSourceProperty);

	/// This controller stores the level at which to create the isosurface.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(Controller, isolevelController, setIsolevelController, PROPERTY_FIELD_MEMORIZE);

	/// The vis element for rendering the surface.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(SurfaceMeshVis, surfaceMeshVis, setSurfaceMeshVis, PROPERTY_FIELD_DONT_PROPAGATE_MESSAGES | PROPERTY_FIELD_MEMORIZE | PROPERTY_FIELD_OPEN_SUBEDITOR);
};

}	// End of namespace
}	// End of namespace
