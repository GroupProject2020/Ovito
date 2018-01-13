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


#include <plugins/stdobj/StdObj.h>
#include <core/dataset/data/DataObject.h>
#include "SimulationCell.h"

namespace Ovito { namespace StdObj {
	
/**
 * \brief Stores the geometry and boundary conditions of a simulation box.
 *
 * The simulation box geometry is a parallelepiped defined by three edge vectors.
 * A fourth vector specifies the origin of the simulation box in space.
 */
class OVITO_STDOBJ_EXPORT SimulationCellObject : public DataObject
{
	Q_OBJECT
	OVITO_CLASS(SimulationCellObject)
	
public:

	/// \brief Constructor. Creates an empty simulation cell.
	Q_INVOKABLE SimulationCellObject(DataSet* dataset) : DataObject(dataset),
		_cellMatrix(AffineTransformation::Zero()),
		_pbcX(false), _pbcY(false), _pbcZ(false), _is2D(false) {
		init(dataset);
	}

	/// \brief Constructs a cell from the given cell data structure.
	explicit SimulationCellObject(DataSet* dataset, const SimulationCell& data) : DataObject(dataset),
			_cellMatrix(data.matrix()),
			_pbcX(data.pbcFlags()[0]), _pbcY(data.pbcFlags()[1]), _pbcZ(data.pbcFlags()[2]), _is2D(data.is2D()) {
		init(dataset);
	}

	/// \brief Constructs a cell from three vectors specifying the cell's edges.
	/// \param a1 The first edge vector.
	/// \param a2 The second edge vector.
	/// \param a3 The third edge vector.
	/// \param origin The origin position.
	SimulationCellObject(DataSet* dataset, const Vector3& a1, const Vector3& a2, const Vector3& a3,
			const Point3& origin = Point3::Origin(), bool pbcX = false, bool pbcY = false, bool pbcZ = false, bool is2D = false) :
		DataObject(dataset),
		_cellMatrix(a1, a2, a3, origin - Point3::Origin()), 
		_pbcX(pbcX), _pbcY(pbcY), _pbcZ(pbcZ), _is2D(is2D) {
		init(dataset);
	}

	/// \brief Constructs a cell from a matrix that specifies its shape and position in space.
	/// \param cellMatrix The matrix
	SimulationCellObject(DataSet* dataset, const AffineTransformation& cellMatrix, bool pbcX = false, bool pbcY = false, bool pbcZ = false, bool is2D = false) :
		DataObject(dataset),
		_cellMatrix(cellMatrix), _pbcX(pbcX), _pbcY(pbcY), _pbcZ(pbcZ), _is2D(is2D) {
		init(dataset);
	}

	/// \brief Constructs a cell with an axis-aligned box shape.
	/// \param box The axis-aligned box.
	/// \param pbcX Specifies whether periodic boundary conditions are enabled in the X direction.
	/// \param pbcY Specifies whether periodic boundary conditions are enabled in the Y direction.
	/// \param pbcZ Specifies whether periodic boundary conditions are enabled in the Z direction.
	SimulationCellObject(DataSet* dataset, const Box3& box, bool pbcX = false, bool pbcY = false, bool pbcZ = false, bool is2D = false) :
		DataObject(dataset),
		_cellMatrix(box.sizeX(), 0, 0, box.minc.x(), 0, box.sizeY(), 0, box.minc.y(), 0, 0, box.sizeZ(), box.minc.z()),
		_pbcX(pbcX), _pbcY(pbcY), _pbcZ(pbcZ), _is2D(is2D) {
		init(dataset);
		OVITO_ASSERT_MSG(box.sizeX() >= 0 && box.sizeY() >= 0 && box.sizeZ() >= 0, "SimulationCellObject constructor", "The simulation box must have a non-negative volume.");
	}

	/// \brief Sets the cell geometry to match the given cell data structure.
	void setData(const SimulationCell& data, bool setBoundaryFlags = true) {
		setCellMatrix(data.matrix());
		if(setBoundaryFlags) {
			setPbcX(data.pbcFlags()[0]);
			setPbcY(data.pbcFlags()[1]);
			setPbcZ(data.pbcFlags()[2]);
			setIs2D(data.is2D());
		}
	}

	/// \brief Returns a simulation cell data structure that stores the cell's properties.
	SimulationCell data() const {
		return SimulationCell(cellMatrix(), pbcFlags(), is2D());
	}

	/// Returns inverse of the simulation cell matrix.
	/// This matrix maps the simulation cell to the unit cube ([0,1]^3).
	AffineTransformation reciprocalCellMatrix() const {
		return cellMatrix().inverse();
	}

	/// Computes the (positive) volume of the three-dimensional cell.
	FloatType volume3D() const {
		return std::abs(cellMatrix().determinant());
	}

	/// Computes the (positive) volume of the two-dimensional cell.
	FloatType volume2D() const {
		return cellMatrix().column(0).cross(cellMatrix().column(1)).length();
	}

	/// \brief Enables or disables periodic boundary conditions in the three spatial directions.
	void setPBCFlags(const std::array<bool,3>& flags) {
		setPbcX(flags[0]);
		setPbcY(flags[1]);
		setPbcZ(flags[2]);
	}

	/// \brief Returns the periodic boundary flags in all three spatial directions.
	std::array<bool,3> pbcFlags() const {
		return {{ pbcX(), pbcY(), pbcZ() }};
	}

	/// \brief Returns the first edge vector of the cell.
	const Vector3& cellVector1() const { return cellMatrix().column(0); }

	/// \brief Returns the second edge vector of the cell.
	const Vector3& cellVector2() const { return cellMatrix().column(1); }

	/// \brief Returns the third edge vector of the cell.
	const Vector3& cellVector3() const { return cellMatrix().column(2); }

	/// \brief Returns the origin point of the cell.
	const Point3& cellOrigin() const { return Point3::Origin() + cellMatrix().column(3); }

	/// \brief Returns the title of this object.
	virtual QString objectTitle() override { return tr("Simulation cell"); }

	////////////////////////////// Support functions for the Python bindings //////////////////////////////

	/// Indicates to the Python binding layer that this object has been temporarily put into a 
	/// writable state. In this state, the binding layer will allow write access to the cell's internal data.
	bool isWritableFromPython() const { return _isWritableFromPython != 0; }

	/// Puts the simulayion cell into a writable state.
	/// In the writable state, the Python binding layer will allow write access to the cell's internal data.
	void makeWritableFromPython() { 
		_isWritableFromPython++; 
	}

	/// Puts the simulation cell array back into the default read-only state. 
	/// In the read-only state, the Python binding layer will not permit write access to the cell's internal data.
	void makeReadOnlyFromPython() {
		OVITO_ASSERT(_isWritableFromPython > 0);
		_isWritableFromPython--;
	}

protected:

	/// Creates the storage for the internal parameters.
	void init(DataSet* dataset);

	/// Stores the three cell vectors and the position of the cell origin.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(AffineTransformation, cellMatrix, setCellMatrix);

	/// Specifies periodic boundary condition in the X direction.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, pbcX, setPbcX);
	/// Specifies periodic boundary condition in the Y direction.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, pbcY, setPbcY);
	/// Specifies periodic boundary condition in the Z direction.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, pbcZ, setPbcZ);

	/// Stores the dimensionality of the system.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, is2D, setIs2D);

	/// This is a special flag used by the Python bindings to indicate that
	/// this simulation cell has been temporarily put into a writable state.
	int _isWritableFromPython = 0;	
};

}	// End of namespace
}	// End of namespace
