///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2019) Alexander Stukowski
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

#include <ovito/particles/Particles.h>
#include "PTMAlgorithm.h"

#include <3rdparty/ptm/ptm_functions.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Util)

/******************************************************************************
* Creates the algorithm object.
******************************************************************************/
PTMAlgorithm::PTMAlgorithm() : NearestNeighborFinder(MAX_INPUT_NEIGHBORS)
{
	ptm_initialize_global();
}

/******************************************************************************
* Constructs a new kernel from the given algorithm object, which must have
* previously been initialized by a call to PTMAlgorithm::prepare().
******************************************************************************/
PTMAlgorithm::Kernel::Kernel(const PTMAlgorithm& algo) : NeighborQuery(algo), _algo(algo)
{
	// Reserve thread-local storage of PTM routine.
	_handle = ptm_initialize_local();
}

/******************************************************************************
* Destructor.
******************************************************************************/
PTMAlgorithm::Kernel::~Kernel()
{
	// Release thread-local storage of PTM routine.
	ptm_uninitialize_local(_handle);
}

// Neighbor data passed to PTM routine.  Used in the get_neighbours callback function.
typedef struct
{
	const NearestNeighborFinder* neighFinder;
	ConstPropertyPtr particleTypes;
	std::vector< uint64_t > *precachedNeighbors;

} ptmnbrdata_t;

static int get_neighbours(void* vdata, size_t _unused_lammps_variable, size_t atom_index, int num_requested, int* ordering, size_t* nbr_indices, int32_t* numbers, double (*nbr_pos)[3])
{
	ptmnbrdata_t* nbrdata = (ptmnbrdata_t*)vdata;
	const NearestNeighborFinder* neighFinder = nbrdata->neighFinder;
	ConstPropertyPtr particleTypes = nbrdata->particleTypes;
	std::vector< uint64_t >& precachedNeighbors = *nbrdata->precachedNeighbors;

	// Find nearest neighbors.
	NearestNeighborFinder::Query<PTMAlgorithm::MAX_INPUT_NEIGHBORS> neighQuery(*neighFinder);
	neighQuery.findNeighbors(atom_index);
	int numNeighbors = std::min(num_requested - 1, neighQuery.results().size());
	OVITO_ASSERT(numNeighbors <= PTMAlgorithm::MAX_INPUT_NEIGHBORS);

	int permutation[PTM_MAX_INPUT_POINTS];
	ptm_index_to_permutation(numNeighbors, precachedNeighbors[atom_index], permutation);

	// Bring neighbor coordinates into a form suitable for the PTM library.
	ordering[0] = 0;
	nbr_indices[0] = atom_index;
	nbr_pos[0][0] = nbr_pos[0][1] = nbr_pos[0][2] = 0;
	for(int i = 0; i < numNeighbors; i++) {

		//ordering[index] = permutation[i] + 1;
		nbr_indices[i+1] = neighQuery.results()[permutation[i]].index;
		nbr_pos[i+1][0] = neighQuery.results()[permutation[i]].delta.x();
		nbr_pos[i+1][1] = neighQuery.results()[permutation[i]].delta.y();
		nbr_pos[i+1][2] = neighQuery.results()[permutation[i]].delta.z();
	}

	// Build list of particle types for ordering identification.
	if(particleTypes != nullptr) {
		numbers[0] = particleTypes->getInt(atom_index);
		for(int i = 0; i < numNeighbors; i++) {
			numbers[i+1] = particleTypes->getInt(neighQuery.results()[permutation[i]].index);
		}
	}
	else {
		for(int i = 0; i < numNeighbors + 1; i++) {
			numbers[i] = 0;
		}
	}

	return numNeighbors + 1;
}


/******************************************************************************
* Identifies the local structure of the given particle and builds the list of
* nearest neighbors that form the structure.
******************************************************************************/
PTMAlgorithm::StructureType PTMAlgorithm::Kernel::identifyStructure(size_t particleIndex, std::vector< uint64_t >& precachedNeighbors, Quaternion* qtarget)
{
	// Validate input.
	if(particleIndex >= _algo.particleCount())
		throw Exception("Particle index is out of range.");

	// Make sure public constants remain consistent with internal ones from the PTM library.
	OVITO_STATIC_ASSERT(MAX_INPUT_NEIGHBORS == PTM_MAX_INPUT_POINTS - 1);
	OVITO_STATIC_ASSERT(MAX_OUTPUT_NEIGHBORS == PTM_MAX_NBRS);


	ptmnbrdata_t nbrdata;
	nbrdata.neighFinder = &_algo;
	nbrdata.particleTypes = _algo._identifyOrdering ? _algo._particleTypes : nullptr;
	nbrdata.precachedNeighbors = &precachedNeighbors;

	int32_t flags = 0;
	if(_algo._typesToIdentify[SC]) flags |= PTM_CHECK_SC;
	if(_algo._typesToIdentify[FCC]) flags |= PTM_CHECK_FCC;
	if(_algo._typesToIdentify[HCP]) flags |= PTM_CHECK_HCP;
	if(_algo._typesToIdentify[ICO]) flags |= PTM_CHECK_ICO;
	if(_algo._typesToIdentify[BCC]) flags |= PTM_CHECK_BCC;
	if(_algo._typesToIdentify[CUBIC_DIAMOND]) flags |= PTM_CHECK_DCUB;
	if(_algo._typesToIdentify[HEX_DIAMOND]) flags |= PTM_CHECK_DHEX;
	if(_algo._typesToIdentify[GRAPHENE]) flags |= PTM_CHECK_GRAPHENE;

	// Call PTM library to identify the local structure.
	int32_t type;
	double F_res[3];

	ptm_index(_handle,
			particleIndex, get_neighbours, (void*)&nbrdata,
			flags,
			true,
			&type,
			&_orderingType,
			&_scale,
			&_rmsd,
			_q,
			_algo._calculateDefGradient ? _F.elements() : nullptr,
			_algo._calculateDefGradient ? F_res : nullptr,
			nullptr,
			nullptr,
			&_interatomicDistance,
			nullptr,
			&_bestTemplateIndex,
			&_bestTemplate,
			_correspondences);

	// Convert PTM classification back to our own scheme.
	if(type == PTM_MATCH_NONE || (_algo._rmsdCutoff != 0 && _rmsd > _algo._rmsdCutoff)) {
		_structureType = OTHER;
		_orderingType = ORDERING_NONE;
		_rmsd = 0;
		_interatomicDistance = 0;
		_q[0] = _q[1] = _q[2] = _q[3] = 0;
		_scale = 0;
		_bestTemplateIndex = 0;
		_F.setZero();
	}
	else {
		if(type == PTM_MATCH_SC) _structureType = SC;
		else if(type == PTM_MATCH_FCC) _structureType = FCC;
		else if(type == PTM_MATCH_HCP) _structureType = HCP;
		else if(type == PTM_MATCH_ICO) _structureType = ICO;
		else if(type == PTM_MATCH_BCC) _structureType = BCC;
		else if(type == PTM_MATCH_DCUB) _structureType = CUBIC_DIAMOND;
		else if(type == PTM_MATCH_DHEX) _structureType = HEX_DIAMOND;
		else if(type == PTM_MATCH_GRAPHENE) _structureType = GRAPHENE;
		else {
			OVITO_ASSERT(false);
			_structureType = OTHER;
		}
	}

	if (_structureType != OTHER && qtarget != nullptr) {

		//arrange orientation in PTM format
		double qtarget_ptm[4] = { qtarget->w(), qtarget->x(), qtarget->y(), qtarget->z() };

		double disorientation = 0;	//TODO: this is probably not needed
		int template_index = ptm_remap_template(type, true, _bestTemplateIndex, qtarget_ptm, _q, &disorientation, _correspondences, &_bestTemplate);
		if (template_index < 0)
			return _structureType;

		_bestTemplateIndex = template_index;
	}

	return _structureType;

#if 0
////--------replace this with saved PTM data--------------------

//todo: when getting stored PTM data (when PTM is not called here), assert that output_conventional_orientations=true was used.

	//TODO: don't hardcode input flags
	int32_t flags = PTM_CHECK_FCC | PTM_CHECK_DCUB | PTM_CHECK_GRAPHENE;// | PTM_CHECK_HCP | PTM_CHECK_BCC;

	// Call PTM library to identify local structure.
	int32_t type = PTM_MATCH_NONE, alloy_type = PTM_ALLOY_NONE;
	double scale, interatomic_distance;
	double rmsd;
	double q[4];
	int8_t correspondences[PolyhedralTemplateMatchingModifier::MAX_NEIGHBORS+1];
	ptm_index(handle, numNeighbors + 1, points, nullptr, flags, true,
			&type, &alloy_type, &scale, &rmsd, q, nullptr, nullptr, nullptr, nullptr,
			&interatomic_distance, nullptr, nullptr, nullptr, correspondences);
	if (rmsd > 0.1) {	//TODO: don't hardcode threshold
		type = PTM_MATCH_NONE;
		rmsd = INFINITY;
	}

	if (type == PTM_MATCH_NONE)
		return;
////------------------------------------------------------------

	double qmapped[4];
	double qtarget[4] = {qw, qx, qy, qz};	//PTM quaterion ordering
	const double (*best_template)[3] = NULL;
	int _template_index = ptm_remap_template(type, true, input_template_index, qtarget, q, qmapped, &disorientation, correspondences, &best_template);
	if (_template_index < 0)
		return;

	//arrange orientation in OVITO format
	qres[0] = qmapped[1];	//qx
	qres[1] = qmapped[2];	//qy
	qres[2] = qmapped[3];	//qz
	qres[3] = qmapped[0];	//qw

	//structure_type = type;
	template_index = _template_index;
	for (int i=0;i<ptm_num_nbrs[type];i++) {

		int index = correspondences[i+1] - 1;
		auto r = neighQuery.results()[index];

		Vector3 tcoords(best_template[i+1][0], best_template[i+1][1], best_template[i+1][2]);
		result.emplace_back(r.delta, tcoords, r.distanceSq, r.index);
	}
#endif
}

int PTMAlgorithm::Kernel::precacheNeighbors(size_t particleIndex, uint64_t* res)
{
	// Validate input.
	if(particleIndex >= _algo.particleCount())
		throw Exception("Particle index is out of range.");

	// Make sure public constants remain consistent with internal ones from the PTM library.
	OVITO_STATIC_ASSERT(MAX_INPUT_NEIGHBORS == PTM_MAX_INPUT_POINTS - 1);
	OVITO_STATIC_ASSERT(MAX_OUTPUT_NEIGHBORS == PTM_MAX_NBRS);

	// Find nearest neighbors around the central particle.
	NeighborQuery::findNeighbors(particleIndex);
	int numNeighbors = NeighborQuery::results().size();

	double points[PTM_MAX_INPUT_POINTS - 1][3];
	for(int i = 0; i < numNeighbors; i++) {
		points[i][0] = NeighborQuery::results()[i].delta.x();
		points[i][1] = NeighborQuery::results()[i].delta.y();
		points[i][2] = NeighborQuery::results()[i].delta.z();
	}

	return ptm_preorder_neighbours(_handle, numNeighbors, points, res);
}

/******************************************************************************
* Returns the number of neighbors for the PTM structure found for the current particle.
******************************************************************************/
int PTMAlgorithm::Kernel::numStructureNeighbors() const
{
	return ptm_num_nbrs[_structureType];
}

/******************************************************************************
* Returns the neighbor information corresponding to the i-th neighbor in the
* PTM template identified for the current particle.
******************************************************************************/
const NearestNeighborFinder::Neighbor& PTMAlgorithm::Kernel::getNeighborInfo(int index) const
{
	OVITO_ASSERT(_structureType != OTHER);
	OVITO_ASSERT(index >= 0 && index < numStructureNeighbors());
	int mappedIndex = _correspondences[index + 1] - 1;
	OVITO_ASSERT(mappedIndex >= 0 && mappedIndex < results().size());
	return results()[mappedIndex];
}

/******************************************************************************
* Returns the ideal vector corresponding to the i-th neighbor in the PTM template
* identified for the current particle.
******************************************************************************/
const Vector_3<double>& PTMAlgorithm::Kernel::getIdealNeighborVector(int index) const
{
	OVITO_ASSERT(_structureType != OTHER);
	OVITO_ASSERT(index >= 0 && index < numStructureNeighbors());
	OVITO_ASSERT(_bestTemplate != nullptr);
	return *reinterpret_cast<const Vector_3<double>*>(_bestTemplate[index + 1]);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
