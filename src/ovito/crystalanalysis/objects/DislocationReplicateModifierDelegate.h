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


#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/crystalanalysis/objects/DislocationNetworkObject.h>
#include <ovito/stdmod/modifiers/ReplicateModifier.h>

namespace Ovito { namespace CrystalAnalysis {

using namespace Ovito::StdMod;

/**
 * \brief Delegate for the ReplicateModifier that operates on dislocation lines.
 */
class DislocationReplicateModifierDelegate : public ReplicateModifierDelegate
{
	/// Give the modifier delegate its own metaclass.
	class OOMetaClass : public ReplicateModifierDelegate::OOMetaClass
	{
	public:

		/// Inherit constructor from base class.
		using ReplicateModifierDelegate::OOMetaClass::OOMetaClass;

		/// Indicates which data objects in the given input data collection the modifier delegate is able to operate on.
		virtual QVector<DataObjectReference> getApplicableObjects(const DataCollection& input) const override;

		/// The name by which Python scripts can refer to this modifier delegate.
		virtual QString pythonDataName() const override { return QStringLiteral("dislocations"); }
	};

	Q_OBJECT
	OVITO_CLASS_META(DislocationReplicateModifierDelegate, OOMetaClass)

	Q_CLASSINFO("DisplayName", "Dislocations");

public:

	/// Constructor.
	Q_INVOKABLE DislocationReplicateModifierDelegate(DataSet* dataset) : ReplicateModifierDelegate(dataset) {}

	/// Applies the modifier operation to the data in a pipeline flow state.
	virtual PipelineStatus apply(Modifier* modifier, PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs) override;
};

}	// End of namespace
}	// End of namespace
