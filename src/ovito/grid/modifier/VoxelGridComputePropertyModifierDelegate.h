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
#include <ovito/stdmod/modifiers/ComputePropertyModifier.h>

namespace Ovito { namespace Grid {

/**
 * \brief Delegate plugin for the ComputePropertyModifier that operates on voxel grids.
 */
class OVITO_GRID_EXPORT VoxelGridComputePropertyModifierDelegate : public ComputePropertyModifierDelegate
{
	/// Give the modifier delegate its own metaclass.
	class OOMetaClass : public ComputePropertyModifierDelegate::OOMetaClass
	{
	public:

		/// Inherit constructor from base class.
		using ComputePropertyModifierDelegate::OOMetaClass::OOMetaClass;

		/// Indicates which data objects in the given input data collection the modifier delegate is able to operate on.
		virtual QVector<DataObjectReference> getApplicableObjects(const DataCollection& input) const override;

		/// Indicates which class of data objects the modifier delegate is able to operate on.
		virtual const DataObject::OOMetaClass& getApplicableObjectClass() const override { return VoxelGrid::OOClass(); }

		/// The name by which Python scripts can refer to this modifier delegate.
		virtual QString pythonDataName() const override { return QStringLiteral("voxels"); }
	};

	Q_OBJECT
	OVITO_CLASS_META(VoxelGridComputePropertyModifierDelegate, OOMetaClass)

	Q_CLASSINFO("DisplayName", "Voxel grids");

public:

	/// Constructor.
	Q_INVOKABLE VoxelGridComputePropertyModifierDelegate(DataSet* dataset) :  ComputePropertyModifierDelegate(dataset) {}

	/// Creates a computation engine that will compute the property values.
	virtual std::shared_ptr<ComputePropertyModifierDelegate::PropertyComputeEngine> createEngine(
				TimePoint time,
				const PipelineFlowState& input,
				const PropertyContainer* container,
				PropertyPtr outputProperty,
				ConstPropertyPtr selectionProperty,
				QStringList expressions) override;

private:

	/// Asynchronous compute engine that does the actual work in a separate thread.
	class ComputeEngine : public ComputePropertyModifierDelegate::PropertyComputeEngine
	{
	public:

		/// Constructor.
		ComputeEngine(
				const TimeInterval& validityInterval,
				TimePoint time,
				PropertyPtr outputProperty,
				const PropertyContainer* container,
				ConstPropertyPtr selectionProperty,
				QStringList expressions,
				int frameNumber,
				const PipelineFlowState& input);

		/// Computes the modifier's results.
		virtual void perform() override;
	};
};

}	// End of namespace
}	// End of namespace
