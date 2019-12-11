/*Copyright (c) 2016 PM Larsen

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

//todo: normalize vertices

#include <cstdlib>
#include <cmath>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <unordered_set>
#include <cstdint>
#include "ptm_constants.h"
#include "ptm_voronoi_cell.h"
#include "ptm_multishell.h"
#include "ptm_normalize_vertices.h"


namespace ptm {

typedef struct
{
	int rank;
	int inner;
	int correspondences;
	size_t atom_index;
	int32_t number;
	double delta[3];
} atomorder_t;

static bool atomorder_compare(atomorder_t const& a, atomorder_t const& b)
{
	return a.rank < b.rank;
}

#define MAX_INNER 4

int calculate_two_shell_neighbour_ordering(	int num_inner, int num_outer,
						size_t atom_index, int (get_neighbours)(void* vdata, size_t _unused_lammps_variable, size_t atom_index, int num, ptm_atomicenv_t* env), void* nbrlist,
						ptm_atomicenv_t* output)
{
	assert(num_inner <= MAX_INNER);

	ptm_atomicenv_t central;
	int num_input_points = get_neighbours(nbrlist, -1, atom_index, PTM_MAX_INPUT_POINTS, &central);
	if (num_input_points < num_inner + 1)
		return -1;

	std::unordered_set<size_t> claimed;
	for (int i=0;i<num_inner+1;i++)
	{
		output->correspondences[i] = central.correspondences[i];
		output->atom_indices[i] = central.atom_indices[i];
		output->numbers[i] = central.numbers[i];
		memcpy(output->points[i], central.points[i], 3 * sizeof(double));

		claimed.insert(central.atom_indices[i]);
	}

	int num_inserted = 0;
	atomorder_t data[MAX_INNER * PTM_MAX_INPUT_POINTS];
	for (int i=0;i<num_inner;i++)
	{
		ptm_atomicenv_t inner;
		int num_points = get_neighbours(nbrlist, -1, central.atom_indices[1 + i], PTM_MAX_INPUT_POINTS, &inner);
		if (num_points < num_inner + 1)
			return -1;

		for (int j=0;j<num_points;j++)
		{
			size_t key = inner.atom_indices[j];

			bool already_claimed = claimed.find(key) != claimed.end();
			if (already_claimed)
				continue;

			data[num_inserted].inner = i;
			data[num_inserted].rank = j;
			data[num_inserted].correspondences = inner.correspondences[j];
			data[num_inserted].atom_index = inner.atom_indices[j];
			data[num_inserted].number = inner.numbers[j];

			memcpy(data[num_inserted].delta, inner.points[j], 3 * sizeof(double));
			for (int k=0;k<3;k++)
				data[num_inserted].delta[k] += central.points[1 + i][k];

			num_inserted++;
		}
	}

	std::sort(data, data + num_inserted, &atomorder_compare);

	int num_found = 0;
	int counts[MAX_INNER] = {0};
	for (int i=0;i<num_inserted;i++)
	{
		int inner = data[i].inner;
		int nbr_atom_index = data[i].atom_index;

		bool already_claimed = claimed.find(nbr_atom_index) != claimed.end();
		if (counts[inner] >= num_outer || already_claimed)
			continue;

		output->correspondences[1 + num_inner + num_outer * inner + counts[inner]] = data[i].correspondences;

		output->atom_indices[1 + num_inner + num_outer * inner + counts[inner]] = nbr_atom_index;
		output->numbers[1 + num_inner + num_outer * inner + counts[inner]] = data[i].number;
		memcpy(output->points[1 + num_inner + num_outer * inner + counts[inner]], &data[i].delta, 3 * sizeof(double));
		claimed.insert(nbr_atom_index);

		counts[inner]++;
		num_found++;
		if (num_found >= num_inner * num_outer)
			break;
	}

	if (num_found != num_inner * num_outer)
		return -1;

	return 0;
}

}

