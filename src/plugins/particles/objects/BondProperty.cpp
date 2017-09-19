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
#include <core/dataset/pipeline/PipelineFlowState.h>
#include "BondProperty.h"
#include "BondsObject.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(BondProperty);

/******************************************************************************
* Constructor.
******************************************************************************/
BondProperty::BondProperty(DataSet* dataset) : PropertyObject(dataset)
{
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
		dataType = qMetaTypeId<int>();
		componentCount = 1;
		stride = sizeof(int);
		break;
	case LengthProperty:
		dataType = qMetaTypeId<FloatType>();
		componentCount = 1;
		stride = sizeof(FloatType);
		break;
	case ColorProperty:
		dataType = qMetaTypeId<FloatType>();
		componentCount = 3;
		stride = componentCount * sizeof(FloatType);
		OVITO_ASSERT(stride == sizeof(Color));
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
	if(BondsObject* bonds = state.findObject<BondsObject>())
		return bonds->size();
	else
		return 0;
}

/******************************************************************************
* Determines if the data elements which this property class applies to are 
* present for the given data state.
******************************************************************************/
bool BondProperty::OOMetaClass::isDataPresent(const PipelineFlowState& state) const
{
	return state.findObject<BondsObject>() != nullptr;
}

/******************************************************************************
* Registers all standard properties with the property traits class.
******************************************************************************/
void BondProperty::OOMetaClass::initialize()
{
	PropertyClass::initialize();
	
	setPropertyClassDisplayName(tr("Bonds"));
	setElementDescriptionName(QStringLiteral("bonds"));
	setPythonName(QStringLiteral("bonds"));

	const QStringList emptyList;
	const QStringList rgbList = QStringList() << "R" << "G" << "B";
	
	registerStandardProperty(TypeProperty, tr("Bond Type"), qMetaTypeId<int>(), emptyList, tr("Bond types"));
	registerStandardProperty(SelectionProperty, tr("Selection"), qMetaTypeId<int>(), emptyList);
	registerStandardProperty(ColorProperty, tr("Color"), qMetaTypeId<FloatType>(), rgbList, tr("Bond colors"));
	registerStandardProperty(LengthProperty, tr("Length"), qMetaTypeId<FloatType>(), emptyList);
}

}	// End of namespace
}	// End of namespace
