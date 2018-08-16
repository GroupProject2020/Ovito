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
#include <plugins/stdobj/series/DataSeriesObject.h>
#include <core/dataset/pipeline/AsynchronousModifier.h>
#include <core/dataset/pipeline/AsynchronousModifierApplication.h>

#include <boost/container/flat_map.hpp>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief This modifier computes the coordination number of each particle (i.e. the number of neighbors within a given cutoff radius).
 */
class OVITO_PARTICLES_EXPORT CoordinationAnalysisModifier : public AsynchronousModifier
{
	/// Give this modifier class its own metaclass.
	class CoordinationAnalysisModifierClass : public AsynchronousModifier::OOMetaClass 
	{
	public:

		/// Inherit constructor from base metaclass.
		using AsynchronousModifier::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const PipelineFlowState& input) const override;
	};
		
	Q_OBJECT
	OVITO_CLASS_META(CoordinationAnalysisModifier, CoordinationAnalysisModifierClass)

	Q_CLASSINFO("ClassNameAlias", "CoordinationNumberModifier");
	Q_CLASSINFO("DisplayName", "Coordination analysis");
	Q_CLASSINFO("ModifierCategory", "Analysis");

public:

	/// Constructor.
	Q_INVOKABLE CoordinationAnalysisModifier(DataSet* dataset);

protected:
	
	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;	

private:

	/// Computes the modifier's results.
	class CoordinationAnalysisEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		CoordinationAnalysisEngine(ParticleOrderingFingerprint fingerprint, ConstPropertyPtr positions, const SimulationCell& simCell, 
				FloatType cutoff, int rdfSampleCount, ConstPropertyPtr particleTypes, boost::container::flat_map<int,QString> uniqueTypeIds) :
			_positions(std::move(positions)), 
			_simCell(simCell),
			_cutoff(cutoff),
			_computePartialRdfs(particleTypes),
			_particleTypes(std::move(particleTypes)),
			_uniqueTypeIds(std::move(uniqueTypeIds)),
			_coordinationNumbers(ParticleProperty::createStandardStorage(fingerprint.particleCount(), ParticleProperty::CoordinationProperty, true)),
			_inputFingerprint(std::move(fingerprint))
		{
			size_t componentCount = _computePartialRdfs ? (this->uniqueTypeIds().size() * (this->uniqueTypeIds().size()+1) / 2) : 1;
			_rdfX = std::make_shared<PropertyStorage>(rdfSampleCount, PropertyStorage::Float, 1, 0, tr("Pair separation distance"), false); 
			_rdfY = std::make_shared<PropertyStorage>(rdfSampleCount, PropertyStorage::Float, componentCount, 0, tr("g(r)"), true);
			if(_computePartialRdfs) {
				QStringList names;
				for(const auto& t1 : this->uniqueTypeIds()) {
					for(const auto& t2 : this->uniqueTypeIds()) {
						if(t1.first <= t2.first)
							names.push_back(QStringLiteral("%1-%2").arg(t1.second, t2.second));
					}
				}
				_rdfY->setComponentNames(std::move(names));
			}
		}

		/// This method is called by the system after the computation was successfully completed.
		virtual void cleanup() override {
			_positions.reset();
			_particleTypes.reset();
			ComputeEngine::cleanup();
		}

		/// Computes the modifier's results.
		virtual void perform() override;

		/// Injects the computed results into the data pipeline.
		virtual PipelineFlowState emitResults(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

		/// Returns the property storage that contains the computed coordination numbers.
		const PropertyPtr& coordinationNumbers() const { return _coordinationNumbers; }

		/// Returns the property storage containing the x-coordinates of the data points of the RDF histograms.
		const PropertyPtr& rdfX() const { return _rdfX; }

		/// Returns the property storage array containing the y-coordinates of the data points of the RDF histograms.
		const PropertyPtr& rdfY() const { return _rdfY; }

		/// Returns the property storage that contains the input particle positions.
		const ConstPropertyPtr& positions() const { return _positions; }

		/// Returns the property storage that contains the input particle types.
		const ConstPropertyPtr& particleTypes() const { return _particleTypes; }

		/// Returns the simulation cell data.
		const SimulationCell& cell() const { return _simCell; }

		/// Returns the cutoff radius.
		FloatType cutoff() const { return _cutoff; }

		/// Returns the set of particle type identifiers in the system.
		const boost::container::flat_map<int,QString>& uniqueTypeIds() const { return _uniqueTypeIds; }

	private:

		const FloatType _cutoff;
		const SimulationCell _simCell;
		bool _computePartialRdfs;
		boost::container::flat_map<int,QString> _uniqueTypeIds;
		ConstPropertyPtr _positions;
		ConstPropertyPtr _particleTypes;
		const PropertyPtr _coordinationNumbers;
		PropertyPtr _rdfX;
		PropertyPtr _rdfY;
		ParticleOrderingFingerprint _inputFingerprint;
	};

private:

	/// Controls the cutoff radius for the neighbor lists.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, cutoff, setCutoff, PROPERTY_FIELD_MEMORIZE);

	/// Controls the number of RDF histogram bins.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(int, numberOfBins, setNumberOfBins, PROPERTY_FIELD_MEMORIZE);

	/// Controls the computation of partials RDFs.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, computePartialRDF, setComputePartialRDF, PROPERTY_FIELD_MEMORIZE);
};

/**
 * \brief The type of ModifierApplication created for a CoordinationAnalysisModifier 
 *        when it is inserted into in a data pipeline. It stores results computed by the
 *        modifier's compute engine so that they can be displayed in the modifier's UI panel.
 */
class OVITO_PARTICLES_EXPORT CoordinationAnalysisModifierApplication : public AsynchronousModifierApplication
{
	Q_OBJECT
	OVITO_CLASS(CoordinationAnalysisModifierApplication)
	Q_CLASSINFO("ClassNameAlias", "CoordinationNumberModifierApplication");

public:

	/// Constructor.
	Q_INVOKABLE CoordinationAnalysisModifierApplication(DataSet* dataset) : AsynchronousModifierApplication(dataset) {}
 
private:
 
	/// The RDF histogram(s).
	DECLARE_RUNTIME_PROPERTY_FIELD_FLAGS(OORef<DataSeriesObject>, rdf, setRdf, PROPERTY_FIELD_NO_CHANGE_MESSAGE);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
