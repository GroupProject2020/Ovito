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

#include <ovito/particles/Particles.h>
#include <ovito/core/utilities/concurrent/Task.h>
#include "NearestNeighborFinder.h"

namespace Ovito { namespace Particles {

#define TREE_DEPTH_LIMIT 		17

/******************************************************************************
* Prepares the neighbor list builder.
******************************************************************************/
bool NearestNeighborFinder::prepare(ConstPropertyAccess<Point3> posProperty, const SimulationCell& cellData, ConstPropertyAccess<int> selectionProperty, Task* promise)
{
	OVITO_ASSERT(posProperty);
	if(promise) promise->setProgressMaximum(0);

	simCell = cellData;

	// Automatically disable PBCs in Z direction for 2D systems.
	if(simCell.is2D()) {
		simCell.setPbcFlags(simCell.pbcFlags()[0], simCell.pbcFlags()[1], false);
		AffineTransformation matrix = simCell.matrix();
		matrix.column(2) = Vector3(0, 0, 0.01f);
		simCell.setMatrix(matrix);
	}

	if(simCell.volume3D() <= FLOATTYPE_EPSILON)
		throw Exception("Simulation cell is degenerate.");

	// Compute normal vectors of simulation cell faces.
	planeNormals[0] = simCell.cellNormalVector(0);
	planeNormals[1] = simCell.cellNormalVector(1);
	planeNormals[2] = simCell.cellNormalVector(2);

	// Create list of periodic image shift vectors.
	int nx = simCell.pbcFlags()[0] ? 1 : 0;
	int ny = simCell.pbcFlags()[1] ? 1 : 0;
	int nz = simCell.pbcFlags()[2] ? 1 : 0;
	for(int iz = -nz; iz <= nz; iz++) {
		for(int iy = -ny; iy <= ny; iy++) {
			for(int ix = -nx; ix <= nx; ix++) {
				pbcImages.push_back(simCell.matrix() * Vector3(ix,iy,iz));
			}
		}
	}
	// Sort PBC images by distance from the master image.
	std::sort(pbcImages.begin(), pbcImages.end(), [](const Vector3& a, const Vector3& b) {
		return a.squaredLength() < b.squaredLength();
	});

	// Compute bounding box of all particles (only for non-periodic directions).
	Box3 boundingBox(Point3(0,0,0), Point3(1,1,1));
	if(simCell.pbcFlags()[0] == false || simCell.pbcFlags()[1] == false || simCell.pbcFlags()[2] == false) {
		for(const Point3& p : posProperty) {
			Point3 reducedp = simCell.absoluteToReduced(p);
			if(simCell.pbcFlags()[0] == false) {
				if(reducedp.x() < boundingBox.minc.x()) boundingBox.minc.x() = reducedp.x();
				else if(reducedp.x() > boundingBox.maxc.x()) boundingBox.maxc.x() = reducedp.x();
			}
			if(simCell.pbcFlags()[1] == false) {
				if(reducedp.y() < boundingBox.minc.y()) boundingBox.minc.y() = reducedp.y();
				else if(reducedp.y() > boundingBox.maxc.y()) boundingBox.maxc.y() = reducedp.y();
			}
			if(simCell.pbcFlags()[2] == false) {
				if(reducedp.z() < boundingBox.minc.z()) boundingBox.minc.z() = reducedp.z();
				else if(reducedp.z() > boundingBox.maxc.z()) boundingBox.maxc.z() = reducedp.z();
			}
		}
	}

	// Create root node.
	root = nodePool.construct();
	root->bounds = boundingBox;
	numLeafNodes++;

	// Create first level of child nodes by splitting in X direction.
	splitLeafNode(root, 0);

	// Create second level of child nodes by splitting in Y direction.
	splitLeafNode(root->children[0], 1);
	splitLeafNode(root->children[1], 1);

	// Create third level of child nodes by splitting in Z direction.
	splitLeafNode(root->children[0]->children[0], 2);
	splitLeafNode(root->children[0]->children[1], 2);
	splitLeafNode(root->children[1]->children[0], 2);
	splitLeafNode(root->children[1]->children[1], 2);

	// Insert particles into tree structure. Refine tree as needed.
	const Point3* p = posProperty.cbegin();
	const int* sel = selectionProperty ? selectionProperty.cbegin() : nullptr;
	atoms.resize(posProperty.size());
	for(NeighborListAtom& a : atoms) {
		if(promise && promise->isCanceled())
			return false;
		a.pos = *p;
		// Wrap atomic positions back into simulation box.
		Point3 rp = simCell.absoluteToReduced(a.pos);
		for(size_t k = 0; k < 3; k++) {
			if(simCell.pbcFlags()[k]) {
				if(FloatType s = std::floor(rp[k])) {
					rp[k] -= s;
					a.pos -= s * simCell.matrix().column(k);
				}
			}
		}
		if(!sel || *sel++) {
			insertParticle(&a, rp, root, 0);
		}
		++p;
	}

	root->convertToAbsoluteCoordinates(simCell);

	return true;
}

/******************************************************************************
* Inserts an atom into the binary tree.
******************************************************************************/
void NearestNeighborFinder::insertParticle(NeighborListAtom* atom, const Point3& p, TreeNode* node, int depth)
{
	if(node->isLeaf()) {
		OVITO_ASSERT(node->bounds.classifyPoint(p) != -1);
		// Insert atom into leaf node.
		atom->nextInBin = node->atoms;
		node->atoms = atom;
		node->numAtoms++;
		if(depth > maxTreeDepth) maxTreeDepth = depth;
		// If leaf node becomes too large, split it in the largest dimension.
		if(node->numAtoms > bucketSize && depth < TREE_DEPTH_LIMIT) {
			splitLeafNode(node, determineSplitDirection(node));
		}
	}
	else {
		// Decide on which side of the splitting plane the atom is located.
		if(p[node->splitDim] < node->splitPos)
			insertParticle(atom, p, node->children[0], depth+1);
		else
			insertParticle(atom, p, node->children[1], depth+1);
	}
}

/******************************************************************************
* Determines in which direction to split the given leaf node.
******************************************************************************/
int NearestNeighborFinder::determineSplitDirection(TreeNode* node)
{
	FloatType dmax = 0.0;
	int dmax_dim = -1;
	for(int dim = 0; dim < 3; dim++) {
		FloatType d = simCell.matrix().column(dim).squaredLength() * node->bounds.size(dim) * node->bounds.size(dim);
		if(d > dmax) {
			dmax = d;
			dmax_dim = dim;
		}
	}
	OVITO_ASSERT(dmax_dim >= 0);
	return dmax_dim;
}

/******************************************************************************
* Splits a leaf node into two new leaf nodes and redistributes the atoms to the child nodes.
******************************************************************************/
void NearestNeighborFinder::splitLeafNode(TreeNode* node, int splitDim)
{
	NeighborListAtom* atom = node->atoms;

	node->splitDim = splitDim;
	node->splitPos = (node->bounds.minc[splitDim] + node->bounds.maxc[splitDim]) * 0.5;

	// Create child nodes and define their bounding boxes.
	node->children[0] = nodePool.construct();
	node->children[1] = nodePool.construct();
	node->children[0]->bounds = node->bounds;
	node->children[1]->bounds = node->bounds;
	node->children[0]->bounds.maxc[splitDim] = node->children[1]->bounds.minc[splitDim] = node->splitPos;

	// Redistribute atoms to child nodes.
	while(atom != nullptr) {
		NeighborListAtom* next = atom->nextInBin;
		FloatType p = simCell.inverseMatrix().prodrow(atom->pos, splitDim);
		if(p < node->splitPos) {
			atom->nextInBin = node->children[0]->atoms;
			node->children[0]->atoms = atom;
		}
		else {
			atom->nextInBin = node->children[1]->atoms;
			node->children[1]->atoms = atom;
		}
		atom = next;
	}

	numLeafNodes++;
}

}	// End of namespace
}	// End of namespace
