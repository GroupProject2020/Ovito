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

#pragma once


#include <ovito/grid/Grid.h>
#include <ovito/grid/objects/VoxelGrid.h>
#include <ovito/stdmod/modifiers/ColorCodingModifier.h>

namespace Ovito { namespace Grid {

using namespace Ovito::StdMod;

/**
 * \brief Function for the ColorCodingModifier that operates on voxel grid cells.
 */
class OVITO_GRID_EXPORT VoxelGridColorCodingModifierDelegate : public ColorCodingModifierDelegate
{
	/// Give the modifier delegate its own metaclass.
	class OOMetaClass : public ColorCodingModifierDelegate::OOMetaClass
	{
	public:

		/// Inherit constructor from base class.
		using ColorCodingModifierDelegate::OOMetaClass::OOMetaClass;

		/// Indicates which data objects in the given input data collection the modifier delegate is able to operate on.
		virtual QVector<DataObjectReference> getApplicableObjects(const DataCollection& input) const override;

		/// Indicates which class of data objects the modifier delegate is able to operate on.
		virtual const DataObject::OOMetaClass& getApplicableObjectClass() const override { return VoxelGrid::OOClass(); }

		/// The name by which Python scripts can refer to this modifier delegate.
		virtual QString pythonDataName() const override { return QStringLiteral("voxels"); }
	};

	Q_OBJECT
	OVITO_CLASS_META(VoxelGridColorCodingModifierDelegate, OOMetaClass)

	Q_CLASSINFO("DisplayName", "Voxel grids");

public:

	/// Constructor.
	Q_INVOKABLE VoxelGridColorCodingModifierDelegate(DataSet* dataset) : ColorCodingModifierDelegate(dataset) {}

protected:

	/// \brief returns the ID of the standard property that will receive the computed colors.
	virtual int outputColorPropertyId() const override { return VoxelGrid::ColorProperty; }
};

}	// End of namespace
}	// End of namespace
