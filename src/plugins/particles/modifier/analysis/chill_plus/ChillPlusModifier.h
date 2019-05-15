///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2019) Henrik Andersen Sveinsson
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

#include <boost/math/special_functions/spherical_harmonic.hpp>
#include <boost/numeric/ublas/matrix.hpp>

#include <plugins/particles/Particles.h>
#include <plugins/particles/modifier/analysis/StructureIdentificationModifier.h>




namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief This modifier computes the ice-like structure of a particle
 */
class OVITO_PARTICLES_EXPORT ChillPlusModifier : public StructureIdentificationModifier
{
	/// Give this modifier class its own metaclass.
		
	Q_OBJECT
	OVITO_CLASS(ChillPlusModifier)

	Q_CLASSINFO("ClassNameAlias", "ChillPlusModifier");
	Q_CLASSINFO("DisplayName", "Chill+");
	Q_CLASSINFO("ModifierCategory", "Structure identification");

public:
    /// The structure types recognized by the bond angle analysis.
	enum StructureType {
		OTHER = 0,				//< Unidentified structure
		HEXAGONAL_ICE,					//< cubic ice
		CUBIC_ICE,					//< hexagonal ice
		INTERFACIAL_ICE,					//< Interfacial ice
		HYDRATE,					//< hydrate
        INTERFACIAL_HYDRATE,                 //< interfacial hydrate
		NUM_STRUCTURE_TYPES 	//< This just counts the number of defined structure types.
	};
	Q_ENUMS(StructureType);
	/// Constructor.
	Q_INVOKABLE ChillPlusModifier(DataSet* dataset);

protected:

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

private:

	/// Computes the modifier's results.
	class ChillPlusEngine : public StructureIdentificationEngine
	{
	public:

		/// Constructor.
		using StructureIdentificationEngine::StructureIdentificationEngine;

		/// Computes the modifier's results.
        std::complex<float> compute_q_lm(CutoffNeighborFinder& neighFinder, size_t particleIndex, int, int);
		virtual void perform() override;

		/// Injects the computed results into the data pipeline.
		virtual void emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

        FloatType cutoff() const { return _cutoff; }
        
        StructureType determineStructure(CutoffNeighborFinder& neighFinder, size_t particleIndex, const QVector<bool>& typesToIdentify);

    private: 
        const FloatType _cutoff = 3.5;
        boost::numeric::ublas::matrix<std::complex<float>> q_values;
        std::pair<float, float> polar_asimuthal(Vector3 delta);
  
	};
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
