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


#include <plugins/mesh/Mesh.h>
#include <plugins/mesh/surface/SurfaceMesh.h>
#include <plugins/stdmod/modifiers/SliceModifier.h>

namespace Ovito { namespace Mesh {

/**
 * \brief Slice function that operates on surface meshes.
 */
class OVITO_MESH_EXPORT SurfaceMeshSliceModifierDelegate : public SliceModifierDelegate
{
	/// Give the modifier delegate its own metaclass.
	class SurfaceMeshSliceModifierDelegateClass : public SliceModifierDelegate::OOMetaClass 
	{
	public:

		/// Inherit constructor from base class.
		using SliceModifierDelegate::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier delegate can operate on the given input data.
		virtual bool isApplicableTo(const PipelineFlowState& input) const override {
			return input.findObjectOfType<SurfaceMesh>() != nullptr;
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
	virtual PipelineStatus apply(Modifier* modifier, const PipelineFlowState& input, PipelineFlowState& output, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs) override;
};

}	// End of namespace
}	// End of namespace
