///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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
#include <plugins/particles/objects/ParticlesVis.h>
#include <plugins/particles/objects/BondsVis.h>
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
			OVITO_ASSERT(p->bundle().isEmpty());
			if(p->size() != inputParticleCount())
				dataset->throwException(PropertyObject::tr("Detected invalid modifier input. Data array size is not the same for all particle properties or property 'Position' is not present."));
		}
	}
	
	// Determine number of input bonds.
	BondProperty* topologyProperty = inputStandardProperty<BondProperty>(BondProperty::TopologyProperty);
	_inputBondCount = (topologyProperty != nullptr) ? topologyProperty->size() : 0;

	// Verify input, make sure array lengths of bond properties are consistent.
	for(DataObject* obj : input.objects()) {
		if(BondProperty* p = dynamic_object_cast<BondProperty>(obj)) {
			OVITO_ASSERT(p->bundle().isEmpty());
			if(p->size() != inputBondCount())
				dataset->throwException(PropertyObject::tr("Detected invalid modifier input. Data array size is not the same for all bond properties."));
		}
	}
}

/******************************************************************************
* Returns the bond topology property.
* Throws an exception if the input does not contain any bonds.
******************************************************************************/
BondProperty* ParticleInputHelper::expectBonds() const
{
	BondProperty* prop = inputStandardProperty<BondProperty>(BondProperty::TopologyProperty);
	if(!prop)
		dataset()->throwException(PropertyObject::tr("The modifier cannot be evaluated because the input does not contain any bonds."));
	return prop;
}

/******************************************************************************
* Returns a vector with the input particles colors.
******************************************************************************/
std::vector<Color> ParticleInputHelper::inputParticleColors(TimePoint time, TimeInterval& validityInterval)
{
	std::vector<Color> colors(inputParticleCount());

	// Obtain the particle vis element.
	if(ParticleProperty* positionProperty = inputStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty)) {
		for(DataVis* vis : positionProperty->visElements()) {
			if(ParticlesVis* particleVis = dynamic_object_cast<ParticlesVis>(vis)) {

				// Query particle colors from vis element.
				particleVis->particleColors(colors,
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
	// Obtain the bond vis element.
	if(BondProperty* topologyProperty = inputStandardProperty<BondProperty>(BondProperty::TopologyProperty)) {
		for(DataVis* vis : topologyProperty->visElements()) {
			if(BondsVis* bondsVis = dynamic_object_cast<BondsVis>(vis)) {

				// Additionally, look up the particle vis element.
				ParticlesVis* particleVis = nullptr;
				if(ParticleProperty* positionProperty = inputStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty)) {
					for(DataVis* vis2 : positionProperty->visElements()) {
						if((particleVis = dynamic_object_cast<ParticlesVis>(vis2)))
							break;
					}
				}

				// Query half-bond colors from vis element.
				std::vector<ColorA> halfBondColors = bondsVis->halfBondColors(
						inputParticleCount(), 
						inputStandardProperty<BondProperty>(BondProperty::TopologyProperty),
						inputStandardProperty<BondProperty>(BondProperty::ColorProperty),
						inputStandardProperty<BondProperty>(BondProperty::TypeProperty),
						inputStandardProperty<BondProperty>(BondProperty::SelectionProperty),
						nullptr, // No transparency needed here
						particleVis, 
						inputStandardProperty<ParticleProperty>(ParticleProperty::ColorProperty),
						inputStandardProperty<ParticleProperty>(ParticleProperty::TypeProperty));
				OVITO_ASSERT(inputBondCount() * 2 == halfBondColors.size());

				// Map half-bond colors to full bond colors.
				std::vector<Color> colors(inputBondCount());				
				auto ci = halfBondColors.cbegin();
				for(Color& co : colors) {
					co = Color(ci->r(), ci->g(), ci->b());
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

	// Obtain the particle vis element.
	if(ParticleProperty* positionProperty = inputStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty)) {
		for(DataVis* vis : positionProperty->visElements()) {
			if(ParticlesVis* particleVis = dynamic_object_cast<ParticlesVis>(vis)) {

				// Query particle radii from vis element.
				particleVis->particleRadii(radii,
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
