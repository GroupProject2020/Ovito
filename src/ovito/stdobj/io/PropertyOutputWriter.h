////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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


#include <ovito/stdobj/StdObj.h>
#include <ovito/stdobj/properties/PropertyStorage.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/stdobj/properties/PropertyContainer.h>
#include <ovito/stdobj/properties/PropertyReference.h>
#include <ovito/core/dataset/pipeline/PipelineFlowState.h>
#include <ovito/core/utilities/io/CompressedTextWriter.h>

namespace Ovito { namespace StdObj {

/**
 * \brief This class lists the properties to be written to an output file as data columns.
 *
 * This is simply a vector of PropertyReference instances. Each reference represents one column in the output file.
 */
class OVITO_STDOBJ_EXPORT OutputColumnMapping : public std::vector<PropertyReference>
{
public:

	using std::vector<PropertyReference>::size_type;

	/// Inherit constructors from std::vector.
	using std::vector<PropertyReference>::vector;

	/// \brief Saves the mapping to the given stream.
	void saveToStream(SaveStream& stream) const;

	/// \brief Loads the mapping from the given stream.
	void loadFromStream(LoadStream& stream);

	/// \brief Converts the mapping data into a byte array.
	QByteArray toByteArray() const;

	/// \brief Loads the mapping from a byte array.
	void fromByteArray(const QByteArray& array);
};

template<class PropertyContainerType>
class TypedOutputColumnMapping : public std::vector<TypedPropertyReference<PropertyContainerType>>
{
public:

	/// Inherit constructors from std::vector.
	using std::vector<TypedPropertyReference<PropertyContainerType>>::vector;

	/// Transparent conversion to an untyped OutputColumnMapping.
	operator OutputColumnMapping&() { return *reinterpret_cast<OutputColumnMapping*>(this); }

	/// Transparent conversion to an untyped OutputColumnMapping.
	operator const OutputColumnMapping&() const { return *reinterpret_cast<const OutputColumnMapping*>(this); }

	/// \brief Saves the mapping to the given stream.
	void saveToStream(SaveStream& stream) const { static_cast<const OutputColumnMapping&>(*this).saveToStream(stream); }

	/// \brief Loads the mapping from the given stream.
	void loadFromStream(LoadStream& stream) { static_cast<OutputColumnMapping&>(*this).loadFromStream(stream); }

	/// \brief Converts the mapping data into a byte array.
	QByteArray toByteArray() const { return static_cast<const OutputColumnMapping&>(*this).toByteArray(); }

	/// \brief Loads the mapping from a byte array.
	void fromByteArray(const QByteArray& array) { static_cast<OutputColumnMapping&>(*this).fromByteArray(array); }
};

/**
 * \brief Writes the data columns to the output file as specified by an OutputColumnMapping.
 */
class OVITO_STDOBJ_EXPORT PropertyOutputWriter : public QObject
{
public:

	/// These modes control how the values of typed properties are 
	/// written to the output file.
	enum TypedPropertyMode {
		WriteNumericIds,		///< Write the integer numeric ID of the type.
		WriteNamesUnmodified,	///< Write the type name as a string.
		WriteNamesUnderscore,	///< Write the type name as a string, with whitespace replaced with underscores.
		WriteNamesInQuotes		///< Write the type name as a string, in quotes if the name contains whitespace.
	};

	/// \brief Initializes the helper object.
	/// \param mapping The mapping between the properties and the columns in the output file.
	/// \param sourceContainer The data source container for the properties.
	/// \throws Exception if the mapping is not valid.
	///
	/// This constructor checks that all necessary properties referenced in the OutputColumnMapping
	/// are present in the source property container.
	PropertyOutputWriter(const OutputColumnMapping& mapping, const PropertyContainer* sourceContainer, TypedPropertyMode typedPropertyMode);

	/// \brief Writes the output line for a single data element to the output stream.
	/// \param index The index of the data element to write (starting at 0).
	/// \param stream An output text stream.
	void writeElement(size_t index, CompressedTextWriter& stream);

private:

	/// Stores the source properties for each column in the output file.
	/// If an entry is NULL, then the element index will be written to the corresponding column.
	QVector<const PropertyObject*> _properties;

	/// Stores the source vector component for each output column.
	QVector<int> _vectorComponents;

	/// Stores the memory buffer object for each output property.
	QVector<ConstPropertyAccess<void,true>> _propertyArrays;

	/// Controls how type names are output.
	TypedPropertyMode _typedPropertyMode;
};

}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::StdObj::OutputColumnMapping);
