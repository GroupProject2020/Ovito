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


#include <core/Core.h>
#include "HalfEdgeMesh.h"

#include <boost/heap/fibonacci_heap.hpp>
#include <boost/optional.hpp>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Mesh)

/// Default implementation of a functor that calculates the vector between to spatial points.
struct DefaultPointPointVector 
{
	Vector3 operator()(const Point3& p0, const Point3& p1) const {
		return p1 - p0;
	}
};

/// Default implementation of a functor that validates an edge vector.
struct DefaultEdgeVectorValidation
{
	bool operator()(const Vector3& v) const {
		return true;
	}
};

/******************************************************************************
* Algorithm that reduces the number of faces/edges/vertices of a HalfEdgeMesh 
* structure using an edge collapse strategy.
*
* See "Fast and Memory Efficient Polygonal Simplification"
* by Peter Lindstrom and Greg Turk.
*
* This implementation only supports closed manifold meshes made of 
* triangle faces.
******************************************************************************/
template<class HEM, typename PointPointVector = DefaultPointPointVector, typename EdgeVectorValidation = DefaultEdgeVectorValidation>
class EdgeCollapseMeshSimplification
{
public:

	using Face = typename HEM::Face;
	using Edge = typename HEM::Edge;
	using Vertex = typename HEM::Vertex;

	/// Constructor.
	EdgeCollapseMeshSimplification(HEM& mesh, PointPointVector ppvec = PointPointVector(), EdgeVectorValidation edgeValidation = EdgeVectorValidation()) : 
		_mesh(mesh), _ppvec(std::move(ppvec)), _edgeValidation(std::move(edgeValidation)) {}

	bool perform(FloatType minEdgeLength, PromiseBase& promise) 
	{
		// Current implementation can only handle closed manifolds.
		OVITO_ASSERT(_mesh.isClosed());

		promise.beginProgressSubSteps(2);

		// First collect all candidate edges in a priority queue
		if(!collect(promise))
			return false;

		promise.nextProgressSubStep();

		// Then proceed to collapse each edge in turn.
		if(!loop(minEdgeLength, promise))
			return false;

		promise.endProgressSubSteps();

		return !promise.isCanceled();
	}

private:

	/// Used to decide whether a half-edge is the primary half-edge of a pair.
	bool edgeSense(const Vector3& v) const 
	{
		if(v.x() >= FLOATTYPE_EPSILON) return true;
		else if(v.x() <= -FLOATTYPE_EPSILON) return false;
		if(v.y() >= FLOATTYPE_EPSILON) return true;
		else if(v.y() <= -FLOATTYPE_EPSILON) return false;
		return v.z() > 0;
	}

	/// Collects all candidate edges.
	bool collect(PromiseBase& promise) 
	{
		promise.setProgressMaximum(_mesh.faceCount());

		// Insert edges into priority queue.
		_pqHandles.clear();
		for(Face* face : _mesh.faces()) {
			Edge* edge = face->edges();
			do {
				// Skip every other half-edge.
				if(edge->oppositeEdge() == nullptr || edgeSense(_ppvec(edge->vertex1()->pos(), edge->vertex2()->pos()))) {
					boost::optional<Vector3> placement = computePlacement(edge);
					FloatType cost = placement ? computeCost(edge, *placement) : -1;
					_pqHandles.insert(std::make_pair(edge, _pq.push({ edge, cost })));
				}
				edge = edge->nextFaceEdge();
			}
			while(edge != face->edges());

			if(!promise.incrementProgressValue())
				return false;
		}

		return true;
	}

	/// Collapses edges in order of priority.
	bool loop(FloatType minEdgeLength, PromiseBase& promise)
	{
		promise.setProgressMaximum(_pq.size());

		// Pop and processe each edge from the PQ.
		size_t collapsedEdges = 0;
		while(!_pq.empty()) {
			if(!promise.incrementProgressValue())
				return false;
			
			Edge* edge = _pq.top().edge;
			FloatType cost = _pq.top().cost;
			_pq.pop();
			_pqHandles.erase(edge);

			// Stopping criterion:
			if(_ppvec(edge->vertex1()->pos(), edge->vertex2()->pos()).squaredLength() > minEdgeLength*minEdgeLength)
				break;

			if(cost >= 0) {
				boost::optional<Vector3> placement = computePlacement(edge);
				if(placement && isCollapseTopologicallyValid(edge, edge->vertex2()->pos() + *placement)) {
					collapse(edge, *placement);
					collapsedEdges++;
				}
			}
		}
		promise.setProgressValue(promise.progressMaximum());

		// Remove faces which were marked for deletion.
		_mesh.removeMarkedFaces();

		// Remove faces and vertices which were marked for deletion.
		for(int v = _mesh.vertexCount() - 1; v >= 0; v--) {
			if(_mesh.vertices()[v]->numEdges() == 0)
				_mesh.removeVertex(v);
		}
		
		// Need to assign new indices to vertices some of the have been deleted.
		_mesh.reindexVerticesAndFaces();

		return true;
	}

	/// Calls a callback function on all faces which are adjacent on the two vertices of the given edge.
	template<typename Callback>
	void visitAdjacentTriangles(Edge* edge, Callback callback) const 
	{
		// Go around first vertex.
		OVITO_ASSERT(edge->oppositeEdge() && edge->oppositeEdge()->nextFaceEdge());
		Edge* currentEdge = edge;
		Edge* stopEdge = edge->oppositeEdge()->nextFaceEdge();
		do {
			callback(currentEdge);
			currentEdge = currentEdge->prevFaceEdge()->oppositeEdge();
			OVITO_ASSERT(currentEdge);
		}
		while(currentEdge != stopEdge);

		// Go around second vertex.
		edge = edge->oppositeEdge();
		OVITO_ASSERT(edge && edge->oppositeEdge()->nextFaceEdge());
		currentEdge = edge;
		stopEdge = edge->oppositeEdge()->nextFaceEdge();
		do {
			callback(currentEdge);
			currentEdge = currentEdge->prevFaceEdge()->oppositeEdge();
			OVITO_ASSERT(currentEdge);
		}
		while(currentEdge != stopEdge);
	}

	/// Calls a callback function on all vertices which are adjacent on the two vertices of the given edge.
	template<typename Callback>
	void visitLink(Edge* edge, Callback callback) const 
	{
		// Go around first vertex.
		OVITO_ASSERT(edge->oppositeEdge() && edge->oppositeEdge()->nextFaceEdge());
		Edge* currentEdge = edge->prevFaceEdge()->oppositeEdge();
		Edge* stopEdge = edge->oppositeEdge()->nextFaceEdge();
		do {
			callback(currentEdge->vertex2());
			currentEdge = currentEdge->prevFaceEdge()->oppositeEdge();
			OVITO_ASSERT(currentEdge);
		}
		while(currentEdge != stopEdge);

		// Go around second vertex.
		edge = edge->oppositeEdge();
		OVITO_ASSERT(edge && edge->oppositeEdge()->nextFaceEdge());
		currentEdge = edge->prevFaceEdge()->oppositeEdge();
		stopEdge = edge->oppositeEdge()->nextFaceEdge();
		do {
			callback(currentEdge->vertex2());
			currentEdge = currentEdge->prevFaceEdge()->oppositeEdge();
			OVITO_ASSERT(currentEdge);
		}
		while(currentEdge != stopEdge);
	}

	/// Each vertex constraint is an equation of the form: Ai * v = bi
	/// Where 'v' is a vector representing the vertex,
	/// 'Ai' is a (row) vector and 'bi' a scalar.
	///
	/// The vertex is completely determined by 3 such constraints, 
	/// so is the solution to the following system:
	///
	///  A.row(0) * v = b0
	///  A.row(1) * v = b1
	///  A.row(2) * v = b2
	///
	/// Which in matrix form is: A * v = b
	///
	/// (with 'A' a 3x3 matrix and 'b' a vector)
	///
	/// The member variables contain A and b. Individual constraints (Ai,bi) can be added to it.
	/// Once 3 such constraints have been added, 'v' is directly solved as:
	///
	///  v = inv(A) * b
	///
	/// A constraint (Ai,bi) must be alpha-compatible with the previously added constraints (see paper); otherwise, it's discarded.
	struct Constraints
	{
		int numConstraints = 0;
		Matrix3 constraintsA = Matrix3::Zero();
		Vector3 constraintsB = Vector3::Zero();

		void addConstraintIfAlphaCompatible(const Vector3& Ai, FloatType bi)
		{
			OVITO_ASSERT(numConstraints < 3);
			FloatType slai = Ai.squaredLength();
			if(slai == 0) return;
			FloatType l = std::sqrt(slai);
			
			Vector3 Ain = Ai / l;
			FloatType bin = bi / l;
			if(numConstraints == 1) {
				FloatType d01 = constraintsA.column(0).dot(Ai);
				FloatType sla0 = constraintsA.column(0).squaredLength();
				FloatType sd01 = d01 * d01;
				FloatType max = sla0 * slai * mcMaxDihedralAngleCos2;
				if(sd01 > max)
					return;
			}
			else if(numConstraints == 2) {
				Vector3 N = constraintsA.column(0).cross(constraintsA.column(1));			
				FloatType dc012 = N.dot(Ai);
				FloatType slc01  = N.squaredLength();
				FloatType sdc012 = dc012 * dc012;		
				FloatType min = slc01 * slai * mcMaxDihedralAngleSin2;
				if(sdc012 <= min)
					return;
			}
					
			constraintsA.column(numConstraints) = Ain;
			constraintsB[numConstraints] = bin;
			++numConstraints;
		}
		
		void addConstraintFromGradient(const Matrix3& H, const Vector3& c) 
		{
			OVITO_ASSERT(numConstraints >= 0 && numConstraints < 3);
			switch(numConstraints)
			{
			case 0:
				addConstraintIfAlphaCompatible(H.column(0), -c.x());
				addConstraintIfAlphaCompatible(H.column(1), -c.y());
				addConstraintIfAlphaCompatible(H.column(2), -c.z());
				break;  
      
			case 1:
				{
				const Vector3& A0 = constraintsA.column(0);
				OVITO_ASSERT(A0 != Vector3::Zero());
        
				Vector3 AbsA0(std::abs(A0.x()), std::abs(A0.y()), std::abs(A0.z()));
           
				Vector3 Q0;      
				switch(AbsA0.maxComponent())
				{
				// Since A0 is guaranteed to be non-zero, the denominators here are known to be non-zero too.
				case 0: Q0 = Vector3(-A0.z()/A0.x(),0             ,1             ); break;
				case 1: Q0 = Vector3(0             ,-A0.z()/A0.y(),1             ); break;
				case 2: Q0 = Vector3(1             ,0             ,-A0.x()/A0.z()); break;
				default: Q0.setZero(); // This should never happen!
		        }
				Vector3 Q1 = A0.cross(Q0);
		        Vector3 A1 = H * Q0;
        		Vector3 A2 = H * Q1;
        
				FloatType b1 = -Q0.dot(c);
				FloatType b2 = -Q1.dot(c);
        
        		addConstraintIfAlphaCompatible(A1,b1);
        		addConstraintIfAlphaCompatible(A2,b2);
				}
				break;
    
			case 2:
				{
				Vector3 Q = constraintsA.column(0).cross(constraintsA.column(1));
				Vector3 A2 = H * Q;
				FloatType b2 = -Q.dot(c);
				addConstraintIfAlphaCompatible(A2,b2);
				}
				break;
			}
		}

		boost::optional<Vector3> solve() const 
		{
			// It might happen that there were not enough alpha-compatible constraints.
			// In that case there is simply no good vertex placement
			if(numConstraints != 3) return {};
			
			// If the matrix is singular it's inverse cannot be computed so an 'absent' value is returned.
			Matrix3 inverseA;
			if(!constraintsA.inverse(inverseA)) return {};
			
			return inverseA.transposed() * constraintsB;
		}
	};


	boost::optional<Vector3> computePlacement(Edge* edge) const 
	{
		Constraints constraints;
		addVolumePreservationConstraints(edge, constraints);
		
		if(constraints.numConstraints < 3)
			addBoundaryAndVolumeOptimizationConstraints(edge, constraints); 
		
		if(constraints.numConstraints < 3)
			addShapeOptimizationConstraints(edge, constraints);
		
		return constraints.solve();
	}

	void addVolumePreservationConstraints(Edge* edge, Constraints& constraints) const
	{
		Vector3 sumV = Vector3::Zero();
		FloatType sumL = 0;
		const Point3& origin = edge->vertex2()->pos();
		visitAdjacentTriangles(edge, [this,&sumV,&sumL,&origin](Edge* faceEdge) {
			const Point3& p0 = faceEdge->prevFaceEdge()->vertex1()->pos();
			const Point3& p1 = faceEdge->vertex1()->pos();
			const Point3& p2 = faceEdge->vertex2()->pos();
			Vector3 v01 = _ppvec(p0, p1);
			Vector3 v02 = _ppvec(p0, p2);
			Vector3 normalV = v01.cross(v02);
			// Determinant:
			FloatType normalL = _ppvec(origin, p0).cross(_ppvec(origin, p1)).dot(_ppvec(origin, p2));
			sumV += normalV;
			sumL += normalL;
		});
		constraints.addConstraintIfAlphaCompatible(sumV, sumL);
	}

	void addBoundaryAndVolumeOptimizationConstraints(Edge* edge, Constraints& constraints) const
	{
		Matrix3 H = Matrix3::Zero();
		Vector3 c = Vector3::Zero();
		const Point3& origin = edge->vertex2()->pos();
  		visitAdjacentTriangles(edge, [this,&H,&c,&origin](Edge* faceEdge) {
			const Point3& p0 = faceEdge->prevFaceEdge()->vertex1()->pos();
			const Point3& p1 = faceEdge->vertex1()->pos();
			const Point3& p2 = faceEdge->vertex2()->pos();
			Vector3 v01 = _ppvec(p0, p1);
			Vector3 v02 = _ppvec(p0, p2);
			Vector3 normalV = v01.cross(v02);
			// Determinant:
			FloatType normalL = _ppvec(origin, p0).cross(_ppvec(origin, p1)).dot(_ppvec(origin, p2));
			H.column(0) += normalV * normalV.x();
			H.column(1) += normalV * normalV.y();
			H.column(2) += normalV * normalV.z();
			c -= normalL * normalV;
		});
		constraints.addConstraintFromGradient(H,c);
	}
	
	void addShapeOptimizationConstraints(Edge* edge, Constraints& constraints) const
	{
		Vector3 c = Vector3::Zero();
  		int linkCount = 0;
		const Point3& origin = edge->vertex2()->pos();
		visitLink(edge, [this,&c,&linkCount,&origin](Vertex* v) {
			c -= _ppvec(origin, v->pos());
			linkCount++;
		});
  		FloatType s = linkCount;
  		Matrix3 H(s,0,0,0,s,0,0,0,s);
		constraints.addConstraintFromGradient(H,c);
	}
	
	/// Computes the costs associated with collapsing the given edge into a single vertex at relative position v.
	FloatType computeCost(Edge* edge, const Vector3& v) const
	{
		const Point3& p0 = edge->vertex1()->pos();
		const Point3& p1 = edge->vertex2()->pos();
    	FloatType squaredLength = _ppvec(p0,  p1).squaredLength();
		FloatType volumeCost = computeVolumeCost(edge, v);
		FloatType shapeCost = computeShapeCost(edge, p1 + v);
    
		FloatType totalCost = _volumeWeight * volumeCost
		                    + _shapeWeight * shapeCost * squaredLength * squaredLength;
    
		OVITO_ASSERT(totalCost >= 0);
    	return std::isfinite(totalCost) ? totalCost : FloatType(-1);
	}

	FloatType computeVolumeCost(Edge* edge, const Vector3& v) const
	{
		FloatType cost = 0;
		const Point3& origin = edge->vertex2()->pos();
		visitAdjacentTriangles(edge, [this,&cost,&v,&origin](Edge* faceEdge) {
			const Point3& p0 = faceEdge->prevFaceEdge()->vertex1()->pos();
			const Point3& p1 = faceEdge->vertex1()->pos();
			const Point3& p2 = faceEdge->vertex2()->pos();
			Vector3 v01 = _ppvec(p0, p1);
			Vector3 v02 = _ppvec(p0, p2);
			Vector3 normalV = v01.cross(v02);
			// Determinant:
			FloatType normalL = _ppvec(origin, p0).cross(_ppvec(origin, p1)).dot(_ppvec(origin, p2));
			FloatType f = normalV.dot(v) - normalL;
			cost += f*f;
		});
		return cost / 36;
	}
	
	FloatType computeShapeCost(Edge* edge, const Point3& p) const
	{
		FloatType cost = 0;
		visitLink(edge, [this,&p,&cost](Vertex* v) {
			cost += _ppvec(p, v->pos()).squaredLength();
		});
		return cost;
	}	

	/// A collapse is geometrically valid if in the resulting local mesh no two adjacent triangles form an internal dihedral angle
	/// greater than a fixed threshold (i.e. triangles do not "fold" into each other).
	bool isCollapseTopologicallyValid(Edge* edge, const Point3& k0) const 
	{
		// Go around first vertex.
		OVITO_ASSERT(edge->nextFaceEdge()->oppositeEdge());
		OVITO_ASSERT(edge->prevFaceEdge()->oppositeEdge());
		if(!checkLinkTriangles(k0, edge->nextFaceEdge()->oppositeEdge()->prevFaceEdge(), edge->prevFaceEdge()->oppositeEdge()->nextFaceEdge()))
			return false;
		OVITO_ASSERT(edge->oppositeEdge() && edge->oppositeEdge()->nextFaceEdge());
		OVITO_ASSERT(edge->vertex1()->numEdges() >= 3);
		Edge* currentEdge = edge->prevFaceEdge()->oppositeEdge();
		Edge* stopEdge = edge->oppositeEdge()->nextFaceEdge();
		OVITO_ASSERT(stopEdge->oppositeEdge());
		stopEdge = stopEdge->oppositeEdge()->nextFaceEdge();
		while(currentEdge != stopEdge) {
			OVITO_ASSERT(currentEdge);
			Edge* nextEdge = currentEdge->prevFaceEdge()->oppositeEdge();
			OVITO_ASSERT(nextEdge);
			if(!checkLinkTriangles(k0, currentEdge->nextFaceEdge(), nextEdge->nextFaceEdge()))
				return false;
			currentEdge = nextEdge;
		}

		// Go around second vertex.
		edge = edge->oppositeEdge();
		OVITO_ASSERT(edge->nextFaceEdge()->oppositeEdge());
		OVITO_ASSERT(edge->prevFaceEdge()->oppositeEdge());
		if(!checkLinkTriangles(k0, edge->nextFaceEdge()->oppositeEdge()->prevFaceEdge(), edge->prevFaceEdge()->oppositeEdge()->nextFaceEdge()))
			return false;
		OVITO_ASSERT(edge->oppositeEdge() && edge->oppositeEdge()->nextFaceEdge());
		OVITO_ASSERT(edge->vertex1()->numEdges() >= 3);
		currentEdge = edge->prevFaceEdge()->oppositeEdge();
		stopEdge = edge->oppositeEdge()->nextFaceEdge();
		OVITO_ASSERT(stopEdge->oppositeEdge());
		stopEdge = stopEdge->oppositeEdge()->nextFaceEdge();
		while(currentEdge != stopEdge) {
			OVITO_ASSERT(currentEdge);
			Edge* nextEdge = currentEdge->prevFaceEdge()->oppositeEdge();
			OVITO_ASSERT(nextEdge);
			if(!checkLinkTriangles(k0, currentEdge->nextFaceEdge(), nextEdge->nextFaceEdge()))
				return false;
			currentEdge = nextEdge;
		}

		return true;
	}

	/// Performs the geometric validity test for two consecutive edges along the link of the collapsing edge.
	bool checkLinkTriangles(const Point3& k0, Edge* e12, Edge* e23) const 
	{
		OVITO_ASSERT(e12->vertex2() == e23->vertex1());
		
		if(!areSharedTrianglesValid(k0, e12->vertex1()->pos(), e12->vertex2()->pos(), e23->vertex2()->pos()))
			return false;

		if(e12->oppositeEdge()) {
			OVITO_ASSERT(e12->oppositeEdge()->face());
			if(!areSharedTrianglesValid(e12->vertex1()->pos(), e12->oppositeEdge()->nextFaceEdge()->vertex2()->pos(), e12->vertex2()->pos(), k0))
				return false;
		}

		return true;
	}	

	/// Given triangles 'p0,p1,p2' and 'p0,p2,p3', both shared along edge 'v0-v2',
	/// determine if they are geometrically valid: that is, the ratio of their
	/// respective areas is no greater than a max value and the internal
	/// dihedral angle formed by their supporting planes is no greater than
	/// a given threshold
	bool areSharedTrianglesValid(const Point3& p0, const Point3& p1, const Point3& p2, const Point3& p3) const
	{
		Vector3 e01 = _ppvec(p0, p1);
		Vector3 e02 = _ppvec(p0, p2);
		Vector3 e03 = _ppvec(p0, p3);
		if(!_edgeValidation(e01)) return false;
		if(!_edgeValidation(e02)) return false;
		if(!_edgeValidation(e03)) return false;

		Vector3 n012 = e01.cross(e02);
		Vector3 n023 = e02.cross(e03);

		FloatType l012 = n012.dot(n012);
		FloatType l023 = n023.dot(n023);

		FloatType larger  = std::max(l012,l023);
		FloatType smaller = std::min(l012,l023);

		if(larger < cMaxAreaRatio * smaller) {
    		FloatType l0123 = n012.dot(n023);
			if(l0123 > 0)
				return true;
			if(l0123 * l0123 <= mcMaxDihedralAngleCos2 * l012 * l023)
				return true;
		}
		return false;
  	}

	/// Removes the given edge from the mesh and updates the neighborhood.
	void collapse(Edge* edge, const Vector3& placement)
	{
		Edge* oppositeEdge = edge->oppositeEdge();
		OVITO_ASSERT(oppositeEdge);

		// Reposition vertex.
		Vertex* remainingVertex = edge->vertex2();
		remainingVertex->setPos(remainingVertex->pos() + placement);

		Edge* ep1 = edge->prevFaceEdge();
		Edge* epo1 = ep1->oppositeEdge();
		Edge* en1 = edge->nextFaceEdge();
		Edge* eno1 = en1->oppositeEdge();
		Edge* ep2 = oppositeEdge->prevFaceEdge();
		Edge* epo2 = ep2->oppositeEdge();
		Edge* en2 = oppositeEdge->nextFaceEdge();
		Edge* eno2 = en2->oppositeEdge();

		eraseEdgeFromPQ(ep1);
		eraseEdgeFromPQ(en2);
		_mesh.joinFaces(ep1);
		_mesh.joinFaces(en2);
		_mesh.collapseEdge(edge);
		
		// Update priority queue and costs of all affected edges.
		Edge* currentEdge = en1;
		do {
			OVITO_ASSERT(currentEdge->vertex1() == remainingVertex);
			updateEdgeCostIfPrimary(currentEdge);
			Edge* currentEdge2 = currentEdge->nextFaceEdge();
			do {
				updateEdgeCostIfPrimary(currentEdge2);
				currentEdge2 = currentEdge2->prevFaceEdge()->oppositeEdge();
				OVITO_ASSERT(currentEdge2);
			}
			while(currentEdge2 != currentEdge->nextFaceEdge());
			currentEdge = currentEdge->prevFaceEdge()->oppositeEdge();
			OVITO_ASSERT(currentEdge);
		}
		while(currentEdge != en1);
	}

	void eraseEdgeFromPQ(Edge* edge) 
	{
		auto handle = _pqHandles.find(edge);
		if(handle == _pqHandles.end()) {
			OVITO_ASSERT(edge->oppositeEdge() != nullptr);
			handle = _pqHandles.find(edge->oppositeEdge());
		}
		if(handle != _pqHandles.end()) {
			_pq.erase(handle->second);
			_pqHandles.erase(handle);
		}
	}

	void updateEdgeCostIfPrimary(Edge* edge) 
	{
		auto handle = _pqHandles.find(edge);
		if(handle == _pqHandles.end())
			return;
		boost::optional<Vector3> placement = computePlacement(edge);
		(*handle->second).cost = placement ? computeCost(edge, *placement) : -1;
		_pq.update(handle->second);
	}

private:

	/// Data type to be stored in priority queue.
	struct EdgeWithCost {
		Edge* edge;
		FloatType cost;
		bool operator<(const EdgeWithCost& other) const { return cost > other.cost; }
	};

	/// The mutable priority queue type.
	using PQ = boost::heap::fibonacci_heap<EdgeWithCost,boost::heap::mutable_<true>>;

	/// The mesh this algorithm operates on.
	HEM& _mesh;

	/// The priority queue of edges.
	PQ _pq;

	/// Loopup map for handles in the priority queue.
	std::unordered_map<Edge*, typename PQ::handle_type> _pqHandles;
	
	/// Functor object that is responsible for calculating the vector between two points.
	PointPointVector _ppvec;
	
	/// Client-provided functor object that allow to reject new edges. 
	EdgeVectorValidation _edgeValidation;

	// Lindstrom-Turk algorithm parameters:
	FloatType _volumeWeight = FloatType(0.5);
	FloatType _boundaryWeight = FloatType(0.5);
	FloatType _shapeWeight = FloatType(0);

	static constexpr FloatType cMaxAreaRatio = FloatType(1e8);
	static constexpr double cMaxDihedralAngleCos = 0.99984769515639123916; // =cos(1 degree)
	static constexpr FloatType mcMaxDihedralAngleCos2 = FloatType(0.999695413509547865); // =cos(1 degree)^2
	static constexpr double cMaxDihedralAngleSin = 0.017452406437284; // =sin(1 degree)
	static constexpr FloatType mcMaxDihedralAngleSin2 = FloatType(0.000304586490452); // =sin(1 degree)^2
};


OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


