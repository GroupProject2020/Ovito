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

#include <plugins/particles/Particles.h>
#include <plugins/particles/objects/BondsDisplay.h>
#include <core/dataset/pipeline/PipelineFlowState.h>
#include "BondProperty.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(BondProperty);

/******************************************************************************
* Constructor.
******************************************************************************/
BondProperty::BondProperty(DataSet* dataset) : PropertyObject(dataset)
{
}

/******************************************************************************
* Gives the property class the opportunity to set up a newly created 
* property object.
******************************************************************************/
void BondProperty::OOMetaClass::prepareNewProperty(PropertyObject* property) const
{
	if(property->type() == BondProperty::TopologyProperty) {
		OORef<BondsDisplay> displayObj = new BondsDisplay(property->dataset());
		displayObj->loadUserDefaults();
		property->addDisplayObject(displayObj);
	}
}

/******************************************************************************
* Creates a storage object for standard bond properties.
******************************************************************************/
PropertyPtr BondProperty::OOMetaClass::createStandardStorage(size_t bondsCount, int type, bool initializeMemory) const
{
	int dataType;
	size_t componentCount;
	size_t stride;
	
	switch(type) {
	case TypeProperty:
	case SelectionProperty:
		dataType = PropertyStorage::Int;
		componentCount = 1;
		stride = sizeof(int);
		break;
	case LengthProperty:
		dataType = PropertyStorage::Float;
		componentCount = 1;
		stride = sizeof(FloatType);
		break;
	case ColorProperty:
		dataType = PropertyStorage::Float;
		componentCount = 3;
		stride = componentCount * sizeof(FloatType);
		OVITO_ASSERT(stride == sizeof(Color));
		break;
	case TopologyProperty:
		dataType = PropertyStorage::Int64;
		componentCount = 2;
		stride = componentCount * sizeof(qlonglong);
		break;		
	case PeriodicImageProperty:
		dataType = PropertyStorage::Int;
		componentCount = 3;
		stride = componentCount * sizeof(int);
		break;		
	default:
		OVITO_ASSERT_MSG(false, "BondProperty::createStandardStorage", "Invalid standard property type");
		throw Exception(tr("This is not a valid standard bond property type: %1").arg(type));
	}
	const QStringList& componentNames = standardPropertyComponentNames(type);
	const QString& propertyName = standardPropertyName(type);

	OVITO_ASSERT(componentCount == standardPropertyComponentCount(type));
	
	return std::make_shared<PropertyStorage>(bondsCount, dataType, componentCount, stride, 
								propertyName, initializeMemory, type, componentNames);
}

/******************************************************************************
* Returns the number of particles in the given data state.
******************************************************************************/
size_t BondProperty::OOMetaClass::elementCount(const PipelineFlowState& state) const
{
	for(DataObject* obj : state.objects()) {
		if(BondProperty* property = dynamic_object_cast<BondProperty>(obj)) {
			return property->size();
		}
	}
	return 0;
}

/******************************************************************************
* Determines if the data elements which this property class applies to are 
* present for the given data state.
******************************************************************************/
bool BondProperty::OOMetaClass::isDataPresent(const PipelineFlowState& state) const
{
	return state.findObject<BondProperty>() != nullptr;
}

/******************************************************************************
* Registers all standard properties with the property traits class.
******************************************************************************/
void BondProperty::OOMetaClass::initialize()
{
	PropertyClass::initialize();

	// Enable automatic conversion of a BondPropertyReference to a generic PropertyReference and vice versa.
	QMetaType::registerConverter<BondPropertyReference, PropertyReference>();
	QMetaType::registerConverter<PropertyReference, BondPropertyReference>();	
	
	setPropertyClassDisplayName(tr("Bonds"));
	setElementDescriptionName(QStringLiteral("bonds"));
	setPythonName(QStringLiteral("bonds"));

	const QStringList emptyList;
	const QStringList abList = QStringList() << "A" << "B";
	const QStringList xyzList = QStringList() << "X" << "Y" << "Z";
	const QStringList rgbList = QStringList() << "R" << "G" << "B";
	
	registerStandardProperty(TypeProperty, tr("Bond Type"), PropertyStorage::Int, emptyList, tr("Bond types"));
	registerStandardProperty(SelectionProperty, tr("Selection"), PropertyStorage::Int, emptyList);
	registerStandardProperty(ColorProperty, tr("Color"), PropertyStorage::Float, rgbList, tr("Bond colors"));
	registerStandardProperty(LengthProperty, tr("Length"), PropertyStorage::Float, emptyList);
	registerStandardProperty(TopologyProperty, tr("Topology"), PropertyStorage::Int64, abList);
	registerStandardProperty(PeriodicImageProperty, tr("Periodic Image"), PropertyStorage::Int, xyzList);
}

}	// End of namespace
}	// End of namespace
