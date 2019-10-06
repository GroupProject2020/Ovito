///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
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


#include <ovito/mesh/Mesh.h>
#include <ovito/mesh/surface/SurfaceMesh.h>
#include <ovito/stdmod/modifiers/SliceModifier.h>

namespace Ovito { namespace Mesh {

using namespace Ovito::StdMod;

/**
 * \brief Slice function that operates on surface meshes.
 */
class SurfaceMeshSliceModifierDelegate : public SliceModifierDelegate
{
	/// Give the modifier delegate its own metaclass.
	class SurfaceMeshSliceModifierDelegateClass : public SliceModifierDelegate::OOMetaClass
	{
	public:

		/// Inherit constructor from base class.
		using SliceModifierDelegate::OOMetaClass::OOMetaClass;

		/// Indicates which data objects in the given input data collection the modifier delegate is able to operate on.
		virtual QVector<DataObjectReference> getApplicableObjects(const DataCollection& input) const override {
			if(input.containsObject<SurfaceMesh>())
				return { DataObjectReference(&SurfaceMesh::OOClass()) };
			return {};
		}

		/// The name by which Python scripts can refer to this modifier delegate.
		virtual QString pythonDataName() const override { return QStringLiteral("surfaces"); }
	};

	Q_OBJECT
	OVITO_CLASS_META(SurfaceMeshSliceModifierDelegate, SurfaceMeshSliceModifierDelegateClass)
	Q_CLASSINFO("DisplayName", "Surfaces");

public:

	/// Constructor.
	Q_INVOKABLE SurfaceMeshSliceModifierDelegate(DataSet* dataset) : SliceModifierDelegate(dataset) {}

	/// \brief Applies a slice operation to a data object.
	virtual PipelineStatus apply(Modifier* modifier, PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs) override;
};

}	// End of namespace
}	// End of namespace
