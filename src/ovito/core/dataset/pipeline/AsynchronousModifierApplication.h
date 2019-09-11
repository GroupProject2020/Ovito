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


#include <ovito/core/Core.h>
#include "ModifierApplication.h"
#include "AsynchronousModifier.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * \brief Represents the application of an AsynchronousModifier in a data pipeline.
 */
class OVITO_CORE_EXPORT AsynchronousModifierApplication : public ModifierApplication
{
	Q_OBJECT
	OVITO_CLASS(AsynchronousModifierApplication)

public:

	/// \brief Constructs a modifier application.
	Q_INVOKABLE AsynchronousModifierApplication(DataSet* dataset);

	/// Returns the cached results of the AsynchronousModifier from the last pipeline evaluation.
	const AsynchronousModifier::ComputeEnginePtr& lastComputeResults() const { return _lastComputeResults; }

	/// Sets the cached results of the AsynchronousModifier from the last pipeline evaluation.
	void setLastComputeResults(AsynchronousModifier::ComputeEnginePtr results) { _lastComputeResults = std::move(results); }

protected:

	/// \brief Is called when a RefTarget referenced by this object has generated an event.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

	/// Is called when the value of a reference field of this object changes.
	virtual void referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget) override;

private:

	/// The cached results of the AsynchronousModifier from the last pipeline evaluation.
	AsynchronousModifier::ComputeEnginePtr _lastComputeResults;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


