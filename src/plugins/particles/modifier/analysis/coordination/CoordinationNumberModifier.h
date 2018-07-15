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
#include <plugins/particles/util/CutoffNeighborFinder.h>
#include <plugins/particles/util/ParticleOrderingFingerprint.h>
#include <plugins/particles/objects/ParticleProperty.h>
#include <plugins/stdobj/simcell/SimulationCell.h>
#include <plugins/stdobj/properties/PropertyStorage.h>
#include <core/dataset/pipeline/AsynchronousModifier.h>
#include <core/dataset/pipeline/AsynchronousModifierApplication.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief This modifier computes the coordination number of each particle (i.e. the number of neighbors within a given cutoff radius).
 */
class OVITO_PARTICLES_EXPORT CoordinationNumberModifier : public AsynchronousModifier
{
	/// Give this modifier class its own metaclass.
	class CoordinationNumberModifierClass : public AsynchronousModifier::OOMetaClass 
	{
	public:

		/// Inherit constructor from base metaclass.
		using AsynchronousModifier::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const PipelineFlowState& input) const override;
	};
		
	Q_OBJECT
	OVITO_CLASS_META(CoordinationNumberModifier, CoordinationNumberModifierClass)

	Q_CLASSINFO("DisplayName", "Coordination analysis");
	Q_CLASSINFO("ModifierCategory", "Analysis");

public:

	/// Constructor.
	Q_INVOKABLE CoordinationNumberModifier(DataSet* dataset);

protected:
	
	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;	

private:

	/// Computes the modifier's results.
	class CoordinationAnalysisEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		CoordinationAnalysisEngine(ParticleOrderingFingerprint fingerprint, ConstPropertyPtr positions, const SimulationCell& simCell, FloatType cutoff, int rdfSampleCount) :
			_positions(std::move(positions)), 
			_simCell(simCell),
			_cutoff(cutoff),
			_rdfSampleCount(rdfSampleCount),
			_coordinationNumbers(ParticleProperty::createStandardStorage(fingerprint.particleCount(), ParticleProperty::CoordinationProperty, true)),
			_rdfX(std::make_shared<PropertyStorage>(rdfSampleCount, PropertyStorage::Float, 1, 0, tr("Pair separation distance"), false)), 
			_rdfY(std::make_shared<PropertyStorage>(rdfSampleCount, PropertyStorage::Float, 1, 0, tr("g(r)"), true)),
			_inputFingerprint(std::move(fingerprint)) {}

		/// This method is called by the system after the computation was successfully completed.
		virtual void cleanup() override {
			_positions.reset();
			ComputeEngine::cleanup();
		}

		/// Computes the modifier's results.
		virtual void perform() override;

		/// Injects the computed results into the data pipeline.
		virtual PipelineFlowState emitResults(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

		/// Returns the property storage that contains the computed coordination numbers.
		const PropertyPtr& coordinationNumbers() const { return _coordinationNumbers; }

		/// Returns the property storage containing the x-coordinates of the data points of the RDF histogram.
		const PropertyPtr& rdfX() const { return _rdfX; }

		/// Returns the property storage containing the y-coordinates of the data points of the RDF histogram.
		const PropertyPtr& rdfY() const { return _rdfY; }

		/// Returns the property storage that contains the input particle positions.
		const ConstPropertyPtr& positions() const { return _positions; }

		/// Returns the simulation cell data.
		const SimulationCell& cell() const { return _simCell; }

		/// Returns the cutoff radius.
		FloatType cutoff() const { return _cutoff; }

	private:

		const FloatType _cutoff;
		const int _rdfSampleCount;
		const SimulationCell _simCell;
		ConstPropertyPtr _positions;
		const PropertyPtr _coordinationNumbers;
		const PropertyPtr _rdfX;
		const PropertyPtr _rdfY;
		ParticleOrderingFingerprint _inputFingerprint;
	};

private:

	/// Controls the cutoff radius for the neighbor lists.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, cutoff, setCutoff, PROPERTY_FIELD_MEMORIZE);

	/// Controls the number of RDF histogram bins.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(int, numberOfBins, setNumberOfBins, PROPERTY_FIELD_MEMORIZE);
};

/**
 * \brief The type of ModifierApplication created for a CoordinationNumberModifier 
 *        when it is inserted into in a data pipeline. Its stores results computed by the
 *        modifier's compute engine so that they can be displayed in the modifier's UI panel.
 */
 class OVITO_PARTICLES_EXPORT CoordinationNumberModifierApplication : public AsynchronousModifierApplication
{
	Q_OBJECT
	OVITO_CLASS(CoordinationNumberModifierApplication)

public:

	/// Constructor.
	Q_INVOKABLE CoordinationNumberModifierApplication(DataSet* dataset) : AsynchronousModifierApplication(dataset) {}
 
private:
 
	/// The x-coordinates of the RDF histogram points.
	DECLARE_RUNTIME_PROPERTY_FIELD_FLAGS(PropertyPtr, rdfX, setRdfX, PROPERTY_FIELD_NO_CHANGE_MESSAGE);

	/// The y-coordinates of the RDF histogram points.
	DECLARE_RUNTIME_PROPERTY_FIELD_FLAGS(PropertyPtr, rdfY, setRdfY, PROPERTY_FIELD_NO_CHANGE_MESSAGE);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
