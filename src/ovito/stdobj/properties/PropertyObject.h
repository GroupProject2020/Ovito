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
#include <ovito/core/dataset/data/DataObject.h>
#include <ovito/stdobj/properties/ElementType.h>
#include "PropertyStorage.h"

namespace Ovito { namespace StdObj {

/**
 * \brief Stores a property data array.
 */
class OVITO_STDOBJ_EXPORT PropertyObject : public DataObject
{
	/// Define a new property metaclass for particle containers.
	class OOMetaClass : public DataObject::OOMetaClass
	{
	public:
		/// Inherit constructor from base class.
		using DataObject::OOMetaClass::OOMetaClass;

		/// Generates a human-readable string representation of the data object reference.
		virtual QString formatDataObjectPath(const ConstDataObjectPath& path) const override;
	};

	Q_OBJECT
	OVITO_CLASS_META(PropertyObject, OOMetaClass);
	Q_CLASSINFO("DisplayName", "Property");

public:

	/// \brief Creates a property object.
	Q_INVOKABLE PropertyObject(DataSet* dataset, PropertyPtr storage = {});

	/// \brief Gets the property's name.
	/// \return The name of property, which is shown to the user.
	const QString& name() const { return storage()->name(); }

	/// \brief Sets the property's name.
	/// \param name The new name string.
	void setName(const QString& name);

	/// \brief Returns the number of elements.
	size_t size() const { return storage()->size(); }

	/// \brief Resizes the property storage.
	/// \param newSize The new number of elements.
	/// \param preserveData Controls whether the existing per-element data is preserved.
	///                     This also determines whether newly allocated memory is initialized to zero.
	void resize(size_t newSize, bool preserveData);

	/// \brief Returns the type of this property.
	int type() const { return storage()->type(); }

	/// \brief Returns the data type of the property.
	/// \return The identifier of the data type used for the elements stored in
	///         this property storage according to the Qt meta type system.
	int dataType() const { return storage()->dataType(); }

	/// \brief Returns the number of bytes per value.
	/// \return Number of bytes used to store a single value of the data type
	///         specified by dataType().
	size_t dataTypeSize() const { return storage()->dataTypeSize(); }

	/// \brief Returns the number of bytes used per particle.
	size_t stride() const { return storage()->stride(); }

	/// \brief Returns the number of values per element.
	/// \return The number of data values stored per element in this storage object.
	size_t componentCount() const { return storage()->componentCount(); }

	/// \brief Returns the human-readable names for the components of one element.
	/// \return The names of the components if this property contains more than one value per element.
	///         The list may be empty for scalar properties.
	const QStringList& componentNames() const { return storage()->componentNames(); }

	/// \brief Returns the display name of the property including the name of the given
	///        vector component.
	QString nameWithComponent(int vectorComponent) const {
		if(componentCount() <= 1 || vectorComponent < 0)
			return name();
		else if(vectorComponent < componentNames().size())
			return QStringLiteral("%1.%2").arg(name()).arg(componentNames()[vectorComponent]);
		else
			return QStringLiteral("%1.%2").arg(name()).arg(vectorComponent + 1);
	}

	/// Returns the data encapsulated by this object after making sure it is not shared with other owners.
	const PropertyPtr& modifiableStorage();

	/// Extends the data array and replicates the existing data N times.
	void replicate(size_t n, bool replicateValues = true);

	/// Reduces the size of the storage array, removing elements for which
	/// the corresponding bits in the bit array are set.
	void filterResize(const boost::dynamic_bitset<>& mask) {
		modifiableStorage()->filterResize(mask);
		notifyTargetChanged();
	}

	/// \brief Sets all array elements to the given uniform value.
	template<typename T>
	void fill(const T& value) {
		modifiableStorage()->fill(value);
	}

	/// \brief Sets all array elements for which the corresponding entries in the 
	///        selection array are non-zero to the given uniform value.
	template<typename T>
	void fillSelected(const T& value, const PropertyStorage& selectionProperty) {
		modifiableStorage()->fillSelected(value, selectionProperty);
	}

	/// \brief Sets all array elements for which the corresponding entries in the 
	///        selection array are non-zero to the given uniform value.
	template<typename T>
	void fillSelected(const T& value, const PropertyObject* selectionProperty) {
		if(selectionProperty)
			modifiableStorage()->fillSelected(value, *selectionProperty->storage());
		else
			modifiableStorage()->fill(value);
	}

	/// Copies the elements from the given source into this property array using a element mapping.
	void mappedCopyFrom(const PropertyObject* source, const std::vector<size_t>& mapping) {
		modifiableStorage()->mappedCopyFrom(*source->storage(), mapping);
	}

	/// Copies the elements from this property array into the given destination array using an index mapping.
	void mappedCopyTo(PropertyObject* destination, const std::vector<size_t>& mapping) const {
		storage()->mappedCopyTo(*destination->modifiableStorage(), mapping);
	}

	/// Copies the data elements from the given source array into this array. 
	/// Array size, component count and data type of source and destination must match exactly.
	void copyFrom(const PropertyObject* source) {
		modifiableStorage()->copyFrom(*source->storage());
	}

	/// Copies a range of data elements from the given source array into this array. 
	/// Component count and data type of source and destination must be compatible.
	void copyRangeFrom(const PropertyObject* source, size_t sourceIndex, size_t destIndex, size_t count) {
		modifiableStorage()->copyRangeFrom(*source->storage(), sourceIndex, destIndex, count);
	}

	//////////////////////////////// Element types //////////////////////////////

	/// Appends an element type to the list of types.
	void addElementType(const ElementType* type) {
		OVITO_ASSERT(elementTypes().contains(const_cast<ElementType*>(type)) == false);
		_elementTypes.push_back(this, PROPERTY_FIELD(elementTypes), type);
	}

	/// Inserts an element type into the list of types.
	void insertElementType(int index, const ElementType* type) {
		OVITO_ASSERT(elementTypes().contains(const_cast<ElementType*>(type)) == false);
		_elementTypes.insert(this, PROPERTY_FIELD(elementTypes), index, type);
	}

	/// Returns the element type with the given ID, or NULL if no such type exists.
	ElementType* elementType(int id) const {
		for(ElementType* type : elementTypes())
			if(type->numericId() == id)
				return type;
		return nullptr;
	}

	/// Returns the element type with the given human-readable name, or NULL if no such type exists.
	ElementType* elementType(const QString& name) const {
		OVITO_ASSERT(!name.isEmpty());
		for(ElementType* type : elementTypes())
			if(type->name() == name)
				return type;
		return nullptr;
	}

	/// Removes a single element type from this object.
	void removeElementType(int index) {
		_elementTypes.remove(this, PROPERTY_FIELD(elementTypes), index);
	}

	/// Removes all elements types from this object.
	void clearElementTypes() {
		_elementTypes.clear(this, PROPERTY_FIELD(elementTypes));
	}

	/// Builds a mapping from numeric IDs to type colors.
	std::map<int,Color> typeColorMap() const {
		std::map<int,Color> m;
		for(ElementType* type : elementTypes())
			m.insert({type->numericId(), type->color()});
		return m;
	}

//	std::map<int,FloatType> typeTransparencyMap() const {
//		std::map<int, FloatType> m;
//		for(ElementType* type : elementTypes())
//			m.insert({type->numericId(), type->transparency()});
///		return m;
//	}

	/// Returns an numeric type ID that is not yet used by any of the existing element types.
	int generateUniqueElementTypeId(int startAt = 1) const {
		int maxId = startAt;
		for(ElementType* type : elementTypes())
			maxId = std::max(maxId, type->numericId() + 1);
		return maxId;
	}

	/// Helper method that remaps the existing type IDs to a contiguous range starting at the given
	/// base ID. This method is mainly used for file output, because some file formats
	/// work with numeric particle types only, which must form a contiguous range.
	/// The method returns the mapping of output type IDs to original type IDs
	/// and a copy of the property array in which the original type ID values have
	/// been remapped to the output IDs.
	std::tuple<std::map<int,int>, ConstPropertyPtr> generateContiguousTypeIdMapping(int baseId = 1) const;

	////////////////////////////// Support functions for the Python bindings //////////////////////////////

	/// Indicates to the Python binding layer that this property object has been temporarily put into a
	/// writable state. In this state, the binding layer will allow write access to the property's internal data.
	bool isWritableFromPython() const { return _isWritableFromPython != 0; }

	/// Puts the property array into a writable state.
	/// In the writable state, the Python binding layer will allow write access to the property's internal data.
	void makeWritableFromPython();

	/// Puts the property array back into the default read-only state.
	/// In the read-only state, the Python binding layer will not permit write access to the property's internal data.
	void makeReadOnlyFromPython() {
		OVITO_ASSERT(_isWritableFromPython > 0);
		_isWritableFromPython--;
	}

	/// Returns whether this data object wants to be shown in the pipeline editor
	/// under the data source section.
	/// This implementation returns true only it this is a typed property, i.e. if the 'elementTypes' list contains
	/// some elements. In this case we want the property to appear in the pipeline editor so that the user can
	/// edit the individual types.
	virtual bool showInPipelineEditor() const override {
		return !elementTypes().empty();
	}

	/// Returns the display title of this property object in the user interface.
	virtual QString objectTitle() const override;

protected:

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// Is called when the value of a non-animatable field of this object changes.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

private:

	/// The internal per-element data.
	DECLARE_RUNTIME_PROPERTY_FIELD(PropertyPtr, storage, setStorage);

	/// Contains the list of defined "types" if this is a typed property.
	DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD(ElementType, elementTypes, setElementTypes);

	/// The user-interface title of this property.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, title, setTitle);

	/// This is a special flag used by the Python bindings to indicate that
	/// this property object has been temporarily put into a writable state.
	int _isWritableFromPython = 0;
};

}	// End of namespace
}	// End of namespace
