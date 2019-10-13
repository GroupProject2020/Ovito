////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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


#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/mesh/surface/SurfaceMeshData.h>
#include <ovito/mesh/surface/SurfaceMeshVis.h>
#include <ovito/stdobj/simcell/SimulationCell.h>
#include <ovito/stdobj/properties/PropertyStorage.h>
#include <ovito/core/dataset/pipeline/AsynchronousModifier.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/*
 * Constructs a surface mesh from a particle system.
 */
class OVITO_PARTICLES_EXPORT ConstructSurfaceModifier : public AsynchronousModifier
{
	/// Give this modifier class its own metaclass.
	class OOMetaClass : public AsynchronousModifier::OOMetaClass
	{
	public:

		/// Inherit constructor from base metaclass.
		using AsynchronousModifier::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const DataCollection& input) const override;
	};

	Q_OBJECT
	OVITO_CLASS_META(ConstructSurfaceModifier, OOMetaClass)

	Q_CLASSINFO("DisplayName", "Construct surface mesh");
	Q_CLASSINFO("ModifierCategory", "Visualization");

public:

	/// The different methods supported by this modifier for constructing the surface.
    enum SurfaceMethod {
		AlphaShape,
		GaussianDensity,
	};
    Q_ENUMS(SurfaceMethod);

public:

	/// Constructor.
	Q_INVOKABLE ConstructSurfaceModifier(DataSet* dataset);

	/// Decides whether a preliminary viewport update is performed after the modifier has been
	/// evaluated but before the entire pipeline evaluation is complete.
	/// We suppress such preliminary updates for this modifier, because it produces a surface mesh,
	/// which requires further asynchronous processing before a viewport update makes sense.
	virtual bool performPreliminaryUpdateAfterEvaluation() override { return false; }

protected:

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

private:

	/// Abstract base class for computation engines that build the surface mesh.
	class ConstructSurfaceEngineBase : public ComputeEngine
	{
	public:

		/// Constructor.
		ConstructSurfaceEngineBase(ConstPropertyPtr positions, ConstPropertyPtr selection, const SimulationCell& simCell) :
			_positions(positions),
			_selection(std::move(selection)),
			_mesh(simCell) {}

		/// This method is called by the system after the computation was successfully completed.
		virtual void cleanup() override {
			_positions.reset();
			_selection.reset();
			ComputeEngine::cleanup();
		}

		/// Returns the generated surface mesh.
		const SurfaceMeshData& mesh() const { return _mesh; }

		/// Returns a mutable reference to the surface mesh structure.
		SurfaceMeshData& mesh() { return _mesh; }

		/// Returns the computed surface area.
		FloatType surfaceArea() const { return (FloatType)_surfaceArea; }

		/// Sums a contribution to the total surface area.
		void addSurfaceArea(FloatType a) { _surfaceArea += a; }

		/// Returns the input particle positions.
		const ConstPropertyPtr& positions() const { return _positions; }

		/// Returns the input particle selection.
		const ConstPropertyPtr& selection() const { return _selection; }

	private:

		/// The input particle coordinates.
		ConstPropertyPtr _positions;

		/// The input particle selection flags.
		ConstPropertyPtr _selection;

		/// The generated surface mesh.
		SurfaceMeshData _mesh;

		/// The computed surface area.
		double _surfaceArea = 0;
	};

	/// Compute engine building the surface mesh using the alpha shape method.
	class AlphaShapeEngine : public ConstructSurfaceEngineBase
	{
	public:

		/// Constructor.
		AlphaShapeEngine(ConstPropertyPtr positions, ConstPropertyPtr selection, const SimulationCell& simCell, FloatType probeSphereRadius, int smoothingLevel, bool selectSurfaceParticles) :
			ConstructSurfaceEngineBase(std::move(positions), std::move(selection), simCell),
			_probeSphereRadius(probeSphereRadius),
			_smoothingLevel(smoothingLevel),
			_totalVolume(std::abs(simCell.matrix().determinant())),
			_surfaceParticleSelection(selectSurfaceParticles ? ParticlesObject::OOClass().createStandardStorage(this->positions()->size(), ParticlesObject::SelectionProperty, true) : nullptr) {}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;

		/// Injects the computed results into the data pipeline.
		virtual void emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

		/// Returns the computed solid volume.
		FloatType solidVolume() const { return (FloatType)_solidVolume; }

		/// Sums a contribution to the total solid volume.
		void addSolidVolume(FloatType v) { _solidVolume += v; }

		/// Returns the computed total volume.
		FloatType totalVolume() const { return (FloatType)_totalVolume; }

		/// Returns the selection set containing the particles at the constructed surfaces.
		const PropertyPtr& surfaceParticleSelection() const { return _surfaceParticleSelection; }

		/// Returns the evalue of the probe sphere radius parameter.
		FloatType probeSphereRadius() const { return _probeSphereRadius; }

	private:

		/// The radius of the virtual probe sphere (alpha-shape parameter).
		const FloatType _probeSphereRadius;

		/// The number of iterations of the smoothing algorithm to apply to the surface mesh.
		const int _smoothingLevel;

		/// The computed solid volume.
		double _solidVolume = 0;

		/// The computed total volume.
		double _totalVolume = 0;

		/// The selection set containing the particles right on the constructed surfaces.
		PropertyPtr _surfaceParticleSelection;
	};

	/// Compute engine building the surface mesh using the Gaussian density method.
	class GaussianDensityEngine : public ConstructSurfaceEngineBase
	{
	public:

		/// Constructor.
		GaussianDensityEngine(ConstPropertyPtr positions, ConstPropertyPtr selection, const SimulationCell& simCell,
				FloatType radiusFactor, FloatType isoLevel, int gridResolution, std::vector<FloatType> radii) :
			ConstructSurfaceEngineBase(std::move(positions), std::move(selection), simCell),
			_radiusFactor(radiusFactor),
			_isoLevel(isoLevel),
			_gridResolution(gridResolution),
			_particleRadii(std::move(radii)) {}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;

		/// Injects the computed results into the data pipeline.
		virtual void emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

	private:

		/// Scaling factor applied to atomic radii.
		const FloatType _radiusFactor;

		/// The threshold for constructing the isosurface of the density field.
		const FloatType _isoLevel;

		/// The number of voxels in the density grid.
		const int _gridResolution;

		/// The atomic input radii.
		std::vector<FloatType> _particleRadii;
	};

	/// Controls the radius of the probe sphere.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, probeSphereRadius, setProbeSphereRadius, PROPERTY_FIELD_MEMORIZE);

	/// Controls the amount of smoothing.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(int, smoothingLevel, setSmoothingLevel, PROPERTY_FIELD_MEMORIZE);

	/// Controls whether only selected particles should be taken into account.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, onlySelectedParticles, setOnlySelectedParticles);

	/// Controls whether the modifier should select surface particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, selectSurfaceParticles, setSelectSurfaceParticles);

	/// The vis element for rendering the surface.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(SurfaceMeshVis, surfaceMeshVis, setSurfaceMeshVis, PROPERTY_FIELD_DONT_PROPAGATE_MESSAGES | PROPERTY_FIELD_MEMORIZE | PROPERTY_FIELD_OPEN_SUBEDITOR);

	/// Surface construction method to use.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(SurfaceMethod, method, setMethod, PROPERTY_FIELD_MEMORIZE);

	/// Controls the number of grid cells in each spatial direction (density field method).
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(int, gridResolution, setGridResolution, PROPERTY_FIELD_MEMORIZE);

	/// The scaling factor applied to atomic radii (density field method).
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, radiusFactor, setRadiusFactor, PROPERTY_FIELD_MEMORIZE);

	/// The threshold value for constructing the isosurface of the density field.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, isoValue, setIsoValue, PROPERTY_FIELD_MEMORIZE);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::Particles::ConstructSurfaceModifier::SurfaceMethod);
Q_DECLARE_TYPEINFO(Ovito::Particles::ConstructSurfaceModifier::SurfaceMethod, Q_PRIMITIVE_TYPE);
