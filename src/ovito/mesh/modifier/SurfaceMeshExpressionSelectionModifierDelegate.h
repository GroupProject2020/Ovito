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

#pragma once


#include <ovito/particles/Particles.h>
#include <ovito/stdmod/modifiers/ExpressionSelectionModifier.h>

namespace Ovito { namespace Mesh {

using namespace Ovito::StdMod;

/**
 * \brief Delegate for the ExpressionSelectionModifier that operates surface mesh regions.
 */
class SurfaceMeshRegionsExpressionSelectionModifierDelegate : public ExpressionSelectionModifierDelegate
{
	/// Give the modifier delegate its own metaclass.
	class OOMetaClass : public ExpressionSelectionModifierDelegate::OOMetaClass
	{
	public:

		/// Inherit constructor from base class.
		using ExpressionSelectionModifierDelegate::OOMetaClass::OOMetaClass;

		/// Indicates which data objects in the given input data collection the modifier delegate is able to operate on.
		virtual QVector<DataObjectReference> getApplicableObjects(const DataCollection& input) const override;

		/// Indicates which class of data objects the modifier delegate is able to operate on.
		virtual const DataObject::OOMetaClass& getApplicableObjectClass() const override { return SurfaceMeshRegions::OOClass(); }

		/// The name by which Python scripts can refer to this modifier delegate.
		virtual QString pythonDataName() const override { return QStringLiteral("surface_regions"); }
	};

	Q_OBJECT
	OVITO_CLASS_META(SurfaceMeshRegionsExpressionSelectionModifierDelegate, OOMetaClass)
	Q_CLASSINFO("DisplayName", "Mesh Regions");

public:

	/// Constructor.
	Q_INVOKABLE SurfaceMeshRegionsExpressionSelectionModifierDelegate(DataSet* dataset) : ExpressionSelectionModifierDelegate(dataset) {}

	/// Creates and initializes the expression evaluator object.
	virtual std::unique_ptr<PropertyExpressionEvaluator> initializeExpressionEvaluator(const QStringList& expressions, const PipelineFlowState& inputState, const DataObjectPath& objectPath, int animationFrame) override;
};

}	// End of namespace
}	// End of namespace
