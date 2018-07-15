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
#include <plugins/particles/modifier/ParticleOutputHelper.h>
#include <plugins/particles/objects/ParticleType.h>
#include <core/viewport/Viewport.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include "StructureIdentificationModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_OVITO_CLASS(StructureIdentificationModifier);
IMPLEMENT_OVITO_CLASS(StructureIdentificationModifierApplication);
DEFINE_REFERENCE_FIELD(StructureIdentificationModifier, structureTypes);
DEFINE_PROPERTY_FIELD(StructureIdentificationModifier, onlySelectedParticles);
SET_PROPERTY_FIELD_LABEL(StructureIdentificationModifier, structureTypes, "Structure types");
SET_PROPERTY_FIELD_LABEL(StructureIdentificationModifier, onlySelectedParticles, "Use only selected particles");
SET_MODIFIER_APPLICATION_TYPE(StructureIdentificationModifier, StructureIdentificationModifierApplication);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
StructureIdentificationModifier::StructureIdentificationModifier(DataSet* dataset) : AsynchronousModifier(dataset),
		_onlySelectedParticles(false)
{
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool StructureIdentificationModifier::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.findObject<ParticleProperty>() != nullptr;
}

/******************************************************************************
* Create an instance of the ParticleType class to represent a structure type.
******************************************************************************/
ParticleType* StructureIdentificationModifier::createStructureType(int id, ParticleType::PredefinedStructureType predefType)
{
	OORef<ParticleType> stype(new ParticleType(dataset()));
	stype->setId(id);
	stype->setName(ParticleType::getPredefinedStructureTypeName(predefType));
	stype->setColor(ParticleType::getDefaultParticleColor(ParticleProperty::StructureTypeProperty, stype->name(), id));
	addStructureType(stype);
	return stype;
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void StructureIdentificationModifier::saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData)
{
	AsynchronousModifier::saveToStream(stream, excludeRecomputableData);
	stream.beginChunk(0x02);
	// For future use.
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void StructureIdentificationModifier::loadFromStream(ObjectLoadStream& stream)
{
	AsynchronousModifier::loadFromStream(stream);
	stream.expectChunkRange(0, 2);
	// For future use.
	stream.closeChunk();
}

/******************************************************************************
* Returns a bit flag array which indicates what structure types to search for.
******************************************************************************/
QVector<bool> StructureIdentificationModifier::getTypesToIdentify(int numTypes) const
{
	QVector<bool> typesToIdentify(numTypes, true);
	for(ElementType* type : structureTypes()) {
		if(type->id() >= 0 && type->id() < numTypes)
			typesToIdentify[type->id()] = type->enabled();
	}
	return typesToIdentify;
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
PipelineFlowState StructureIdentificationModifier::StructureIdentificationEngine::emitResults(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	StructureIdentificationModifier* modifier = static_object_cast<StructureIdentificationModifier>(modApp->modifier());
	OVITO_ASSERT(modifier);

	if(_inputFingerprint.hasChanged(input))
		modApp->throwException(tr("Cached modifier results are obsolete, because the number or the storage order of input particles has changed."));

	PipelineFlowState output = input;
	ParticleOutputHelper poh(modApp->dataset(), output);

	// Create output property object.
	PropertyPtr outputStructures = postProcessStructureTypes(time, modApp, structures());
	OVITO_ASSERT(outputStructures->size() == poh.outputParticleCount());
	ParticleProperty* structureProperty = poh.outputProperty<ParticleProperty>(outputStructures);

	// Attach structure types to output particle property.
	structureProperty->setElementTypes(modifier->structureTypes());

	// Build structure type-to-color map.
	std::vector<Color> structureTypeColors(modifier->structureTypes().size());
	std::vector<size_t> typeCounters(modifier->structureTypes().size(), 0);
	for(ElementType* stype : modifier->structureTypes()) {
		OVITO_ASSERT(stype->id() >= 0);
		if(stype->id() >= (int)structureTypeColors.size()) {
			structureTypeColors.resize(stype->id() + 1);
			typeCounters.resize(stype->id() + 1, 0);
		}
		structureTypeColors[stype->id()] = stype->color();
	}

	// Assign colors to particles based on their structure type.
	ParticleProperty* colorProperty = poh.outputStandardProperty<ParticleProperty>(ParticleProperty::ColorProperty);
	const int* s = structureProperty->constDataInt();
	for(Color& c : colorProperty->colorRange()) {
		if(*s >= 0 && *s < structureTypeColors.size()) {
			c = structureTypeColors[*s];
			typeCounters[*s]++;
		}
		else c.setWhite();
		++s;
	}

	// Store the per-type counts in the ModifierApplication.
	static_object_cast<StructureIdentificationModifierApplication>(modApp)->setStructureCounts(std::move(typeCounters));

	return output;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
