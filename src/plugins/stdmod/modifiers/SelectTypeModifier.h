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


#include <plugins/stdmod/StdMod.h>
#include <plugins/stdobj/properties/GenericPropertyModifier.h>
#include <plugins/stdobj/properties/PropertyReference.h>

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

	/// Modifies the input data in an immediate, preliminary way.
	virtual PipelineFlowState evaluatePreliminary(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

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
