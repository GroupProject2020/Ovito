///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
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


#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/mesh/surface/SurfaceMesh.h>
#include <plugins/mesh/surface/SurfaceMeshDisplay.h>
#include <plugins/stdobj/simcell/SimulationCell.h>
#include <plugins/stdobj/properties/PropertyStorage.h>
#include <core/dataset/pipeline/AsynchronousModifier.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/*
 * Constructs a surface mesh from a particle system.
 */
class OVITO_CRYSTALANALYSIS_EXPORT ConstructSurfaceModifier : public AsynchronousModifier
{
	/// Give this modifier class its own metaclass.
	class OOMetaClass : public AsynchronousModifier::OOMetaClass 
	{
	public:

		/// Inherit constructor from base metaclass.
		using AsynchronousModifier::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const PipelineFlowState& input) const override;
	};

	Q_OBJECT
	OVITO_CLASS_META(ConstructSurfaceModifier, OOMetaClass)

	Q_CLASSINFO("DisplayName", "Construct surface mesh");
	Q_CLASSINFO("ModifierCategory", "Analysis");

public:

	/// Constructor.
	Q_INVOKABLE ConstructSurfaceModifier(DataSet* dataset);

	/// Decides whether a preliminary viewport update is performed after the modifier has been 
	/// evaluated but before the entire pipeline evaluation is complete.
	/// We suppress such preliminary updates for this modifier, because it produces a surface mesh,
	/// which requires further asynchronous processing before a viewport update makes sense. 
	virtual bool performPreliminaryUpdateAfterEvaluation() override { return false; }

protected:

	/// Handles reference events sent by reference targets of this object.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;
	
private:

	/// Holds the modifier's results.
	class ConstructSurfaceResults : public ComputeEngineResults
	{
	public:

		/// Constructor.
		ConstructSurfaceResults(FloatType totalVolume) : _mesh(std::make_shared<HalfEdgeMesh<>>()), _totalVolume(totalVolume) {}

		/// Injects the computed results into the data pipeline.
		virtual PipelineFlowState apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;
		
		/// Indicates whether the entire simulation cell is part of the solid region.
		bool isCompletelySolid() const { return _isCompletelySolid; }

		/// Sets whether the entire simulation cell is part of the solid region.
		void setIsCompletelySolid(bool isSolid) { _isCompletelySolid = isSolid; }

		/// Returns the generated mesh.
		const SurfaceMeshPtr& mesh() { return _mesh; }
		
		/// Returns the computed solid volume.
		FloatType solidVolume() const { return (FloatType)_solidVolume; }

		/// Sums a contribution to the total solid volume.
		void addSolidVolume(FloatType v) { _solidVolume += v; }
		
		/// Returns the computed total volume.
		FloatType totalVolume() const { return (FloatType)_totalVolume; }

		/// Returns the computed surface area.
		FloatType surfaceArea() const { return (FloatType)_surfaceArea; }

		/// Sums a contribution to the total surface area.
		void addSurfaceArea(FloatType a) { _surfaceArea += a; }
				
	private:

		/// The surface mesh produced by the modifier.
		SurfaceMeshPtr _mesh;

		/// Indicates that the entire simulation cell is part of the solid region.
		bool _isCompletelySolid = false;

		/// The computed solid volume.
		double _solidVolume = 0;
	
		/// The computed total volume.
		double _totalVolume = 0;
	
		/// The computed surface area.
		double _surfaceArea = 0;
	};

	/// Computation engine that builds the surface mesh.
	class ConstructSurfaceEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		ConstructSurfaceEngine(const TimeInterval& validityInterval, ConstPropertyPtr positions, ConstPropertyPtr selection, const SimulationCell& simCell, FloatType radius, int smoothingLevel) :
			ComputeEngine(validityInterval), _positions(std::move(positions)), _selection(std::move(selection)), _simCell(simCell), _radius(radius), _smoothingLevel(smoothingLevel), 
			_results(std::make_shared<ConstructSurfaceResults>(std::abs(simCell.matrix().determinant()))) {}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;

		/// Returns the input particle positions.
		const ConstPropertyPtr& positions() const { return _positions; }

		/// Returns the input particle selection.
		const ConstPropertyPtr& selection() const { return _selection; }

	private:

		const FloatType _radius;
		const int _smoothingLevel;
		const ConstPropertyPtr _positions;
		const ConstPropertyPtr _selection;
		const SimulationCell _simCell;
		std::shared_ptr<ConstructSurfaceResults> _results;
	};

	/// Controls the radius of the probe sphere.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, probeSphereRadius, setProbeSphereRadius, PROPERTY_FIELD_MEMORIZE);

	/// Controls the amount of smoothing.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(int, smoothingLevel, setSmoothingLevel, PROPERTY_FIELD_MEMORIZE);

	/// Controls whether only selected particles should be taken into account.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, onlySelectedParticles, setOnlySelectedParticles);

	/// The display object for rendering the surface mesh.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(SurfaceMeshDisplay, surfaceMeshDisplay, setSurfaceMeshDisplay, PROPERTY_FIELD_ALWAYS_DEEP_COPY | PROPERTY_FIELD_MEMORIZE);
};

}	// End of namespace
}	// End of namespace
}	// End of namespace


