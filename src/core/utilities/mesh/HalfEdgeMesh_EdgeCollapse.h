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
#include <fstream>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Mesh)

/******************************************************************************
* Algorithm that reduces the number of faces/edges/vertices of a HalfEdgeMesh 
* structure using an edge collapse strategy.
*
* "Fast and Memory Efficient Polygonal Symplification", 
* by Peter Lindstrom and Greg Turk
******************************************************************************/
template<class HEM>
class EdgeCollapseMeshSimplification
{
public:

	using Face = typename HEM::Face;
	using Edge = typename HEM::Edge;
	using Vertex = typename HEM::Vertex;

	/// Constructor.
	EdgeCollapseMeshSimplification(HEM& mesh) : _mesh(mesh) {}

	void perform() 
	{
		// Current implementation can only handle closed manifolds.
		OVITO_ASSERT(_mesh.isClosed());

		// First collect all candidate edges in a priority queue
		collect();

		// Then proceed to collapse each edge in turn.
		loop();
	}

private:

	class Profile
	{
	public:
		/// Constructor.
		Profile(Edge* edge) : _V0V1(edge), _V1V0(edge->oppositeEdge()) {
			OVITO_ASSERT(_V0V1->face() != nullptr);
			OVITO_ASSERT(_V1V0 == nullptr || _V1V0->face() != nullptr);
		}

		bool is_v0_v1_a_border() const { return _V0V1 == nullptr; }
		bool is_v1_v0_a_border() const { return _V1V0 == nullptr; }  
		bool left_face_exists () const { return _V0V1 != nullptr; }
		bool right_face_exists() const { return _V1V0 != nullptr; }

		const Point3& p0() const { OVITO_ASSERT(_V1V0); return _V1V0->vertex2()->pos(); }
		const Point3& p1() const { OVITO_ASSERT(_V0V1); return _V0V1->vertex2()->pos(); }

		Edge* edge() const { return _V0V1; }

	private:

		Edge* _V0V1;
		Edge* _V1V0;
	};

	/// Collects all candidate edges.
	void collect() 
	{
		// Insert edges into priority queue.
		_pqHandles.clear();
		for(Face* face : _mesh.faces()) {
			Edge* edge = face->edges();
			do {
				// Skip every other halfedge.
				if(edge->oppositeEdge() == nullptr || edge->vertex2()->index() >= edge->oppositeEdge()->vertex2()->index()) {
					Point3 placement = computePlacement(edge);
					FloatType cost = computeCost(edge, placement);
					_pqHandles.insert(std::make_pair(edge, _pq.push({ edge, cost })));
				}
				edge = edge->nextFaceEdge();
			}
			while(edge != face->edges());
		}

		std::ofstream s("edge_collapse.data");
		s << "LAMMPS data file\n";
		s << "\n";
		s << _mesh.vertexCount() << " atoms\n";
		s << _pq.size() << " bonds\n";
		s << "\n";
		s << "1 atom types\n";
		s << "1 bond types\n";
		s << "\n";
		Box3 bbox;
		for(Vertex* v : _mesh.vertices())
			bbox.addPoint(v->pos());
		s << bbox.minc[0] << " " << bbox.maxc[0] << "xlo xhi\n";
		s << bbox.minc[1] << " " << bbox.maxc[1] << "ylo yhi\n";
		s << bbox.minc[2] << " " << bbox.maxc[2] << "zlo zhi\n";
		s << "\nMasses\n\n";
		s << "1 1\n";
		s << "\nAtoms # bond\n\n";
		for(Vertex* v : _mesh.vertices())
			s << v->index() << " 1 1 " << v->pos()[0] << " " << v->pos()[1] << " " << v->pos()[2] << "\n";
		s << "\nBonds\n\n";
		size_t counter = 1;
		for(const EdgeWithCost& e : _pq) {
			s << counter++ << " 1 " << e.edge->vertex1()->index() << " " << e.edge->vertex2()->index() << "\n";
		}
	}

	/// Collapses edges in order of priority.
	void loop()
	{
		// Pop and processe each edge from the PQ.
		size_t collapsedEdges = 0;
		while(!_pq.empty()) {
			Edge* edge = _pq.top().edge;
			FloatType cost = _pq.top().cost;
			_pq.pop();
			_pqHandles.erase(edge);

			if(cost >= 0) {
				Point3 placement = computePlacement(edge);

#if 0
				for(int f = _mesh.faceCount() - 1; f >= 0; f--) {
					if(_mesh.faces()[f]->edges() == nullptr)
						_mesh.removeFace(f);
				}
				for(int v = _mesh.vertexCount() - 1; v >= 0; v--) {
					if(_mesh.vertices()[v]->numEdges() == 0)
						_mesh.removeVertex(v);
				}
				_mesh.reindexVerticesAndFaces();
#endif				
				
				qDebug() << collapsedEdges << "Checking edge " << edge->vertex1()->index() << "-" << edge->vertex2()->index() << ": cost=" << cost;
				if(isCollapseTopologicallyValid(edge, placement)) {
					collapse(edge, placement);
					collapsedEdges++;
				}
				else {
					qDebug() << "collapse not possible";
				}
				if(collapsedEdges == 315) break;
			}
		}

		// Remove faces and vertices which were marked for deletion.
		int oldFaceCount = _mesh.faceCount();
		int oldVertexCount = _mesh.vertexCount();
		for(int f = _mesh.faceCount() - 1; f >= 0; f--) {
			if(_mesh.faces()[f]->edges() == nullptr)
				_mesh.removeFace(f);
		}
		for(int v = _mesh.vertexCount() - 1; v >= 0; v--) {
			if(_mesh.vertices()[v]->numEdges() == 0)
				_mesh.removeVertex(v);
		}
		_mesh.reindexVerticesAndFaces();
		qDebug() << "Deleted" << (oldFaceCount - _mesh.faceCount()) << "faces";
		qDebug() << "Deleted" << (oldVertexCount - _mesh.vertexCount()) << "vertices";

		std::ofstream s("edge_collapse_result.data");
		s << "LAMMPS data file\n";
		s << "\n";
		s << _mesh.vertexCount() << " atoms\n";
		s << (_mesh.faceCount() * 3 / 2) << " bonds\n";
		s << "\n";
		s << "1 atom types\n";
		s << "1 bond types\n";
		s << "\n";
		Box3 bbox;
		for(Vertex* v : _mesh.vertices())
			bbox.addPoint(v->pos());
		bbox = bbox.padBox(10.0);
		s << bbox.minc[0] << " " << bbox.maxc[0] << "xlo xhi\n";
		s << bbox.minc[1] << " " << bbox.maxc[1] << "ylo yhi\n";
		s << bbox.minc[2] << " " << bbox.maxc[2] << "zlo zhi\n";
		s << "\nMasses\n\n";
		s << "1 1\n";
		s << "\nAtoms # bond\n\n";
		for(Vertex* v : _mesh.vertices())
			s << v->index() << " 1 1 " << v->pos()[0] << " " << v->pos()[1] << " " << v->pos()[2] << "\n";
		s << "\nBonds\n\n";
		size_t counter = 1;
		for(Face* face : _mesh.faces()) {
			Edge* e = face->edges();
			OVITO_ASSERT(e);
			do {
				if(e->vertex2()->index() > e->vertex1()->index()) {
					s << counter++ << " 1 " << e->vertex1()->index() << " " << e->vertex2()->index() << "\n";
				}
				e = e->nextFaceEdge();
			}
			while(e != face->edges());
		}
		qDebug() << "Done";
	}

	Point3 computePlacement(Edge* edge) const 
	{
#if 0		
		const Point3& p0 = edge->vertex1()->pos();
		const Point3& p1 = edge->vertex2()->pos();
		return p0 + (p1 - p0) * FloatType(0.5);
#else 
		Constraints constraints;
		addVolumePreservationConstraints(edge, constraints);

		// It might happen that there were not enough alpha-compatible constraints.
		// In that case there is simply no good vertex placement
  		if ( mConstraints_n == 3 ) 
  {
    // If the matrix is singular it's inverse cannot be computed so an 'absent' value is returned.
    optional<Matrix> lOptional_Ai = inverse_matrix(mConstraints_A);
    if ( lOptional_Ai )
    {
      Matrix const& lAi = *lOptional_Ai ;
      
      CGAL_ECMS_LT_TRACE(2,"       b: " << xyz_to_string(mConstraints_b) );
      CGAL_ECMS_LT_TRACE(2,"  inv(A): " << matrix_to_string(lAi) );
      
      lPlacementV = filter_infinity(mConstraints_b * lAi) ;
      
      CGAL_ECMS_LT_TRACE(0,"  New vertex point: " << xyz_to_string(*lPlacementV) );
    }
    else
    {
      CGAL_ECMS_LT_TRACE(1,"  Can't solve optimization, singular system.");
    }  
  }
  else
  {
    CGAL_ECMS_LT_TRACE(1,"  Can't solve optimization, not enough alpha-compatible constraints.");
  }  
  
  if ( lPlacementV )
    rPlacementP = ORIGIN + (*lPlacementV) ;
    
  return rPlacementP;		
#endif		
	}

	int addVolumePreservationConstraints(Edge& edge, Constraints& constraints)
	{
		Vector3 sumV = Vector3::Zero();
		FloatType sumL = 0;
		visitAdjacentTriangles(edge, [&sumV,&sumL](Face* face) {
			Edge* fedge0 = face->edges();
			Edge* fedge1 = fedge0->nextFaceEdge();
			Edge* fedge2 = fedge1->nextFaceEdge();
			const Point3& p0 = fedge0->vertex2()->pos();
			const Point3& p1 = fedge1->vertex2()->pos();
			const Point3& p2 = fedge2->vertex2()->pos();
			Vector3 v01 = p1 - p0;
			Vector3 v02 = p2 - p0;
			Vector3 normalV = v01.cross(v02);
			FloatType normalL = (p0 - Point3::Origin()).cross(p1 - Point3::Origin()).dot(p2 - Point3::Origin());
			sumV += normalV;
			sumL += normalL;
		});		
		constraints.addConstraintIfAlphaCompatible(sumV, sumL);
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
	///  v = b*inverse(A)
	///
	/// A constraint (Ai,bi) must be alpha-compatible with the previously added constraints (see paper); otherwise, it's discarded.
	struct Constraints
	{
		int numConstraints = 0;
		Matrix3 constraintsA = Matrix3::Zero();
		Vector3 constraintsB = Vector3::Zero();

		void addConstraintIfAlphaCompatible(const Vector3& Ai, FloatType Bi)
		{
			FloatType slai = Ai.dot(Ai);
			if(std::isfinite(slai)) {
				FloatType l = std::sqrt(slai);
				if(l != 0) {
					Vector3 Ain = Ai / l;
					FloatType bin = bi / l;
					bool addIt = true;
					if(numConstraints == 1) {
						FloatType d01 = constraintsA.column(0).dot(Ai);
						FloatType sla0 = constraintsA.column(0).dot(constraintsA.column(0));
						FloatType sd01 = d01 * d01;
						FloatType max = sla0 * slai * _squared_cos_alpha;
						if(sd01 > max)
							addIt = false;
					}
					else if(numConstraints == 2) {
						Vector3 N = constraintsA.column(0).cross(constraintsA.column(1));			
						FloatType dc012 = N.dot(Ai);
						FloatType slc01  = N.dot(N);
						FloatType sdc012 = dc012 * dc012;		
						FloatType min = slc01 * slai * _squared_sin_alpha;
						if(sdc012 <= min)
							addIt = false;
					}
					
					if(addIt) {
						switch(numConstraints)
						{
						case 0:
							constraintsA.column(0) = Ain;
							constraintsB = Vector3(bin,constraintsB.y(),ConstraintsB.z());
							break;
						case 1:
							constraintsA.column(1) = Ain;
							constraintsB = Vector3(constraintsB.x(),bin,constraintsB.z());
							break;
						case 2:
							constraintsA.column(2) = Ain;
							constraintsB = Vector3(constraintsB.x(),constraintsB.y(),bin);
							break;
						}
			
						++numConstraints;			
					}
				}
			}
		}

		bool solve(Vector3& solution) const 
		{

		}
	};

	/// Computes the costs associated with collapsing the given edge into a single vertex at position p.
	FloatType computeCost(Edge* edge, const Point3& p) const
	{
		const Point3& p0 = edge->vertex1()->pos();
		const Point3& p1 = edge->vertex2()->pos();
		Vector3 v = p - Point3::Origin();
    	FloatType squaredLength = (p0 - p1).squaredLength();
    	FloatType bdryCost = computeBoundaryCost(edge, v);
		FloatType volumeCost = computeVolumeCost(edge, v);
		FloatType shapeCost = 0;//computeShapeCost(p, profile.link());
    
		FloatType totalCost = _volumeWeight * volumeCost
		                    + _boundaryWeight * bdryCost * squaredLength
                    		+ _shapeWeight * shapeCost * squaredLength * squaredLength;
    
		OVITO_ASSERT(totalCost >= 0);
    	return std::isfinite(totalCost) ? totalCost : FloatType(-1);
	}

	/// Calls a callback function on all faces which are adjacenet to the two vertices of the given edge.
	template<typename Callback>
	void visitAdjacentTriangles(Edge* edge, Callback callback) const {
		// Go around first vertex.
		OVITO_ASSERT(edge->oppositeEdge() && edge->oppositeEdge()->nextFaceEdge());
		Edge* currentEdge = edge;
		Edge* stopEdge = edge->oppositeEdge()->nextFaceEdge();
		do {
			OVITO_ASSERT(currentEdge->face());
			callback(currentEdge->face());
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
			OVITO_ASSERT(currentEdge->face());
			callback(currentEdge->face());
			currentEdge = currentEdge->prevFaceEdge()->oppositeEdge();
			OVITO_ASSERT(currentEdge);
		}
		while(currentEdge != stopEdge);
	}

	FloatType computeBoundaryCost(Edge* edge, const Vector3& v) const { return 0; }

	FloatType computeVolumeCost(Edge* edge, const Vector3& v) const
	{
		FloatType cost = 0;
		visitAdjacentTriangles(edge, [&cost,&v](Face* face) {
			Edge* fedge0 = face->edges();
			Edge* fedge1 = fedge0->nextFaceEdge();
			Edge* fedge2 = fedge1->nextFaceEdge();
			const Point3& p0 = fedge0->vertex2()->pos();
			const Point3& p1 = fedge1->vertex2()->pos();
			const Point3& p2 = fedge2->vertex2()->pos();
			Vector3 v01 = p1 - p0;
			Vector3 v02 = p2 - p0;
			Vector3 normalV = v01.cross(v02);
			FloatType normalL = (p0 - Point3::Origin()).cross(p1 - Point3::Origin()).dot(p2 - Point3::Origin());
			FloatType f = normalV.dot(v) - normalL;
			cost += f*f;
		});
		return cost / 36;
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
		Vector3 e01 = p1 - p0;
		Vector3 e02 = p2 - p0;
		Vector3 e03 = p3 - p0;

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
			if((l0123 * l0123) <= mcMaxDihedralAngleCos2 * (l012 * l023))
				return true;
		}
		return false;
  	}

	/// Removes the given edge from the mesh and updates the neighborhood.
	void collapse(Edge* edge, const Point3& placement)
	{
		Edge* oppositeEdge = edge->oppositeEdge();
		OVITO_ASSERT(oppositeEdge);

		// Reposition vertex.
		Vertex* remainingVertex = edge->vertex2();
		remainingVertex->setPos(placement);

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

	void eraseEdgeFromPQ(Edge* edge) {
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

	void updateEdgeCostIfPrimary(Edge* edge) {
		auto handle = _pqHandles.find(edge);
		if(handle == _pqHandles.end())
			return;
		Point3 placement = computePlacement(edge);
		(*handle->second).cost = computeCost(edge, placement);
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

	// Lindstrom-Turk algorithm parameters:
	FloatType _volumeWeight = FloatType(0.5);
	FloatType _boundaryWeight = FloatType(0.5);
	FloatType _shapeWeight = FloatType(0);

	static constexpr FloatType cMaxAreaRatio = FloatType(1e8);
	static constexpr double cMaxDihedralAngleCos = 0.99984769515639123916; // =cos(1 degree)
	static constexpr FloatType mcMaxDihedralAngleCos2 = FloatType(0.999695413509547865); // =cos(1 degree)^2
};


OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


