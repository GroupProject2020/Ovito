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


#include <plugins/particles/Particles.h>
#include <plugins/particles/objects/BondProperty.h>
#include <plugins/mesh/surface/SurfaceMesh.h>
#include <plugins/mesh/surface/SurfaceMeshVis.h>
#include <core/dataset/pipeline/AsynchronousModifier.h>
#include <plugins/stdobj/simcell/SimulationCell.h>

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
		virtual bool isApplicableTo(const PipelineFlowState& input) const override;
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

	/// Holds the modifier's results.
	class ComputePolyhedraResults : public ComputeEngineResults
	{
	public:

		/// Constructor.
		using ComputeEngineResults::ComputeEngineResults;

		/// Injects the computed results into the data pipeline.
		virtual PipelineFlowState apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;
	
		/// Returns the generated mesh.
		const std::shared_ptr<HalfEdgeMesh<>>& mesh() const { return _mesh; }
						
	private:

		std::shared_ptr<HalfEdgeMesh<>> _mesh = std::make_shared<HalfEdgeMesh<>>();
	};

	/// Computation engine that builds the polyhedra.
	class ComputePolyhedraEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		ComputePolyhedraEngine(ConstPropertyPtr positions, 
				ConstPropertyPtr selection, ConstPropertyPtr particleTypes, 
				ConstPropertyPtr bondTopology, ConstPropertyPtr bondPeriodicImages, const SimulationCell& simCell) :
			_positions(std::move(positions)), 
			_selection(std::move(selection)), 
			_particleTypes(std::move(particleTypes)), 
			_bondTopology(std::move(bondTopology)), 
			_bondPeriodicImages(std::move(bondPeriodicImages)),
			_simCell(simCell),
			_results(std::make_shared<ComputePolyhedraResults>()) {}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;

		/// Returns the mesh.
		const std::shared_ptr<HalfEdgeMesh<>>& mesh() const { return _results->mesh(); }
				
	private:

		/// Constructs the convex hull from a set of points and adds the resulting polyhedron to the mesh.
		void constructConvexHull(std::vector<Point3>& vecs);

	private:

		const ConstPropertyPtr _positions;
		const ConstPropertyPtr _selection;
		const ConstPropertyPtr _particleTypes;
		const ConstPropertyPtr _bondTopology;
		const ConstPropertyPtr _bondPeriodicImages;
		const SimulationCell _simCell;
		std::shared_ptr<ComputePolyhedraResults> _results;
	};

	/// The vis element for rendering the polyhedra.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(SurfaceMeshVis, surfaceMeshVis, setSurfaceMeshVis, PROPERTY_FIELD_DONT_PROPAGATE_MESSAGES | PROPERTY_FIELD_MEMORIZE | PROPERTY_FIELD_OPEN_SUBEDITOR);	
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
