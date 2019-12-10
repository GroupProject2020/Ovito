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
#include <ovito/stdobj/properties/PropertyStorage.h>
#include <ovito/particles/util/NearestNeighborFinder.h>
#include <3rdparty/ptm/ptm_functions.h>

extern "C" {
    typedef struct ptm_local_handle* ptm_local_handle_t;
}

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Util)

/**
 * \brief This class is a wrapper around the Polyhedral Template Matching algorithm
 *        implemented in the PTM third-party library.
 *
 * It allows clients to perform the PTM structure analysis for individual atoms.
 *
 * The PolyhedralTemplateMatchingModifier internally employs the PTMAlgorithm to
 * perform the actual calculation for every input particle of a system.
 */
class OVITO_PARTICLES_EXPORT PTMAlgorithm : private NearestNeighborFinder
{
public:
    /// The structure types known by the PTM routine.
    enum StructureType {
        OTHER = 0,			//< Unidentified structure
        FCC,				//< Face-centered cubic
        HCP,				//< Hexagonal close-packed
        BCC,				//< Body-centered cubic
        ICO,				//< Icosahedral structure
        SC,				//< Simple cubic structure
        CUBIC_DIAMOND,			//< Cubic diamond structure
        HEX_DIAMOND,			//< Hexagonal diamond structure
        GRAPHENE,			//< Graphene structure

        NUM_STRUCTURE_TYPES //< This counts the number of defined structure types.
    };
    Q_ENUMS(StructureType);

	/// The lattice ordering types known by the PTM routine.
	enum OrderingType {
		ORDERING_NONE = 0,
		ORDERING_PURE = 1,
		ORDERING_L10 = 2,
		ORDERING_L12_A = 3,
		ORDERING_L12_B = 4,
		ORDERING_B2 = 5,
		ORDERING_ZINCBLENDE_WURTZITE = 6,
		ORDERING_BORON_NITRIDE = 7,

		NUM_ORDERING_TYPES 	//< This just counts the number of defined ordering types.
	};
	Q_ENUMS(OrderingType);

#ifndef Q_CC_MSVC
    /// Maximum number of input nearest neighbors needed for the PTM analysis.
    static constexpr int MAX_INPUT_NEIGHBORS = 18;
#else // Workaround for a deficiency in the MSVC compiler:
    /// Maximum number of input nearest neighbors needed for the PTM analysis.
    enum { MAX_INPUT_NEIGHBORS = 18 };
#endif


#ifndef Q_CC_MSVC
    /// Maximum number of nearest neighbors for any structure returned by the PTM analysis routine.
    static constexpr int MAX_OUTPUT_NEIGHBORS = 16;
#else // Workaround for a deficiency in the MSVC compiler:
    /// Maximum number of nearest neighbors for any structure returned by the PTM analysis routine.
    enum { MAX_OUTPUT_NEIGHBORS = 16 };
#endif

    /// Creates the algorithm object.
    PTMAlgorithm();

    /// Sets the threshold for the RMSD that must not be exceeded for a structure match to be valid.
    /// A zero cutoff value turns off the threshold filtering. The default threshold value is 0.1.
    void setRmsdCutoff(FloatType cutoff) { _rmsdCutoff = cutoff; }

    /// Returns the threshold for the RMSD that must not be exceeded for a structure match to be valid.
    FloatType rmsdCutoff() const { return _rmsdCutoff; }

    /// Enables/disables the identification of a specific structure type by the PTM.
    /// When the PTMAlgorithm is created, identification is activated for no structure type.
    void setStructureTypeIdentification(StructureType structureType, bool enableIdentification) {
        _typesToIdentify[structureType] = enableIdentification;
    }

    /// Returns true if at least one of the supported structure types has been enabled for identification.
    bool isAnyStructureTypeEnabled() const {
        return std::any_of(_typesToIdentify.begin()+1, _typesToIdentify.end(), [](bool b) { return b; });
    }

    /// Activates the calculation of local elastic deformation gradients by the PTM algorithm (off by default).
    /// After a successful call to Kernel::identifyStructure(), use the Kernel::deformationGradient() method to access the computed
    /// deformation gradient tensor.
    void setCalculateDefGradient(bool calculateDefGradient) { _calculateDefGradient = calculateDefGradient; }

    /// Returns whether calculation of local elastic deformation gradients by the PTM algorithm is enabled.
    bool calculateDefGradient() const { return _calculateDefGradient; }

    /// Activates the identification of chemical ordering types and specifies the chemical types of the input particles.
    void setIdentifyOrdering(ConstPropertyPtr particleTypes) {
        _particleTypes = std::move(particleTypes);
        _identifyOrdering = (_particleTypes != nullptr);
    }

    /// \brief Initializes the PTMAlgorithm with the input system of particles.
    /// \param posProperty The particle coordinates.
    /// \param cell The simulation cell information.
    /// \param selectionProperty Per-particle selection flags determining which particles are included in the neighbor search (optional).
    /// \param promise A callback object that will be used to the report progress during the algorithm initialization (optional).
    /// \return \c false when the operation has been canceled by the user;
    ///         \c true on success.
    /// \throw Exception on error.
    bool prepare(const PropertyStorage& posProperty, const SimulationCell& cell, const PropertyStorage* selectionProperty = nullptr, Task* task = nullptr) {
        return NearestNeighborFinder::prepare(posProperty, cell, selectionProperty, task);
    }

    /// This nested class performs a PTM calculation on a single input particle.
    /// It is thread-safe to use several Kernel objects concurrently, initialized from the same PTMAlgorithm object.
    /// The Kernel object performs the PTM analysis and yields the identified structure type and, if a match has been detected,
    /// the ordered list of neighbor particles forming the structure around the central particle.
    class OVITO_PARTICLES_EXPORT Kernel : private NearestNeighborFinder::Query<MAX_INPUT_NEIGHBORS>
    {
    private:
        /// The internal query type for finding the input set of nearest neighbors.
        using NeighborQuery =  NearestNeighborFinder::Query<MAX_INPUT_NEIGHBORS>;

    public:
        /// Constructs a new kernel from the given algorithm object, which must have previously been initialized
        /// by a call to PTMAlgorithm::prepare().
        Kernel(const PTMAlgorithm& algo);

        /// Destructor.
        ~Kernel();

        /// Identifies the local structure of the given particle and builds the list of nearest neighbors
        /// that form that structure. Subsequently, in case of a successful match, additional outputs of the calculation
        /// can be retrieved with the query methods below.
        StructureType identifyStructure(size_t particleIndex, std::vector< uint64_t >& precachedNeighbors, Quaternion* qtarget);

        // Calculates the topological ordering of a particle's neighbors.
        int precacheNeighbors(size_t particleIndex, uint64_t* res);

        /// Returns the structure type identified by the PTM for the current particle.
        StructureType structureType() const { return _structureType; }

        /// Returns the root-mean-square deviation calculated by the PTM for the current particle.
        double rmsd() const { return _rmsd; }

        /// Returns the elastic deformation gradient computed by PTM for the current particle.
        const Matrix_3<double>& deformationGradient() const { return _F; }

        /// Returns the local interatomic distance parameter computed by the PTM routine for the current particle.
        double interatomicDistance() const { return _interatomicDistance; }

        /// Returns the local chemical ordering identified by the PTM routine for the current particle.
        OrderingType orderingType() const { return static_cast<OrderingType>(_orderingType); }

        /// Returns the local structure orientation computed by the PTM routine for the current particle.
        Quaternion orientation() const {
            return Quaternion((FloatType)_q[1], (FloatType)_q[2], (FloatType)_q[3], (FloatType)_q[0]);
        }

        /// The index of the best-matching structure template.
    	int bestTemplateIndex() const { return _bestTemplateIndex; }

        /// Returns the number of neighbors for the PTM structure found for the current particle.
        int numStructureNeighbors() const;

        /// Returns the neighbor information corresponding to the i-th neighbor in the PTM template
        /// identified for the current particle.
        const NearestNeighborFinder::Neighbor& getNeighborInfo(int index) const;

        /// Returns the ideal vector corresponding to the i-th neighbor in the PTM template
        /// identified for the current particle.
        const Vector_3<double>& getIdealNeighborVector(int index) const;

//TODO: don't leave this public
    	ptm_atomicenv_t _env;

    private:
        /// Reference to the parent algorithm object.
        const PTMAlgorithm& _algo;

        /// Thread-local storage needed by the PTM.
        ptm_local_handle_t _handle;

        // Output quantities computed by the PTM routine during the last call to identifyStructure():
        double _rmsd;
    	double _scale;
        double _interatomicDistance;
    	double _q[4];
        Matrix_3<double> _F{Matrix_3<double>::Zero()};
        StructureType _structureType = OTHER;
    	int32_t _orderingType = ORDERING_NONE;
    	int _bestTemplateIndex;
    	const double (*_bestTemplate)[3] = nullptr;
    	//int8_t _correspondences[MAX_INPUT_NEIGHBORS+1];
	std::vector<uint64_t> _cachedNeighbors;
    };

private:

    /// Bit array controlling which structures the PTM algorithm will look for.
    std::array<bool, NUM_STRUCTURE_TYPES> _typesToIdentify = {};

    /// Activates the identification of chemical orderings.
    bool _identifyOrdering = false;

    /// The chemical types of the input particles, needed for ordering analysis.
    ConstPropertyPtr _particleTypes;

    /// Activates the calculation of the elastic deformation gradient by PTM.
    bool _calculateDefGradient = false;

    /// The RMSD threshold that must not be exceeded.
    FloatType _rmsdCutoff = 0.1;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
