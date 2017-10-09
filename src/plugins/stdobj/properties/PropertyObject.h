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

#pragma once


#include <plugins/stdobj/StdObj.h>
#include <core/dataset/data/DataObject.h>
#include <plugins/stdobj/properties/ElementType.h>
#include "PropertyStorage.h"
#include "PropertyClass.h"

namespace Ovito { namespace StdObj {
	
/**
 * \brief Stores a property data array.
 */
class OVITO_STDOBJ_EXPORT PropertyObject : public DataObject
{
	Q_OBJECT
	OVITO_CLASS_META(PropertyObject, PropertyClass)
	
public:

	/// \brief Creates a property object.
	PropertyObject(DataSet* dataset);

	/// \brief Gets the property's name.
	/// \return The name of property, which is shown to the user.
	const QString& name() const { return storage()->name(); }

	/// \brief Sets the property's name.
	/// \param name The new name string.
	/// \undoable
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

	/// Reduces the size of the storage array, removing elements for which 
	/// the corresponding bits in the bit array are set.
	void filterResize(const boost::dynamic_bitset<>& mask) {
		modifiableStorage()->filterResize(mask);
		notifyDependents(ReferenceEvent::TargetChanged);
	}

	/// \brief Returns a read-only pointer to the raw elements stored in this property object.
	const void* constData() const {
		return storage()->constData();
	}

	/// \brief Returns a read-only pointer to the first integer element stored in this object.
	/// \note This method may only be used if this property is of data type integer.
	const int* constDataInt() const {
		return storage()->constDataInt();
	}

	/// \brief Returns a read-only pointer to the first integer element stored in this object.
	/// \note This method may only be used if this property is of data type 64-bit integer.
	const qlonglong* constDataInt64() const {
		return storage()->constDataInt64();
	}
	
	/// \brief Returns a read-only pointer to the first float element in the property storage.
	/// \note This method may only be used if this property is of data type float.
	const FloatType* constDataFloat() const {
		return storage()->constDataFloat();
	}

	/// \brief Returns a read-only pointer to the first vector element in the property storage.
	/// \note This method may only be used if this property is of data type Vector3 or a FloatType channel with 3 components.
	const Vector3* constDataVector3() const {
		return storage()->constDataVector3();
	}

	/// \brief Returns a read-only pointer to the first point element in the property storage.
	/// \note This method may only be used if this property is of data type Point3 or a FloatType channel with 3 components.
	const Point3* constDataPoint3() const {
		return storage()->constDataPoint3();
	}

	/// \brief Returns a read-only pointer to the first point element in the property storage.
	/// \note This method may only be used if this property is of data type Point3I or an integer channel with 3 components.
	const Point3I* constDataPoint3I() const {
		return storage()->constDataPoint3I();
	}

	/// \brief Returns a read-only pointer to the first point element in the property storage.
	/// \note This method may only be used if this property is of data type Color or a FloatType channel with 3 components.
	const Color* constDataColor() const {
		return storage()->constDataColor();
	}

	/// \brief Returns a read-only pointer to the first symmetric tensor element in the property storage.
	/// \note This method may only be used if this property is of data type SymmetricTensor2 or a FloatType channel with 6 components.
	const SymmetricTensor2* constDataSymmetricTensor2() const {
		return storage()->constDataSymmetricTensor2();
	}

	/// \brief Returns a read-only pointer to the first quaternion element in the property storage.
	/// \note This method may only be used if this property is of data type Quaternion or a FloatType channel with 4 components.
	const Quaternion* constDataQuaternion() const {
		return storage()->constDataQuaternion();
	}

	/// \brief Returns a range of const iterators over the elements stored in this object.
	boost::iterator_range<const int*> constIntRange() const {
		return storage()->constIntRange();
	}

	/// \brief Returns a range of const iterators over the elements stored in this object.
	boost::iterator_range<const qlonglong*> constInt64Range() const {
		return storage()->constInt64Range();
	}
	
	/// \brief Returns a range of const iterators over the elements stored in this object.
	boost::iterator_range<const FloatType*> constFloatRange() const {
		return storage()->constFloatRange();
	}

	/// \brief Returns a range of const iterators over the elements stored in this object.
	boost::iterator_range<const Point3*> constPoint3Range() const {
		return storage()->constPoint3Range();
	}

	/// \brief Returns a range of const iterators over the elements stored in this object.
	boost::iterator_range<const Vector3*> constVector3Range() const {
		return storage()->constVector3Range();
	}

	/// \brief Returns a range of const iterators over the elements stored in this object.
	boost::iterator_range<const Color*> constColorRange() const {
		return storage()->constColorRange();
	}

	/// \brief Returns a range of const iterators over the elements stored in this object.
	boost::iterator_range<const Point3I*> constPoint3IRange() const {
		return storage()->constPoint3IRange();
	}

	/// \brief Returns a range of const iterators over the elements stored in this object.
	boost::iterator_range<const SymmetricTensor2*> constSymmetricTensor2Range() const {
		return storage()->constSymmetricTensor2Range();
	}

	/// \brief Returns a range of const iterators over the elements stored in this object.
	boost::iterator_range<const Quaternion*> constQuaternionRange() const {
		return storage()->constQuaternionRange();
	}

	/// Returns a read-write pointer to the raw elements in the property storage.
	void* data() {
		return modifiableStorage()->data();
	}

	/// \brief Returns a read-write pointer to the first integer element stored in this object.
	/// \note This method may only be used if this property is of data type integer.
	int* dataInt() {
		return modifiableStorage()->dataInt();
	}

	/// \brief Returns a read-write pointer to the first integer element stored in this object.
	/// \note This method may only be used if this property is of data type 64-bit integer.
	qlonglong* dataInt64() {
		return modifiableStorage()->dataInt64();
	}
	
	/// \brief Returns a read-only pointer to the first float element in the property storage.
	/// \note This method may only be used if this property is of data type float.
	FloatType* dataFloat() {
		return modifiableStorage()->dataFloat();
	}

	/// \brief Returns a read-write pointer to the first vector element in the property storage.
	/// \note This method may only be used if this property is of data type Vector3 or a FloatType channel with 3 components.
	Vector3* dataVector3() {
		return modifiableStorage()->dataVector3();
	}

	/// \brief Returns a read-write pointer to the first point element in the property storage.
	/// \note This method may only be used if this property is of data type Point3 or a FloatType channel with 3 components.
	Point3* dataPoint3() {
		return modifiableStorage()->dataPoint3();
	}

	/// \brief Returns a read-write pointer to the first point element in the property storage.
	/// \note This method may only be used if this property is of data type Point3I or an integer channel with 3 components.
	Point3I* dataPoint3I() {
		return modifiableStorage()->dataPoint3I();
	}

	/// \brief Returns a read-write pointer to the first point element in the property storage.
	/// \note This method may only be used if this property is of data type Color or a FloatType channel with 3 components.
	Color* dataColor() {
		return modifiableStorage()->dataColor();
	}

	/// \brief Returns a read-write pointer to the first symmetric tensor element in the property storage.
	/// \note This method may only be used if this property is of data type SymmetricTensor2 or a FloatType channel with 6 components.
	SymmetricTensor2* dataSymmetricTensor2() {
		return modifiableStorage()->dataSymmetricTensor2();
	}

	/// \brief Returns a read-write pointer to the first quaternion element in the property storage.
	/// \note This method may only be used if this property is of data type Quaternion or a FloatType channel with 4 components.
	Quaternion* dataQuaternion() {
		return modifiableStorage()->dataQuaternion();
	}

	/// \brief Returns a range of iterators over the elements stored in this object.
	boost::iterator_range<int*> intRange() {
		return modifiableStorage()->intRange();
	}

	/// \brief Returns a range of iterators over the elements stored in this object.
	boost::iterator_range<qlonglong*> int64Range() {
		return modifiableStorage()->int64Range();
	}
	
	/// \brief Returns a range of iterators over the elements stored in this object.
	boost::iterator_range<FloatType*> floatRange() {
		return modifiableStorage()->floatRange();
	}

	/// \brief Returns a range of iterators over the elements stored in this object.
	boost::iterator_range<Point3*> point3Range() {
		return modifiableStorage()->point3Range();
	}

	/// \brief Returns a range of iterators over the elements stored in this object.
	boost::iterator_range<Vector3*> vector3Range() {
		return modifiableStorage()->vector3Range();
	}

	/// \brief Returns a range of const iterators over the elements stored in this object.
	boost::iterator_range<Color*> colorRange() {
		return modifiableStorage()->colorRange();
	}

	/// \brief Returns a range of iterators over the elements stored in this object.
	boost::iterator_range<Point3I*> point3IRange() {
		return modifiableStorage()->point3IRange();
	}

	/// \brief Returns a range of iterators over the elements stored in this object.
	boost::iterator_range<SymmetricTensor2*> symmetricTensor2Range() {
		return modifiableStorage()->symmetricTensor2Range();
	}

	/// \brief Returns a range of iterators over the elements stored in this object.
	boost::iterator_range<Quaternion*> quaternionRange() {
		return modifiableStorage()->quaternionRange();
	}

	/// \brief Returns an integer element at the given index (if this is an integer property).
	int getInt(size_t index) const {
		return storage()->getInt(index);
	}

	/// \brief Returns an integer element at the given index (if this is a 64-bit integer property).
	qlonglong getInt64(size_t index) const {
		return storage()->getInt64(index);
	}

	/// Returns a float element at the given index (if this is a float property).
	FloatType getFloat(size_t index) const {
		return storage()->getFloat(index);
	}

	/// Returns an integer element at the given index (if this is an integer property).
	int getIntComponent(size_t index, size_t componentIndex) const {
		return storage()->getIntComponent(index, componentIndex);
	}

	/// Returns an integer element at the given index (if this is a 64-bit integer property).
	qlonglong getInt64Component(size_t index, size_t componentIndex) const {
		return storage()->getInt64Component(index, componentIndex);
	}
	
	/// Returns a float element at the given index (if this is a float property).
	FloatType getFloatComponent(size_t index, size_t componentIndex) const {
		return storage()->getFloatComponent(index, componentIndex);
	}

	/// Returns a Vector3 element at the given index (if this is a vector property).
	const Vector3& getVector3(size_t index) const {
		return storage()->getVector3(index);
	}

	/// Returns a Point3 element at the given index (if this is a point property).
	const Point3& getPoint3(size_t index) const {
		return storage()->getPoint3(index);
	}

	/// Returns a Point3I element at the given index (if this is a point property).
	const Point3I& getPoint3I(size_t index) const {
		return storage()->getPoint3I(index);
	}

	/// Returns a Color element at the given index (if this is a point property).
	const Color& getColor(size_t index) const {
		return storage()->getColor(index);
	}

	/// Returns a SymmetricTensor2 element stored for the given particle.
	const SymmetricTensor2& getSymmetricTensor2(size_t index) const {
		return storage()->getSymmetricTensor2(index);
	}

	/// Returns a Quaternion element stored for the given particle.
	const Quaternion& getQuaternion(size_t index) const {
		return storage()->getQuaternion(index);
	}

	/// Sets the value of an integer element at the given index (if this is an integer property).
	void setInt(size_t index, int newValue) {
		modifiableStorage()->setInt(index, newValue);
	}

	/// Sets the value of an integer element at the given index (if this is a 64-bit integer property).
	void setInt64(size_t index, qlonglong newValue) {
		modifiableStorage()->setInt64(index, newValue);
	}
	
	/// Sets the value of a float element at the given index (if this is a float property).
	void setFloat(size_t index, FloatType newValue) {
		modifiableStorage()->setFloat(index, newValue);
	}

	/// Sets the value of an integer element at the given index (if this is an integer property).
	void setIntComponent(size_t index, size_t componentIndex, int newValue) {
		modifiableStorage()->setIntComponent(index, componentIndex, newValue);
	}

	/// Sets the value of an integer element at the given index (if this is a 64-bit integer property).
	void setInt64Component(size_t index, size_t componentIndex, qlonglong newValue) {
		modifiableStorage()->setInt64Component(index, componentIndex, newValue);
	}
	
	/// Sets the value of a float element at the given index (if this is a float property).
	void setFloatComponent(size_t index, size_t componentIndex, FloatType newValue) {
		modifiableStorage()->setFloatComponent(index, componentIndex, newValue);
	}

	/// Sets the value of a Vector3 element at the given index (if this is a vector property).
	void setVector3(size_t index, const Vector3& newValue) {
		modifiableStorage()->setVector3(index, newValue);
	}

	/// Sets the value of a Point3 element at the given index (if this is a point property).
	void setPoint3(size_t index, const Point3& newValue) {
		modifiableStorage()->setPoint3(index, newValue);
	}

	/// Sets the value of a Point3I element at the given index (if this is a point property).
	void setPoint3I(size_t index, const Point3I& newValue) {
		modifiableStorage()->setPoint3I(index, newValue);
	}

	/// Sets the value of a Color element at the given index (if this is a point property).
	void setColor(size_t index, const Color& newValue) {
		modifiableStorage()->setColor(index, newValue);
	}

	/// Sets the value of a SymmetricTensor2 element for the given particle.
	void setSymmetricTensor2(size_t index, const SymmetricTensor2& newValue) {
		modifiableStorage()->setSymmetricTensor2(index, newValue);
	}

	/// Sets the value of a Quaternion element for the given particle.
	void setQuaternion(size_t index, const Quaternion& newValue) {
		modifiableStorage()->setQuaternion(index, newValue);
	}

	//////////////////////////////// Element types //////////////////////////////
	
	/// Appends an element type to the list of types.
	void addElementType(ElementType* type) {
		OVITO_ASSERT(elementTypes().contains(type) == false);
		_elementTypes.push_back(this, PROPERTY_FIELD(elementTypes), type);
	}

	/// Inserts an element type into the list of types.
	void insertElementType(int index, ElementType* type) {
		OVITO_ASSERT(elementTypes().contains(type) == false);
		_elementTypes.insert(this, PROPERTY_FIELD(elementTypes), index, type);
	}

	/// Returns the element type with the given ID, or NULL if no such type exists.
	ElementType* elementType(int id) const {
		for(ElementType* type : elementTypes())
			if(type->id() == id)
				return type;
		return nullptr;
	}

	/// Returns the element type with the given human-readable name, or NULL if no such type exists.
	ElementType* elementType(const QString& name) const {
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
	
	/// Builds a mapping from element type identifier to type colors.
	std::map<int,Color> typeColorMap() const {
		std::map<int,Color> m;
		for(ElementType* type : elementTypes())
			m.insert({type->id(), type->color()});
		return m;
	}
	
	//////////////////////////////// from RefTarget //////////////////////////////

	/// Returns whether this object, when returned as an editable sub-object by another object,
	/// should be displayed in the modification stack.
	///
	/// This implementation returns true only it this is a typed property, i.e. if the 'elementTypes' list contains
	/// some elements. In this case we want the property to appear in the pipeline editor so that the user can
	/// edit the individual types.
	virtual bool isSubObjectEditable() const override { 
		return !elementTypes().empty();
	}

	/// Returns the display title of this property object in the user interface.
	virtual QString objectTitle() override;

protected:

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

private:

	/// The internal per-element data.
	DECLARE_RUNTIME_PROPERTY_FIELD(PropertyPtr, storage, setStorage);
	
	/// Contains the list of defined "types" if this is a typed property.
	DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD(ElementType, elementTypes, setElementTypes);
};

}	// End of namespace
}	// End of namespace
