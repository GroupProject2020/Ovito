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
#include <plugins/stdobj/properties/PropertyObject.h>
#include <plugins/stdobj/properties/PropertyContainer.h>
#include <core/app/Application.h>
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
	// Operate on particles by default.
	setDefaultSubject(QStringLiteral("Particles"), QStringLiteral("ParticlesObject"));
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void SelectTypeModifier::initializeModifier(ModifierApplication* modApp)
{
	GenericPropertyModifier::initializeModifier(modApp);

	if(sourceProperty().isNull() && subject() && !Application::instance()->scriptMode()) {

		// When the modifier is first inserted, automatically select the most recently added typed property.
		const PipelineFlowState& input = modApp->evaluateInputPreliminary();
		if(const PropertyContainer* container = input.getLeafObject(subject())) {
			PropertyReference bestProperty;
			for(PropertyObject* property : container->properties()) {
				if(property->elementTypes().empty() == false && property->componentCount() == 1 && property->dataType() == PropertyStorage::Int) {
					bestProperty = PropertyReference(subject().dataClass(), property);
				}
			}
			if(!bestProperty.isNull())
				setSourceProperty(bestProperty);
		}
	}
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void SelectTypeModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	// Whenever the selected property class of this modifier is changed, update the source property reference accordingly.
	if(field == PROPERTY_FIELD(GenericPropertyModifier::subject) && !isBeingLoaded() && !dataset()->undoStack().isUndoingOrRedoing()) {
		setSourceProperty(sourceProperty().convertToContainerClass(subject().dataClass()));
	}
	GenericPropertyModifier::propertyChanged(field);
}

/******************************************************************************
* Modifies the input data in an immediate, preliminary way.
******************************************************************************/
PipelineFlowState SelectTypeModifier::evaluatePreliminary(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	if(!subject())
		throwException(tr("No input element type selected."));
	if(sourceProperty().isNull())
		throwException(tr("No input property selected."));

	// Check if the source property is the right kind of property.
	if(sourceProperty().containerClass() != subject().dataClass())
		throwException(tr("Modifier was set to operate on '%1', but the selected input is a '%2' property.")
			.arg(subject().dataClass()->pythonName()).arg(sourceProperty().containerClass()->propertyClassDisplayName()));

	PipelineFlowState output = input;
	PropertyContainer* container = output.expectMutableLeafObject(subject());

	// Get the input property.
	const PropertyObject* typeProperty = sourceProperty().findInContainer(container);
	if(!typeProperty)
		throwException(tr("The selected input property '%1' is not present.").arg(sourceProperty().name()));
	if(typeProperty->componentCount() != 1)
		throwException(tr("The input property '%1' has the wrong number of components. Must be a scalar property.").arg(typeProperty->name()));
	if(typeProperty->dataType() != PropertyStorage::Int)
		throwException(tr("The input property '%1' has the wrong data type. Must be an integer property.").arg(typeProperty->name()));

	// Create the selection property.
	PropertyPtr selProperty = container->createProperty(PropertyStorage::GenericSelectionProperty)->modifiableStorage();

	// Counts the number of selected elements.
	size_t nSelected = 0;

	// Generate set of numeric type IDs to select.
	QSet<int> idsToSelect = selectedTypeIDs();
	// Convert type names to to IDs.
	for(const QString& typeName : selectedTypeNames()) {
		if(ElementType* t = typeProperty->elementType(typeName))
			idsToSelect.insert(t->numericId());
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

	output.addAttribute(QStringLiteral("SelectType.num_selected"), QVariant::fromValue(nSelected), modApp);
	
	QString statusMessage = tr("%1 out of %2 %3 selected (%4%)")
		.arg(nSelected)
		.arg(typeProperty->size())
		.arg(container->getOOMetaClass().elementDescriptionName())
		.arg((FloatType)nSelected * 100 / std::max((size_t)1,typeProperty->size()), 0, 'f', 1);

	output.setStatus(PipelineStatus(PipelineStatus::Success, std::move(statusMessage)));
	return output;
}

}	// End of namespace
}	// End of namespace
