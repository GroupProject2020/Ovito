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

#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/ParticleType.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/series/DataSeriesObject.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include "StructureIdentificationModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_OVITO_CLASS(StructureIdentificationModifier);
DEFINE_REFERENCE_FIELD(StructureIdentificationModifier, structureTypes);
DEFINE_PROPERTY_FIELD(StructureIdentificationModifier, onlySelectedParticles);
DEFINE_PROPERTY_FIELD(StructureIdentificationModifier, colorByType);
SET_PROPERTY_FIELD_LABEL(StructureIdentificationModifier, structureTypes, "Structure types");
SET_PROPERTY_FIELD_LABEL(StructureIdentificationModifier, onlySelectedParticles, "Use only selected particles");
SET_PROPERTY_FIELD_LABEL(StructureIdentificationModifier, colorByType, "Color particles by type");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
StructureIdentificationModifier::StructureIdentificationModifier(DataSet* dataset) : AsynchronousModifier(dataset),
		_onlySelectedParticles(false),
		_colorByType(true)
{
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool StructureIdentificationModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	return input.containsObject<ParticlesObject>();
}

/******************************************************************************
* Create an instance of the ParticleType class to represent a structure type.
******************************************************************************/
ParticleType* StructureIdentificationModifier::createStructureType(int id, ParticleType::PredefinedStructureType predefType)
{
	OORef<ParticleType> stype(new ParticleType(dataset()));
	stype->setNumericId(id);
	stype->setName(ParticleType::getPredefinedStructureTypeName(predefType));
	stype->setColor(ParticleType::getDefaultParticleColor(ParticlesObject::StructureTypeProperty, stype->name(), id));
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
		if(type->numericId() >= 0 && type->numericId() < numTypes)
			typesToIdentify[type->numericId()] = type->enabled();
	}
	return typesToIdentify;
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
void StructureIdentificationModifier::StructureIdentificationEngine::emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	StructureIdentificationModifier* modifier = static_object_cast<StructureIdentificationModifier>(modApp->modifier());
	OVITO_ASSERT(modifier);

	ParticlesObject* particles = state.expectMutableObject<ParticlesObject>();

	if(_inputFingerprint.hasChanged(particles))
		modApp->throwException(tr("Cached modifier results are obsolete, because the number or the storage order of input particles has changed."));

	// Create output property object.
	PropertyPtr outputStructures = postProcessStructureTypes(time, modApp, structures());
	OVITO_ASSERT(outputStructures->size() == particles->elementCount());
	PropertyObject* structureProperty = particles->createProperty(outputStructures);

	// Attach structure types to output particle property.
	structureProperty->setElementTypes(modifier->structureTypes());

	if(modifier->colorByType()) {

		// Build structure type-to-color map.
		std::vector<Color> structureTypeColors(modifier->structureTypes().size());
		for(ElementType* stype : modifier->structureTypes()) {
			OVITO_ASSERT(stype->numericId() >= 0);
			if(stype->numericId() >= (int)structureTypeColors.size()) {
				structureTypeColors.resize(stype->numericId() + 1);
			}
			structureTypeColors[stype->numericId()] = stype->color();
		}

		// Assign colors to particles based on their structure type.
		PropertyObject* colorProperty = particles->createProperty(ParticlesObject::ColorProperty, false);
		const int* s = structureProperty->constDataInt();
		for(Color& c : colorProperty->colorRange()) {
			if(*s >= 0 && *s < structureTypeColors.size()) {
				c = structureTypeColors[*s];
			}
			else c.setWhite();
			++s;
		}
	}

	// Count the number of particles of each identified type.
	int maxTypeId = 0;
	for(ElementType* stype : modifier->structureTypes()) {
		OVITO_ASSERT(stype->numericId() >= 0);
		maxTypeId = std::max(maxTypeId, stype->numericId());
	}
	_typeCounts = std::make_shared<PropertyStorage>(maxTypeId + 1, PropertyStorage::Int64, 1, 0, tr("Count"), true, DataSeriesObject::YProperty);
	auto typeCountsData = _typeCounts->dataInt64();
	for(int t : structureProperty->constIntRange()) {
		if(t >= 0 && t <= maxTypeId)
			typeCountsData[t]++;
	}
	PropertyPtr typeIds = std::make_shared<PropertyStorage>(maxTypeId + 1, PropertyStorage::Int, 1, 0, tr("Structure type"), false, DataSeriesObject::XProperty);
	std::iota(typeIds->dataInt(), typeIds->dataInt() + typeIds->size(), 0);

	// Output a data series object with the type counts.
	DataSeriesObject* seriesObj = state.createObject<DataSeriesObject>(QStringLiteral("structures"), modApp, DataSeriesObject::BarChart, tr("Structure counts"), _typeCounts, std::move(typeIds));
#if 0
	PropertyObject* yProperty = seriesObj->expectMutableProperty(DataSeriesObject::YProperty);
	for(const ElementType* type : modifier->structureTypes()) {
		if(type->enabled())
			yProperty->addElementType(type);
	}
	seriesObj->setAxisLabelX(tr("Structure type"));
#else
	PropertyObject* xProperty = seriesObj->expectMutableProperty(DataSeriesObject::XProperty);
	for(const ElementType* type : modifier->structureTypes()) {
		if(type->enabled())
			xProperty->addElementType(type);
	}
#endif
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
