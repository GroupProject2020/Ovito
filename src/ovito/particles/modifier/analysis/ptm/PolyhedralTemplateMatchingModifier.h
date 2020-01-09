////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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
#include <ovito/particles/modifier/analysis/StructureIdentificationModifier.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/table/DataTable.h>
#include "PTMAlgorithm.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief A modifier that uses the Polyhedral Template Matching (PTM) method to identify
 *        local coordination structures.
 */
class OVITO_PARTICLES_EXPORT PolyhedralTemplateMatchingModifier : public StructureIdentificationModifier
{
	Q_OBJECT
	OVITO_CLASS(PolyhedralTemplateMatchingModifier)

	Q_CLASSINFO("DisplayName", "Polyhedral template matching");
	Q_CLASSINFO("ModifierCategory", "Structure identification");

public:

	/// Constructor.
	Q_INVOKABLE PolyhedralTemplateMatchingModifier(DataSet* dataset);

	/// This method indicates whether cached computation results of the modifier should be discarded whenever
	/// a parameter of the modifier changes.
	virtual bool discardResultsOnModifierChange(const PropertyFieldEvent& event) const override {
		// Avoid a recomputation from scratch if the RMSD cutoff has been changed.
		if(event.field() == &PROPERTY_FIELD(rmsdCutoff)) return false;
		return StructureIdentificationModifier::discardResultsOnModifierChange(event);
	}

protected:

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

private:

	/// Analysis engine that performs the PTM.
	class PTMEngine : public StructureIdentificationEngine
	{
	public:

		/// Constructor.
		PTMEngine(ConstPropertyPtr positions, ParticleOrderingFingerprint fingerprint, ConstPropertyPtr particleTypes, const SimulationCell& simCell,
				QVector<bool> typesToIdentify, ConstPropertyPtr selection,
				bool outputInteratomicDistance, bool outputOrientation, bool outputDeformationGradient) :
			StructureIdentificationEngine(std::move(fingerprint), positions, simCell, std::move(typesToIdentify), std::move(selection)),
			_rmsd(std::make_shared<PropertyStorage>(positions->size(), PropertyStorage::Float, 1, 0, tr("RMSD"), false)),
			_interatomicDistances(outputInteratomicDistance ? std::make_shared<PropertyStorage>(positions->size(), PropertyStorage::Float, 1, 0, tr("Interatomic Distance"), true) : nullptr),
			_orientations(outputOrientation ? ParticlesObject::OOClass().createStandardStorage(positions->size(), ParticlesObject::OrientationProperty, true) : nullptr),
			_deformationGradients(outputDeformationGradient ? ParticlesObject::OOClass().createStandardStorage(positions->size(), ParticlesObject::ElasticDeformationGradientProperty, true) : nullptr),
			_orderingTypes(particleTypes ? std::make_shared<PropertyStorage>(positions->size(), PropertyStorage::Int, 1, 0, tr("Ordering Type"), true) : nullptr)
			{
				_algorithm.emplace();
				_algorithm->setCalculateDefGradient(outputDeformationGradient);
				_algorithm->setIdentifyOrdering(particleTypes);
				_algorithm->setRmsdCutoff(0.0); // Note: We do our own RMSD threshold filtering in postProcessStructureTypes().
			}

		/// This method is called by the system after the computation was successfully completed.
		virtual void cleanup() override {
			_algorithm.reset();
			StructureIdentificationEngine::cleanup();
		}

		/// Computes the modifier's results.
		virtual void perform() override;

		/// Injects the computed results into the data pipeline.
		virtual void emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

		const PropertyPtr& rmsd() const { return _rmsd; }
		const PropertyPtr& interatomicDistances() const { return _interatomicDistances; }
		const PropertyPtr& orientations() const { return _orientations; }
		const PropertyPtr& deformationGradients() const { return _deformationGradients; }
		const PropertyPtr& orderingTypes() const { return _orderingTypes; }

		/// Returns the RMSD value range of the histogram.
		FloatType rmsdHistogramRange() const { return _rmsdHistogramRange; }

		/// Returns the histogram of computed RMSD values.
		const PropertyPtr& rmsdHistogram() const { return _rmsdHistogram; }

	protected:

		/// Post-processes the per-particle structure types before they are output to the data pipeline.
		virtual PropertyPtr postProcessStructureTypes(TimePoint time, ModifierApplication* modApp, const PropertyPtr& structures) override;

	private:

		/// The internal PTM algorithm object.
		/// Store it in an optional<> so that it can be released early in the cleanup() method.
		boost::optional<PTMAlgorithm> _algorithm;

		// Modifier outputs:
		const PropertyPtr _rmsd;
		const PropertyPtr _interatomicDistances;
		const PropertyPtr _orientations;
		const PropertyPtr _deformationGradients;
		const PropertyPtr _orderingTypes;
		PropertyPtr _rmsdHistogram;
		FloatType _rmsdHistogramRange;
	};

private:

	/// The RMSD cutoff.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, rmsdCutoff, setRmsdCutoff, PROPERTY_FIELD_MEMORIZE);

	/// Controls the output of the per-particle RMSD values.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, outputRmsd, setOutputRmsd);

	/// Controls the output of local interatomic distances.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, outputInteratomicDistance, setOutputInteratomicDistance, PROPERTY_FIELD_MEMORIZE);

	/// Controls the output of local orientations.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, outputOrientation, setOutputOrientation, PROPERTY_FIELD_MEMORIZE);

	/// Controls the output of elastic deformation gradients.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, outputDeformationGradient, setOutputDeformationGradient);

	/// Controls the output of alloy ordering types.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, outputOrderingTypes, setOutputOrderingTypes, PROPERTY_FIELD_MEMORIZE);

	/// Contains the list of ordering types recognized by this analysis modifier.
	DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD(ElementType, orderingTypes, setOrderingTypes);
};


OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
