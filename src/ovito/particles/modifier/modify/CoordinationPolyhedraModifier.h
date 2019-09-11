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


#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/mesh/surface/SurfaceMeshData.h>
#include <ovito/mesh/surface/SurfaceMeshVis.h>
#include <ovito/core/dataset/pipeline/AsynchronousModifier.h>
#include <ovito/stdobj/simcell/SimulationCell.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

/**
 * \brief A modifier that creates coordination polyhedra around atoms.
 */
class OVITO_PARTICLES_EXPORT CoordinationPolyhedraModifier : public AsynchronousModifier
{
	/// Give this modifier class its own metaclass.
	class CoordinationPolyhedraModifierClass : public AsynchronousModifier::OOMetaClass
	{
	public:

		/// Inherit constructor from base metaclass.
		using AsynchronousModifier::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const DataCollection& input) const override;
	};

	Q_OBJECT
	OVITO_CLASS_META(CoordinationPolyhedraModifier, CoordinationPolyhedraModifierClass)

	Q_CLASSINFO("DisplayName", "Coordination polyhedra");
	Q_CLASSINFO("ModifierCategory", "Visualization");

public:

	/// Constructor.
	Q_INVOKABLE CoordinationPolyhedraModifier(DataSet* dataset);

protected:

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

private:

	/// Computation engine that builds the polyhedra.
	class ComputePolyhedraEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		ComputePolyhedraEngine(ConstPropertyPtr positions,
				ConstPropertyPtr selection, ConstPropertyPtr particleTypes, ConstPropertyPtr particleIdentifiers,
				ConstPropertyPtr bondTopology, ConstPropertyPtr bondPeriodicImages, const SimulationCell& simCell) :
			_positions(std::move(positions)),
			_selection(std::move(selection)),
			_particleTypes(std::move(particleTypes)),
			_particleIdentifiers(std::move(particleIdentifiers)),
			_bondTopology(std::move(bondTopology)),
			_bondPeriodicImages(std::move(bondPeriodicImages)),
			_mesh(simCell) {}

		/// This method is called by the system after the computation was successfully completed.
		virtual void cleanup() override {
			_positions.reset();
			_selection.reset();
			_particleTypes.reset();
			_particleIdentifiers.reset();
			_bondTopology.reset();
			_bondPeriodicImages.reset();
			ComputeEngine::cleanup();
		}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;

		/// Injects the computed results into the data pipeline.
		virtual void emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

		/// Returns the generated surface mesh.
		const SurfaceMeshData& mesh() const { return _mesh; }

		/// Returns the generated surface mesh.
		SurfaceMeshData& mesh() { return _mesh; }

		/// Returns the simulation cell geometry.
		const SimulationCell& cell() const { return mesh().cell(); }

	private:

		/// Constructs the convex hull from a set of points and adds the resulting polyhedron to the mesh.
		void constructConvexHull(std::vector<Point3>& vecs);

	private:

		ConstPropertyPtr _positions;
		ConstPropertyPtr _selection;
		ConstPropertyPtr _particleTypes;
		ConstPropertyPtr _particleIdentifiers;
		ConstPropertyPtr _bondTopology;
		ConstPropertyPtr _bondPeriodicImages;

		/// The output mesh.
		SurfaceMeshData _mesh;
	};

	/// The vis element for rendering the polyhedra.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(SurfaceMeshVis, surfaceMeshVis, setSurfaceMeshVis, PROPERTY_FIELD_DONT_PROPAGATE_MESSAGES | PROPERTY_FIELD_MEMORIZE | PROPERTY_FIELD_OPEN_SUBEDITOR);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
