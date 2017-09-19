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
#include <plugins/particles/objects/ParticleDisplay.h>
#include <plugins/particles/objects/BondsObject.h>
#include <plugins/particles/objects/BondsDisplay.h>
#include <plugins/particles/objects/BondProperty.h>
#include <core/dataset/DataSet.h>
#include "ParticleInputHelper.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers)

/******************************************************************************
* Constructor.
******************************************************************************/
ParticleInputHelper::ParticleInputHelper(DataSet* dataset, const PipelineFlowState& input) : 
	InputHelper(dataset, input)
{
	ParticleProperty* posProperty = inputStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty);
	_inputParticleCount = (posProperty != nullptr) ? posProperty->size() : 0;

	// Verify input, make sure array lengths of particle properties are consistent.
	for(DataObject* obj : input.objects()) {
		if(ParticleProperty* p = dynamic_object_cast<ParticleProperty>(obj)) {
			if(p->size() != inputParticleCount())
				dataset->throwException(PropertyObject::tr("Detected invalid modifier input. Data array size is not the same for all particle properties or property 'Position' is not present."));
		}
	}
	
	// Determine number of input bonds.
	BondsObject* bondsObj = input.findObject<BondsObject>();
	_inputBondCount = (bondsObj != nullptr) ? bondsObj->size() : 0;	

	// Verify input, make sure array lengths of bond properties are consistent.
	for(DataObject* obj : input.objects()) {
		if(BondProperty* p = dynamic_object_cast<BondProperty>(obj)) {
			if(p->size() != inputBondCount())
				dataset->throwException(PropertyObject::tr("Detected invalid modifier input. Data array size is not the same for all bond properties."));
		}
	}
}

/******************************************************************************
* Returns the input bonds.
******************************************************************************/
BondsObject* ParticleInputHelper::expectBonds() const
{
	BondsObject* bondsObj = input().findObject<BondsObject>();
	if(!bondsObj)
		dataset()->throwException(BondsObject::tr("The modifier cannot be evaluated because the input does not contain any bonds."));
	return bondsObj;
}

/******************************************************************************
* Returns a vector with the input particles colors.
******************************************************************************/
std::vector<Color> ParticleInputHelper::inputParticleColors(TimePoint time, TimeInterval& validityInterval)
{
	std::vector<Color> colors(inputParticleCount());

	// Obtain the particle display object.
	ParticleProperty* positionProperty = inputStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty);
	if(positionProperty) {
		for(DisplayObject* displayObj : positionProperty->displayObjects()) {
			if(ParticleDisplay* particleDisplay = dynamic_object_cast<ParticleDisplay>(displayObj)) {

				// Query particle colors from display object.
				particleDisplay->particleColors(colors,
						inputStandardProperty<ParticleProperty>(ParticleProperty::ColorProperty),
						inputStandardProperty<ParticleProperty>(ParticleProperty::TypeProperty));

				return colors;
			}
		}
	}

	std::fill(colors.begin(), colors.end(), Color(1,1,1));
	return colors;
}

/******************************************************************************
* Returns a vector with the input bond colors.
******************************************************************************/
std::vector<Color> ParticleInputHelper::inputBondColors(TimePoint time, TimeInterval& validityInterval)
{
	// Obtain the bond display object.
	BondsObject* bondObj = input().findObject<BondsObject>();
	if(bondObj) {
		for(DisplayObject* displayObj : bondObj->displayObjects()) {
			if(BondsDisplay* bondsDisplay = dynamic_object_cast<BondsDisplay>(displayObj)) {

				// Obtain the particle display object.
				ParticleDisplay* particleDisplay = nullptr;
				if(ParticleProperty* positionProperty = inputStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty)) {
					for(DisplayObject* displayObj : positionProperty->displayObjects()) {
						particleDisplay = dynamic_object_cast<ParticleDisplay>(displayObj);
						if(particleDisplay) break;
					}
				}

				// Query half-bond colors from display object.
				std::vector<Color> halfBondColors = bondsDisplay->halfBondColors(
						inputParticleCount(), 
						bondObj,
						inputStandardProperty<BondProperty>(BondProperty::ColorProperty),
						inputStandardProperty<BondProperty>(BondProperty::TypeProperty),
						inputStandardProperty<BondProperty>(BondProperty::SelectionProperty),
						particleDisplay, 
						inputStandardProperty<ParticleProperty>(ParticleProperty::ColorProperty),
						inputStandardProperty<ParticleProperty>(ParticleProperty::TypeProperty));
				OVITO_ASSERT(inputBondCount() * 2 == halfBondColors.size());

				// Map half-bond colors to full bond colors.
				std::vector<Color> colors(inputBondCount());				
				auto ci = halfBondColors.cbegin();
				for(Color& co : colors) {
					co = *ci;
					ci += 2;
				}
				return colors;
			}
		}
	}

	return std::vector<Color>(inputBondCount(), Color(1,1,1));
}

/******************************************************************************
* Returns a vector with the input particles radii.
******************************************************************************/
std::vector<FloatType> ParticleInputHelper::inputParticleRadii(TimePoint time, TimeInterval& validityInterval)
{
	std::vector<FloatType> radii(inputParticleCount());

	// Obtain the particle display object.
	ParticleProperty* positionProperty = inputStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty);
	if(positionProperty) {
		for(DisplayObject* displayObj : positionProperty->displayObjects()) {
			if(ParticleDisplay* particleDisplay = dynamic_object_cast<ParticleDisplay>(displayObj)) {

				// Query particle radii from display object.
				particleDisplay->particleRadii(radii,
						inputStandardProperty<ParticleProperty>(ParticleProperty::RadiusProperty),
						inputStandardProperty<ParticleProperty>(ParticleProperty::TypeProperty));

				return radii;
			}
		}
	}

	std::fill(radii.begin(), radii.end(), FloatType(1));
	return radii;
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

