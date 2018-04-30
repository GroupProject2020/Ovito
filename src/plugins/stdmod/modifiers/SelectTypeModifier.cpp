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

#include <plugins/stdmod/StdMod.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/UndoStack.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <plugins/stdobj/util/OutputHelper.h>
#include <plugins/stdobj/properties/PropertyObject.h>
#include <core/app/PluginManager.h>
#include "SelectTypeModifier.h"

namespace Ovito { namespace StdMod {

IMPLEMENT_OVITO_CLASS(SelectTypeModifier);
DEFINE_PROPERTY_FIELD(SelectTypeModifier, sourceProperty);
DEFINE_PROPERTY_FIELD(SelectTypeModifier, selectedTypeIDs);
DEFINE_PROPERTY_FIELD(SelectTypeModifier, selectedTypeNames);
SET_PROPERTY_FIELD_LABEL(SelectTypeModifier, sourceProperty, "Property");
SET_PROPERTY_FIELD_LABEL(SelectTypeModifier, selectedTypeIDs, "Selected type IDs");
SET_PROPERTY_FIELD_LABEL(SelectTypeModifier, selectedTypeNames, "Selected type names");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
SelectTypeModifier::SelectTypeModifier(DataSet* dataset) : GenericPropertyModifier(dataset)
{
	// Operate on particle properties by default.
	setPropertyClass(static_cast<const PropertyClass*>(
		PluginManager::instance().findClass(QStringLiteral("Particles"), QStringLiteral("ParticleProperty"))));	
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void SelectTypeModifier::initializeModifier(ModifierApplication* modApp)
{
	GenericPropertyModifier::initializeModifier(modApp);

	if(sourceProperty().isNull() && propertyClass()) {

		// Select the first type property from the input with more than one type.
		PipelineFlowState input = modApp->evaluateInputPreliminary();
		PropertyReference bestProperty;
		for(DataObject* o : input.objects()) {
			if(PropertyObject* property = dynamic_object_cast<PropertyObject>(o)) {
				if(propertyClass()->isMember(property) && property->elementTypes().empty() == false && 
					property->componentCount() == 1 && property->dataType() == PropertyStorage::Int) {
						bestProperty = PropertyReference(property);
				}
			}
		}
		if(!bestProperty.isNull())
			setSourceProperty(bestProperty);
	}
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void SelectTypeModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	// Whenever the selected property class of this modifier is changed, clear the source property reference.
	// Otherwise it might be pointing to the wrong kind of property.
	if(field == PROPERTY_FIELD(GenericPropertyModifier::propertyClass) && !isBeingLoaded()) {
		if(propertyClass() != sourceProperty().propertyClass()) {
			setSourceProperty({});
		}
	}
	GenericPropertyModifier::propertyChanged(field);
}

/******************************************************************************
* Modifies the input data in an immediate, preliminary way.
******************************************************************************/
PipelineFlowState SelectTypeModifier::evaluatePreliminary(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	if(!propertyClass())
		throwException(tr("No input property class selected."));
	if(sourceProperty().isNull())
		throwException(tr("No input property selected."));

	// Check if the source property is the right kind of property.
	if(sourceProperty().propertyClass() != propertyClass())
		throwException(tr("Modifier was set to operate on '%1', but the selected input is a '%2' property.")
			.arg(propertyClass()->pythonName()).arg(sourceProperty().propertyClass()->propertyClassDisplayName()));

	// Get the input property.
	PropertyObject* typeProperty = sourceProperty().findInState(input);
	if(!typeProperty)
		throwException(tr("The selected input property '%1' is not present.").arg(sourceProperty().name()));
	if(typeProperty->componentCount() != 1)
		throwException(tr("The input property '%1' has the wrong number of components.").arg(typeProperty->name()));
	if(typeProperty->dataType() != PropertyStorage::Int)
		throwException(tr("The input property '%1' has the wrong data type.").arg(typeProperty->name()));

	// Create the selection property.
	PipelineFlowState output = input;	
	OutputHelper oh(dataset(), output);
	PropertyPtr selProperty = oh.outputStandardProperty(*propertyClass(), PropertyStorage::GenericSelectionProperty)->modifiableStorage();

	// Counts the number of selected elements.
	size_t nSelected = 0;

	// Generate set of numeric type IDs to select.
	QSet<int> idsToSelect = selectedTypeIDs();
	// Convert type names to to IDs.
	for(const QString& typeName : selectedTypeNames()) {
		if(ElementType* t = typeProperty->elementType(typeName))
			idsToSelect.insert(t->id());
		else
			throwException(tr("There is no type named '%1' in the type list of input property '%2'.").arg(typeName).arg(typeProperty->name()));
	}
	
	OVITO_ASSERT(selProperty->size() == typeProperty->size());
	const int* t = typeProperty->constDataInt();
	for(int& s : selProperty->intRange()) {
		if(idsToSelect.contains(*t++)) {
			s = 1;
			nSelected++;
		}
		else s = 0;
	}

	oh.outputAttribute(QStringLiteral("SelectType.num_selected"), QVariant::fromValue(nSelected));
	
	// For backward compatibility with OVITO 2.9.0:
	if(propertyClass()->pythonName() == QStringLiteral("particles"))
		oh.outputAttribute(QStringLiteral("SelectParticleType.num_selected"), QVariant::fromValue(nSelected));

	QString statusMessage = tr("%1 out of %2 %3 selected (%4%)")
		.arg(nSelected)
		.arg(typeProperty->size())
		.arg(propertyClass()->elementDescriptionName())
		.arg((FloatType)nSelected * 100 / std::max(1,(int)typeProperty->size()), 0, 'f', 1);

	output.setStatus(PipelineStatus(PipelineStatus::Success, std::move(statusMessage)));
	return output;
}

}	// End of namespace
}	// End of namespace
