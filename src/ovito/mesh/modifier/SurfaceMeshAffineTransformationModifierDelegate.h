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


#include <ovito/mesh/Mesh.h>
#include <ovito/mesh/surface/SurfaceMesh.h>
#include <ovito/stdmod/modifiers/AffineTransformationModifier.h>

namespace Ovito { namespace Mesh {

using namespace Ovito::StdMod;

/**
 * \brief Delegate for the AffineTransformationModifier that operates on surface meshes.
 */
class SurfaceMeshAffineTransformationModifierDelegate : public AffineTransformationModifierDelegate
{
	/// Give the modifier delegate its own metaclass.
	class SurfaceMeshAffineTransformationModifierDelegateClass : public AffineTransformationModifierDelegate::OOMetaClass
	{
	public:

		/// Inherit constructor from base class.
		using AffineTransformationModifierDelegate::OOMetaClass::OOMetaClass;

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
	OVITO_CLASS_META(SurfaceMeshAffineTransformationModifierDelegate, SurfaceMeshAffineTransformationModifierDelegateClass)
	Q_CLASSINFO("DisplayName", "Surfaces");

public:

	/// Constructor.
	Q_INVOKABLE SurfaceMeshAffineTransformationModifierDelegate(DataSet* dataset) : AffineTransformationModifierDelegate(dataset) {}

	/// Applies the modifier operation to the data in a pipeline flow state.
	virtual PipelineStatus apply(Modifier* modifier, PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs) override;
};

}	// End of namespace
}	// End of namespace
