///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2019) Alexander Stukowski
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

#include <plugins/particles/Particles.h>
#include <plugins/particles/modifier/analysis/StructureIdentificationModifier.h>

#include <boost/math/special_functions/spherical_harmonic.hpp>
#include <boost/numeric/ublas/matrix.hpp>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief This modifier implements the Chill+ algorithm [Nguyen & Molinero, J. Phys. Chem. B 2015, 119, 9369-9376]
 *        for identifying various water phases.
 */
class OVITO_PARTICLES_EXPORT ChillPlusModifier : public StructureIdentificationModifier
{
    Q_OBJECT
    OVITO_CLASS(ChillPlusModifier)

    Q_CLASSINFO("DisplayName", "Chill+");
    Q_CLASSINFO("ModifierCategory", "Structure identification");

public:

    /// The structure types recognized by the Chill+ algorithm.
    enum StructureType {
        OTHER = 0,				//< Unidentified structure
        HEXAGONAL_ICE,			//< Cubic ice
        CUBIC_ICE,				//< Hexagonal ice
        INTERFACIAL_ICE,		//< Interfacial ice
        HYDRATE,				//< Hydrate
        INTERFACIAL_HYDRATE,    //< Interfacial hydrate

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
        ChillPlusEngine(ParticleOrderingFingerprint fingerprint, ConstPropertyPtr positions, const SimulationCell& simCell, QVector<bool> typesToIdentify, ConstPropertyPtr selection, FloatType cutoff) :
            StructureIdentificationEngine(fingerprint, positions, simCell, typesToIdentify, selection),
            _cutoff(cutoff) {}

        /// Computes the modifier's results.
        virtual void perform() override;

        /// Injects the computed results into the data pipeline.
        virtual void emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

        StructureType determineStructure(CutoffNeighborFinder& neighFinder, size_t particleIndex, const QVector<bool>& typesToIdentify);

        /// Returns the value of the cutoff parameter.
        FloatType cutoff() const { return _cutoff; }

    private:

        /// Helper method.
        static std::complex<float> compute_q_lm(CutoffNeighborFinder& neighFinder, size_t particleIndex, int, int);

        /// Helper method.
        static std::pair<float, float> polar_asimuthal(const Vector3& delta);

        const FloatType _cutoff;
        boost::numeric::ublas::matrix<std::complex<float>> q_values;
    };

    DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, cutoff, setCutoff, PROPERTY_FIELD_MEMORIZE);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
