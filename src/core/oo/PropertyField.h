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


#include <core/Core.h>
#include <core/oo/OvitoObject.h>
#include <core/dataset/UndoStack.h>
#include <core/oo/PropertyFieldDescriptor.h>
#include <core/oo/ReferenceEvent.h>

#include <boost/type_traits/has_equal_to.hpp>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem)

/**
 * \brief RefMaker derived classes use this implement properties and reference fields.
 */
class OVITO_CORE_EXPORT PropertyFieldBase
{
protected:

	/// Generates a notification event to inform the dependents of the field's owner that it has changed.
	void generateTargetChangedEvent(RefMaker* owner, const PropertyFieldDescriptor& descriptor, ReferenceEvent::Type eventType = ReferenceEvent::TargetChanged);

	/// Generates a notification event to inform the dependents of the field's owner that it has changed.
	void generatePropertyChangedEvent(RefMaker* owner, const PropertyFieldDescriptor& descriptor) const;

	/// Indicates whether undo records should be created.
	bool isUndoRecordingActive(RefMaker* owner, const PropertyFieldDescriptor& descriptor) const;

	/// Puts a record on the undo stack.
	static void pushUndoRecord(RefMaker* owner, std::unique_ptr<UndoableOperation>&& operation);

protected:

	/// This abstract undo record class keeps a strong reference object whose property has been changed.
	/// This is needed to keep the owner object alive as long as this undo record is on the undo stack.
	class OVITO_CORE_EXPORT PropertyFieldOperation : public UndoableOperation
	{
	public:
		/// Constructor.
		PropertyFieldOperation(RefMaker* owner, const PropertyFieldDescriptor& descriptor);
		/// Access to the object whose property was changed.
		RefMaker* owner() const;
		/// Access to the descriptor of the reference field whose value has changed.
		const PropertyFieldDescriptor& descriptor() const { return _descriptor; }
	private:
		/// The object whose property has been changed.
		/// This is only used if the owner is not the DataSet, because that would create a circular reference.
		OORef<OvitoObject> _owner;
		/// The descriptor of the reference field whose value has changed.
		const PropertyFieldDescriptor& _descriptor;
	};
};

/**
 * \brief Stores a non-animatable property of a RefTarget derived class, which is not serializable.
 */
template<typename property_data_type>
class RuntimePropertyField : public PropertyFieldBase
{
public:
	using property_type = property_data_type;

	// Pick the QVariant data type used to wrap the property type.
	// If the property type is an enum, then use 'int'.
	// If the property is 'Color', then use 'QColor'.
	// Otherwise just use the property type.
	using qvariant_type = std::conditional_t<std::is_enum<property_data_type>::value, int,
		std::conditional_t<std::is_same<property_data_type, Color>::value, QColor, property_data_type>>;

	// For enum types, the QVariant data type must always be set to 'int'.
	static_assert(!std::is_enum<property_type>::value || std::is_same<qvariant_type, int>::value, "QVariant data type must be 'int' for enum property types.");
	// For color properties, the QVariant data type must always be set to 'QColor'.
	static_assert(!std::is_same<property_type, Color>::value || std::is_same<qvariant_type, QColor>::value, "QVariant data type must be 'QColor' for color property types.");

	/// Forwarding constructor.
	template<class... Args>
	explicit RuntimePropertyField(Args&&... args) : PropertyFieldBase(), _value(std::forward<Args>(args)...) {}

	/// Changes the value of the property. Handles undo and sends a notification message.
	template<typename T>
	void set(RefMaker* owner, const PropertyFieldDescriptor& descriptor, T&& newValue) {
		OVITO_ASSERT(owner != nullptr);
		if(isEqualToCurrentValue(get(), newValue)) return;
		if(isUndoRecordingActive(owner, descriptor))
			pushUndoRecord(owner, std::make_unique<PropertyChangeOperation>(owner, *this, descriptor));
		mutableValue() = std::forward<T>(newValue);
		valueChangedInternal(owner, descriptor);
	}

	/// Changes the value of the property. Handles undo and sends a notification message.
	template<typename T = qvariant_type>
	std::enable_if_t<QMetaTypeId2<T>::Defined>
	setQVariant(RefMaker* owner, const PropertyFieldDescriptor& descriptor, const QVariant& newValue) {
		if(newValue.canConvert<qvariant_type>()) {
			set(owner, descriptor, static_cast<property_type>(newValue.value<qvariant_type>()));
		}
		else {
			OVITO_ASSERT_MSG(false, "PropertyField::setQVariant()", "The assigned QVariant value cannot be converted to the data type of the property field.");
		}
	}

	/// Changes the value of the property. Handles undo and sends a notification message.
	template<typename T = qvariant_type>
	std::enable_if_t<!QMetaTypeId2<T>::Defined>
	setQVariant(RefMaker* owner, const PropertyFieldDescriptor& descriptor, const QVariant& newValue) {
		OVITO_ASSERT_MSG(false, "RuntimePropertyField::setQVariant()", "The data type of the property field does not support conversion to/from QVariant.");
	}

	/// Returns the internal value stored in this property field as a QVariant.
	template<typename T = qvariant_type>
	std::enable_if_t<QMetaTypeId2<T>::Defined, QVariant>
	getQVariant() const {
		return QVariant::fromValue<qvariant_type>(static_cast<qvariant_type>(this->get()));
	}

	/// Returns the internal value stored in this property field as a QVariant.
	template<typename T = qvariant_type>
	std::enable_if_t<!QMetaTypeId2<T>::Defined, QVariant>
	getQVariant() const {
		OVITO_ASSERT_MSG(false, "RuntimePropertyField::getQVariant()", "The data type of the property field does not support conversion to/from QVariant.");
		return {};
	}

	/// Returns the internal value stored in this property field.
	inline const property_type& get() const { return _value; }

	/// Returns a reference to the internal value stored in this property field.
	/// Warning: Do not use this function unless you know what you are doing!
	inline property_type& mutableValue() { return _value; }

	/// Cast the property field to the property value.
	inline operator const property_type&() const { return get(); }

private:

	/// Internal helper function that generates notification events.
	void valueChangedInternal(RefMaker* owner, const PropertyFieldDescriptor& descriptor) {
		generatePropertyChangedEvent(owner, descriptor);
		generateTargetChangedEvent(owner, descriptor);
		if(descriptor.extraChangeEventType() != 0)
			generateTargetChangedEvent(owner, descriptor, static_cast<ReferenceEvent::Type>(descriptor.extraChangeEventType()));
	}

	/// Helper function that tests if the new value is equal to the current value of the property field.
	template<typename T = property_type>
	static inline std::enable_if_t<boost::has_equal_to<const T&>::value, bool>
	isEqualToCurrentValue(const T& oldValue, const T& newValue) { return oldValue == newValue; }

	/// Helper function that tests if the new value is equal to the current value of the property field.
	template<typename T = property_type>
	static inline std::enable_if_t<!boost::has_equal_to<const T&>::value, bool>
	isEqualToCurrentValue(const T& oldValue, const T& newValue) { return false; }

	/// This undo class records a change to the property value.
	class PropertyChangeOperation : public PropertyFieldOperation
	{
	public:

		/// Constructor.
		/// Makes a copy of the current property value.
		PropertyChangeOperation(RefMaker* owner, RuntimePropertyField& field, const PropertyFieldDescriptor& descriptor) :
			PropertyFieldOperation(owner, descriptor), _field(field), _oldValue(field.get()) {}

		/// Restores the old property value.
		virtual void undo() override {
			// Swap old value and current property value.
			using std::swap; // using ADL here
			swap(_field.mutableValue(), _oldValue);
			_field.valueChangedInternal(owner(), descriptor());
		}

	private:

		/// The property field that has been changed.
		RuntimePropertyField& _field;
		/// The old value of the property.
		property_type _oldValue;
	};

	/// The internal property value.
	property_type _value;
};

/**
 * \brief Stores a non-animatable property of a RefTarget derived class.
 */
template<typename property_data_type>
class PropertyField : public RuntimePropertyField<property_data_type>
{
public:
	using property_type = property_data_type;

	/// Constructor.
	using RuntimePropertyField<property_data_type>::RuntimePropertyField;

	/// Saves the property's value to a stream.
	inline void saveToStream(SaveStream& stream) const {
		stream << this->get();
	}

	/// Loads the property's value from a stream.
	inline void loadFromStream(LoadStream& stream) {
		stream >> this->mutableValue();
	}
};

// Template specialization for size_t property fields.
template<>
inline void PropertyField<size_t>::saveToStream(SaveStream& stream) const {
	stream.writeSizeT(this->get());
}

// Template specialization for size_t property fields.
template<>
inline void PropertyField<size_t>::loadFromStream(LoadStream& stream) {
	stream.readSizeT(this->mutableValue());
}

/**
 * \brief Manages a pointer to a RefTarget derived class held by a RefMaker derived class.
 */
class OVITO_CORE_EXPORT SingleReferenceFieldBase : public PropertyFieldBase
{
public:

	/// Constructor.
	SingleReferenceFieldBase() = default;

	/// Returns the RefTarget pointer.
	inline operator RefTarget*() const {
		return _pointer;
	}

	/// Returns the RefTarget pointer.
	inline RefTarget* getInternal() const {
		return _pointer;
	}

	/// Replaces the reference target.
	void setInternal(RefMaker* owner, const PropertyFieldDescriptor& descriptor, const RefTarget* newTarget);

protected:

	/// Replaces the target stored in the reference field.
	void swapReference(RefMaker* owner, const PropertyFieldDescriptor& descriptor, OORef<RefTarget>& inactiveTarget, bool generateNotificationEvents = true);

	/// The actual pointer to the reference target.
	RefTarget* _pointer = nullptr;

	friend class RefMaker;
	friend class RefTarget;

protected:

	class OVITO_CORE_EXPORT SetReferenceOperation : public PropertyFieldOperation
	{
	private:
		/// The reference target that is currently not assigned to the reference field.
		/// This is stored here so that we can restore it on a call to undo().
		OORef<RefTarget> _inactiveTarget;
		/// The reference field whose value has changed.
		SingleReferenceFieldBase& _reffield;
	public:
		SetReferenceOperation(RefMaker* owner, RefTarget* oldTarget, SingleReferenceFieldBase& reffield, const PropertyFieldDescriptor& descriptor);
		virtual void undo() override { _reffield.swapReference(owner(), descriptor(), _inactiveTarget); }
		virtual QString displayName() const override;
	};
};

/**
 * \brief Templated version of the SingleReferenceFieldBase class.
 */
template<typename RefTargetType>
class ReferenceField : public SingleReferenceFieldBase
{
public:
	using target_type = RefTargetType;

#ifdef OVITO_DEBUG
	/// Destructor that releases all referenced objects.
	~ReferenceField() {
		if(_pointer)
			qDebug() << "Reference field value:" << _pointer;
		OVITO_ASSERT_MSG(!_pointer, "~ReferenceField()", "Owner object of reference field has not been deleted correctly. The reference field was not empty when the class destructor was called.");
	}
#endif

	/// Read access to the RefTarget derived pointer.
	operator RefTargetType*() const { return reinterpret_cast<RefTargetType*>(_pointer); }

	/// Write access to the RefTarget pointer. Changes the value of the reference field.
	/// The old reference target will be released and the new reference target
	/// will be bound to this reference field.
	/// This operator automatically handles undo so the value change can be undone.
	void set(RefMaker* owner, const PropertyFieldDescriptor& descriptor, const RefTargetType* newPointer) {
		SingleReferenceFieldBase::setInternal(owner, descriptor, newPointer);
	}

	/// Overloaded arrow operator; implements pointer semantics.
	/// Just use this operator as you would with a normal C++ pointer.
	RefTargetType* operator->() const {
		OVITO_ASSERT_MSG(_pointer, "ReferenceField operator->", "Tried to make a call to a NULL pointer.");
		return reinterpret_cast<RefTargetType*>(_pointer);
	}

	/// Dereference operator; implements pointer semantics.
	/// Just use this operator as you would with a normal C++ pointer.
	RefTargetType& operator*() const {
		OVITO_ASSERT_MSG(_pointer, "ReferenceField operator*", "Tried to dereference a NULL pointer.");
		return *reinterpret_cast<RefTargetType*>(_pointer);
	}

	/// Returns true if the internal pointer is non-null.
	operator bool() const { return _pointer != nullptr; }
};

/// \brief Dynamic casting function for reference fields.
///
/// Returns the given object cast to type \c T if the object is of type \c T
/// (or of a subclass); otherwise returns \c NULL.
///
/// \relates ReferenceField
template<class T, class U>
inline T* dynamic_object_cast(const ReferenceField<U>& field) {
	return dynamic_object_cast<T,U>(field.value());
}

/**
 * \brief Manages a list of references to RefTarget objects held by a RefMaker derived class.
 */
class OVITO_CORE_EXPORT VectorReferenceFieldBase : public PropertyFieldBase
{
public:

	/// Returns the stored references as a QVector.
	operator const QVector<RefTarget*>&() const { return pointers; }

	/// Returns the reference target at index position i.
	RefTarget* operator[](int i) const { return pointers[i]; }

	/// Returns the number of objects in the vector reference field.
	int size() const { return pointers.size(); }

	/// Returns true if the vector has size 0; otherwise returns false.
	bool empty() const { return pointers.empty(); }

	/// Returns true if the vector contains an occurrence of value; otherwise returns false.
	bool contains(const RefTarget* value) const { return pointers.contains(const_cast<RefTarget*>(value)); }

	/// Returns the index position of the first occurrence of value in the vector,
	/// searching forward from index position from. Returns -1 if no item matched.
	int indexOf(const RefTarget* value, int from = 0) const { return pointers.indexOf(const_cast<RefTarget*>(value), from); }

	/// Returns the stored references as a QVector.
	const QVector<RefTarget*>& targets() const { return pointers; }

	/// Clears all references at sets the vector size to zero.
	void clear(RefMaker* owner, const PropertyFieldDescriptor& descriptor);

	/// Removes the element at index position i.
	void remove(RefMaker* owner, const PropertyFieldDescriptor& descriptor, int i);

	/// Replaces a reference in the vector.
	/// This method removes the reference at index i and inserts the new reference at the same index.
	void setInternal(RefMaker* owner, const PropertyFieldDescriptor& descriptor, int i, const RefTarget* object) {
		OVITO_ASSERT(i >= 0 && i < size());
		if(targets()[i] != object) {
			remove(owner, descriptor, i);
			insertInternal(owner, descriptor, object, i);
		}
	}

protected:

	/// The actual pointer list to the reference targets.
	QVector<RefTarget*> pointers;

	/// Adds a reference target to the internal list.
	int insertInternal(RefMaker* owner, const PropertyFieldDescriptor& descriptor, const RefTarget* newTarget, int index = -1);

	/// Removes a target from the list reference field.
	OORef<RefTarget> removeReference(RefMaker* owner, const PropertyFieldDescriptor& descriptor, int index, bool generateNotificationEvents = true);

	/// Adds the target to the list reference field.
	int addReference(RefMaker* owner, const PropertyFieldDescriptor& descriptor, const OORef<RefTarget>& target, int index);

protected:

	class OVITO_CORE_EXPORT InsertReferenceOperation : public PropertyFieldOperation
	{
	public:
    	InsertReferenceOperation(RefMaker* owner, RefTarget* target, VectorReferenceFieldBase& reffield, int index, const PropertyFieldDescriptor& descriptor);

		virtual void undo() override {
			OVITO_ASSERT(!_target);
			_target = _reffield.removeReference(owner(), descriptor(), _index);
		}

		virtual void redo() override {
			_index = _reffield.addReference(owner(), descriptor(), _target, _index);
			_target.reset();
		}

		int insertionIndex() const { return _index; }

		virtual QString displayName() const override;

	private:
		/// The target that has been added into the vector reference field.
	    OORef<RefTarget> _target;
		/// The vector reference field to which the reference has been added.
		VectorReferenceFieldBase& _reffield;
		/// The position at which the target has been inserted into the vector reference field.
		int _index;
	};

	class OVITO_CORE_EXPORT RemoveReferenceOperation : public PropertyFieldOperation
	{
	public:
    	RemoveReferenceOperation(RefMaker* owner, VectorReferenceFieldBase& reffield, int index, const PropertyFieldDescriptor& descriptor);

		virtual void undo() override {
			_index = _reffield.addReference(owner(), descriptor(), _target, _index);
			_target.reset();
		}

		virtual void redo() override {
			OVITO_ASSERT(!_target);
			_target = _reffield.removeReference(owner(), descriptor(), _index);
		}

		virtual QString displayName() const override;

	private:
		/// The target that has been removed from the vector reference field.
	    OORef<RefTarget> _target;
		/// The vector reference field from which the reference has been removed.
		VectorReferenceFieldBase& _reffield;
		/// The position at which the target has been removed from the vector reference field.
		int _index;
	};

	friend class RefMaker;
	friend class RefTarget;
};

/**
 * \brief Templated version of the VectorReferenceFieldBase class.
 */
template<typename RefTargetType>
class VectorReferenceField : public VectorReferenceFieldBase
{
public:
	using target_type = RefTargetType;

	typedef QVector<RefTargetType*> RefTargetVector;
	typedef QVector<const RefTargetType*> ConstRefTargetVector;
	typedef typename RefTargetVector::ConstIterator ConstIterator;
	typedef typename RefTargetVector::const_iterator const_iterator;
	typedef typename RefTargetVector::const_pointer const_pointer;
	typedef typename RefTargetVector::const_reference const_reference;
	typedef typename RefTargetVector::difference_type difference_type;
	typedef typename RefTargetVector::size_type size_type;
	typedef typename RefTargetVector::value_type value_type;

#ifdef OVITO_DEBUG
	/// Destructor that releases all referenced objects.
	~VectorReferenceField() {
		OVITO_ASSERT_MSG(pointers.empty(), "~VectorReferenceField()", "Owner object of vector reference field has not been deleted correctly. The reference field was not empty when the class destructor was called.");
	}
#endif

	/// Returns the stored references as a QVector.
	operator const RefTargetVector&() const { return targets(); }

	/// Returns the reference target at index position i.
	RefTargetType* operator[](int i) const { return static_object_cast<RefTargetType>(pointers[i]); }

	/// Inserts a reference at the end of the vector.
	void push_back(RefMaker* owner, const PropertyFieldDescriptor& descriptor, const RefTargetType* object) { insertInternal(owner, descriptor, object); }

	/// Inserts a reference at index position i in the vector.
	/// If i is 0, the value is prepended to the vector.
	/// If i is size() or negative, the value is appended to the vector.
	void insert(RefMaker* owner, const PropertyFieldDescriptor& descriptor, int i, const RefTargetType* object) { insertInternal(owner, descriptor, object, i); }

	/// Replaces a reference in the vector.
	/// This method removes the reference at index i and inserts the new reference at the same index.
	void set(RefMaker* owner, const PropertyFieldDescriptor& descriptor, int i, const RefTargetType* object) {
		setInternal(owner, descriptor, i, object);
	}

	/// Returns an STL-style iterator pointing to the first item in the vector.
	const_iterator begin() const { return targets().begin(); }

	/// Returns an STL-style iterator pointing to the imaginary item after the last item in the vector.
	const_iterator end() const { return targets().end(); }

	/// Returns an STL-style iterator pointing to the first item in the vector.
	const_iterator constBegin() const { return begin(); }

	/// Returns a const STL-style iterator pointing to the imaginary item after the last item in the vector.
	const_iterator constEnd() const { return end(); }

	/// Returns the first reference stored in this vector reference field.
	RefTargetType* front() const { return targets().front(); }

	/// Returns the last reference stored in this vector reference field.
	RefTargetType* back() const { return targets().back(); }

	/// Finds the first object stored in this vector reference field that is of the given type.
	/// or can be cast to the given type. Returns NULL if no such object is in the list.
	template<class Clazz>
	Clazz* firstOf() const {
		for(const_iterator i = constBegin(); i != constEnd(); ++i) {
			Clazz* obj = dynamic_object_cast<Clazz>(*i);
			if(obj) return obj;
		}
		return nullptr;
	}

	/// Copies the references of another vector reference field.
	void set(RefMaker* owner, const PropertyFieldDescriptor& descriptor, const VectorReferenceField& other) {
		for(int i = 0; i < other.size() && i < this->size(); i++)
			set(owner, descriptor, i, other[i]);
		for(int i = this->size(); i < other.size(); i++)
			push_back(owner, descriptor, other[i]);
		for(int i = this->size() - 1; i >= other.size(); i--)
			remove(owner, descriptor, i);
	}

	/// Assigns the given list of targets to this vector reference field.
	void set(RefMaker* owner, const PropertyFieldDescriptor& descriptor, const RefTargetVector& other) {
		for(int i = 0; i < other.size() && i < this->size(); i++)
			set(owner, descriptor, i, other[i]);
		for(int i = this->size(); i < other.size(); i++)
			push_back(owner, descriptor, other[i]);
		for(int i = this->size() - 1; i >= other.size(); i--)
			remove(owner, descriptor, i);
	}

	/// Assigns the given list of targets to this vector reference field.
	void set(RefMaker* owner, const PropertyFieldDescriptor& descriptor, const ConstRefTargetVector& other) {
		set(owner, descriptor, reinterpret_cast<const RefTargetVector&>(other));
	}

	/// Returns the stored references as a QVector.
	const RefTargetVector& targets() const { return reinterpret_cast<const RefTargetVector&>(pointers); }
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
