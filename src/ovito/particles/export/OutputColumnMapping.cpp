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
#include "OutputColumnMapping.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Export)

/******************************************************************************
 * Saves the mapping to the given stream.
 *****************************************************************************/
void OutputColumnMapping::saveToStream(SaveStream& stream) const
{
	stream.beginChunk(0x01);
	stream << (int)size();
	for(const ParticlePropertyReference& col : *this) {
		stream << col;
	}
	stream.endChunk();
}

/******************************************************************************
 * Loads the mapping from the given stream.
 *****************************************************************************/
void OutputColumnMapping::loadFromStream(LoadStream& stream)
{
	stream.expectChunk(0x01);
	int numColumns;
	stream >> numColumns;
	resize(numColumns);
	for(ParticlePropertyReference& col : *this) {
		stream >> col;
	}
	stream.closeChunk();
}

/******************************************************************************
 * Saves the mapping into a byte array.
 *****************************************************************************/
QByteArray OutputColumnMapping::toByteArray() const
{
	QByteArray buffer;
	QDataStream dstream(&buffer, QIODevice::WriteOnly);
	SaveStream stream(dstream);
	saveToStream(stream);
	stream.close();
	return buffer;
}

/******************************************************************************
 * Loads the mapping from a byte array.
 *****************************************************************************/
void OutputColumnMapping::fromByteArray(const QByteArray& array)
{
	QDataStream dstream(array);
	LoadStream stream(dstream);
	loadFromStream(stream);
	stream.close();
}

/******************************************************************************
 * Initializes the writer object.
 *****************************************************************************/
OutputColumnWriter::OutputColumnWriter(const OutputColumnMapping& mapping, const PipelineFlowState& source, bool writeTypeNames)
	: _mapping(mapping), _source(source), _writeTypeNames(writeTypeNames)
{
	const ParticlesObject* particles = source.expectObject<ParticlesObject>();

	// Gather the source properties.
	for(int i = 0; i < (int)mapping.size(); i++) {
		const ParticlePropertyReference& pref = mapping[i];

		const PropertyObject* property = pref.findInContainer(particles);
		if(property == nullptr && pref.type() != ParticlesObject::IdentifierProperty) {
			throw Exception(tr("The specified list of output file columns is invalid. "
			                   "The property '%2', which is needed to write file column %1, does not exist or could not be computed.").arg(i+1).arg(pref.name()));
		}
		if(property) {
			if((int)property->componentCount() <= std::max(0, pref.vectorComponent()))
				throw Exception(tr("The output vector component selected for column %1 is out of range. The particle property '%2' has only %3 component(s).").arg(i+1).arg(pref.name()).arg(property->componentCount()));
			if(property->dataType() == QMetaType::Void)
				throw Exception(tr("The particle property '%1' cannot be written to the output file, because it is empty.").arg(pref.name()));
		}

		// Build internal list of property objects for fast look up during writing.
		_properties.push_back(property);
		_vectorComponents.push_back(std::max(0, pref.vectorComponent()));
		_propertyArrays.push_back(ConstPropertyAccess<void,true>(property));
	}
}

/******************************************************************************
 * Writes the data record for a single atom to the output stream.
 *****************************************************************************/
void OutputColumnWriter::writeParticle(size_t particleIndex, CompressedTextWriter& stream)
{
	QVector<const PropertyObject*>::const_iterator property = _properties.constBegin();
	QVector<int>::const_iterator vcomp = _vectorComponents.constBegin();
	QVector<ConstPropertyAccess<void,true>>::const_iterator array = _propertyArrays.constBegin();
	for(; property != _properties.constEnd(); ++property, ++vcomp, ++array) {
		if(property != _properties.constBegin()) stream << ' ';
		if(*property) {
			if((*property)->dataType() == PropertyStorage::Int) {
				if(!_writeTypeNames || (*property)->type() != ParticlesObject::TypeProperty) {
					stream << *reinterpret_cast<const int*>(array->cdata(particleIndex, *vcomp));
				}
				else {
					// Write type name instead of type number.
					// Replace spaces in the name with underscores.
					int particleTypeId = *reinterpret_cast<const int*>(array->cdata(particleIndex, *vcomp));
					const ElementType* type = (*property)->elementType(particleTypeId);
					if(type && !type->name().isEmpty()) {
						QString s = type->name();
						stream << s.replace(QChar(' '), QChar('_'));
					}
					else {
						stream << particleTypeId;
					}
				}
			}
			else if((*property)->dataType() == PropertyStorage::Int64) {
				stream << *reinterpret_cast<const qlonglong*>(array->cdata(particleIndex, *vcomp));
			}
			else if((*property)->dataType() == PropertyStorage::Float) {
				stream << *reinterpret_cast<const FloatType*>(array->cdata(particleIndex, *vcomp));
			}
			else {
				throw Exception(tr("The property '%1' cannot be written to the output file, because it has a non-standard data type.").arg((*property)->name()));
			}
		}
		else {
			stream << (particleIndex + 1);
		}
	}
	stream << '\n';
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
