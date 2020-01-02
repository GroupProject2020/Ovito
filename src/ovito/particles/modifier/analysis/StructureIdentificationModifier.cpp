////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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
	particles->verifyIntegrity();

	if(_inputFingerprint.hasChanged(particles))
		modApp->throwException(tr("Cached modifier results are obsolete, because the number or the storage order of input particles has changed."));

	// Create output property object.
	PropertyPtr outputStructures = postProcessStructureTypes(time, modApp, structures());
	OVITO_ASSERT(outputStructures->size() == particles->elementCount());
	PropertyObject* structureProperty = particles->createProperty(outputStructures);
	ConstPropertyAccess<int> structureData(structureProperty);

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
		PropertyAccess<Color> colorProperty = particles->createProperty(ParticlesObject::ColorProperty, false);
		boost::transform(structureData, colorProperty.begin(), [&](int s) {
			if(s >= 0 && s < structureTypeColors.size())
				return structureTypeColors[s];
			else 
				return Color(1,1,1);
		});
	}

	// Count the number of particles of each identified type.
	int maxTypeId = 0;
	for(ElementType* stype : modifier->structureTypes()) {
		OVITO_ASSERT(stype->numericId() >= 0);
		maxTypeId = std::max(maxTypeId, stype->numericId());
	}
	_typeCounts.resize(maxTypeId + 1);
	boost::fill(_typeCounts, 0);
	for(int t : structureData) {
		if(t >= 0 && t <= maxTypeId)
			_typeCounts[t]++;
	}

	// Create the property arrays for the bar chart.
	PropertyPtr typeCounts = std::make_shared<PropertyStorage>(maxTypeId + 1, PropertyStorage::Int64, 1, 0, tr("Count"), false, DataSeriesObject::YProperty);
	boost::copy(_typeCounts, PropertyAccess<qlonglong>(typeCounts).begin());
	PropertyPtr typeIds = std::make_shared<PropertyStorage>(maxTypeId + 1, PropertyStorage::Int, 1, 0, tr("Structure type"), false, DataSeriesObject::XProperty);
	boost::algorithm::iota_n(PropertyAccess<int>(typeIds).begin(), 0, typeIds->size());

	// Output a bar chart with the type counts.
	DataSeriesObject* seriesObj = state.createObject<DataSeriesObject>(QStringLiteral("structures"), modApp, DataSeriesObject::BarChart, tr("Structure counts"), std::move(typeCounts), std::move(typeIds));

	// Use the structure types as labels for the output bar chart.
	PropertyObject* xProperty = seriesObj->expectMutableProperty(DataSeriesObject::XProperty);
	for(const ElementType* type : modifier->structureTypes()) {
		if(type->enabled())
			xProperty->addElementType(type);
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
