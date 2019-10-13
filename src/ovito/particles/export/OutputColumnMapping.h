////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 Alexander Stukowski
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

#pragma once


#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/core/dataset/pipeline/PipelineFlowState.h>
#include <ovito/core/utilities/io/CompressedTextWriter.h>
#include <ovito/stdobj/properties/PropertyStorage.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Export)

/**
 * \brief This class lists the particle properties that should be written to an output file.
 *
 * This is simply a vector of ParticlePropertyReference instances. Each reference
 * represents one column in the output file.
 */
class OVITO_PARTICLES_EXPORT OutputColumnMapping : public std::vector<ParticlePropertyReference>
{
public:

	using std::vector<ParticlePropertyReference>::size_type;

	/// Default constructor.
	OutputColumnMapping() = default;

	/// Constructor.
	OutputColumnMapping(size_type size) : std::vector<ParticlePropertyReference>(size) {}

	/// Constructor.
	template<class InputIt>
	OutputColumnMapping(InputIt first, InputIt last) : std::vector<ParticlePropertyReference>(first, last) {}

	/// \brief Saves the mapping to the given stream.
	void saveToStream(SaveStream& stream) const;

	/// \brief Loads the mapping from the given stream.
	void loadFromStream(LoadStream& stream);

	/// \brief Converts the mapping data into a byte array.
	QByteArray toByteArray() const;

	/// \brief Loads the mapping from a byte array.
	void fromByteArray(const QByteArray& array);
};

/**
 * \brief Writes the data columns to the output file as specified by a OutputColumnMapping.
 */
class OutputColumnWriter : public QObject
{
public:

	/// \brief Initializes the helper object.
	/// \param mapping The mapping between the particle properties
	///                and the columns in the output file.
	/// \param source The data source for the particle properties.
	/// \throws Exception if the mapping is not valid.
	///
	/// This constructor checks that all necessary particle properties referenced in the OutputColumnMapping
	/// are present in the source object.
	OutputColumnWriter(const OutputColumnMapping& mapping, const PipelineFlowState& source, bool writeTypeNames = false);

	/// \brief Writes the output line for a single particle to the output stream.
	/// \param particleIndex The index of the particle to write (starting at 0).
	/// \param stream An output text stream.
	void writeParticle(size_t particleIndex, CompressedTextWriter& stream);

private:

	/// Determines how particle properties are written to which data columns of the output file.
	const OutputColumnMapping& _mapping;

	/// The data source.
	const PipelineFlowState& _source;

	/// Stores the source particle properties for each column in the output file.
	/// If an entry is NULL then the particle index will be written to the corresponding column.
	QVector<const PropertyObject*> _properties;

	/// Stores the source vector component for each output column.
	QVector<int> _vectorComponents;

	/// Controls whether type names are output in the particle type column instead of type numbers.
	bool _writeTypeNames;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::Particles::OutputColumnMapping);


