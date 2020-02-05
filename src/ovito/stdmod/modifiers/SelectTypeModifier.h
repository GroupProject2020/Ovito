////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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


#include <ovito/stdmod/StdMod.h>
#include <ovito/stdobj/properties/GenericPropertyModifier.h>
#include <ovito/stdobj/properties/PropertyReference.h>

namespace Ovito { namespace StdMod {

/**
 * \brief Selects data elements of one or more types.
 */
class OVITO_STDMOD_EXPORT SelectTypeModifier : public GenericPropertyModifier
{
	Q_OBJECT
	OVITO_CLASS(SelectTypeModifier)

	Q_CLASSINFO("DisplayName", "Select type");
	Q_CLASSINFO("ModifierCategory", "Selection");

public:

	/// Constructor.
	Q_INVOKABLE SelectTypeModifier(DataSet* dataset);

	/// This method is called by the system after the modifier has been inserted into a data pipeline.
	virtual void initializeModifier(ModifierApplication* modApp) override;

	/// Modifies the input data synchronously.
	virtual void evaluateSynchronous(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

protected:

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

private:

	/// The input type property that is used as data source for the selection.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(PropertyReference, sourceProperty, setSourceProperty);

	/// The numeric IDs of the types to select.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QSet<int>, selectedTypeIDs, setSelectedTypeIDs);

	/// The names of the types to select.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QSet<QString>, selectedTypeNames, setSelectedTypeNames);
};

}	// End of namespace
}	// End of namespace
