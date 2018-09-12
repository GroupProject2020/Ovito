///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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
#include <core/oo/OORef.h>
#include <core/oo/PropertyField.h>
#include <core/oo/RefTarget.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem)

/**
 * \brief Helper class that is used to clone a RefTarget derived object.
 *
 * To create a copy of an RefTarget derived object use this class.
 * The first step is to create an instance of the CloneHelper class on the stack
 * and then call cloneObject() to create a copy of the object passed to the method.
 *
 * You can either create deep or shallow copies of an object. For a deep copy all
 * sub-objects of the input object are also copied. For a shallow copy only the input object
 * itself is copied whereas all its sub-objects are only referenced by the cloned object.
 *
 * For a RefTarget derived class to be clonable it must implement the RefTarget::clone() method.
 * The CloneHelper used to clone an object is passed to its \c clone() method. You never call the
 * RefTarget::clone() method directly because only the CloneHelper makes sure that an object
 * in the object graph is copied only once during one clone operation.
 *
 * Implementations of the RefTarget::clone() method should use the copyReference() method to clone
 * sub-object references. This methods interprets the \c deepCopy parameter by returning the unmodified
 * input object if \c deepCopy==false.
 */
class OVITO_CORE_EXPORT CloneHelper
{
public:

	/// \brief This creates a copy of a RefTarget derived object.
	/// \param obj The input object to be cloned. Can be a \c NULL pointer.
	/// \param deepCopy Specifies whether a deep or a shallow copy of the object should be created.
	///                 The exact interpretation of this parameter depends on the implementation
	///                 of the RefTarget::clone() method of the object. For a deep copy the complete
	///                 object graph is duplicated including sub-objects. For a shallow copy the clone
	///                 of the input object will reference the same sub-objects as the original one.
	/// \return The clone of the input object or \c NULL if \a obj was \c NULL.
	/// \note If this CloneHelper instance has already been used to create a copy of the
	///       input object \a obj, then the existing clone of this object is returned.
	template<class T>
	OORef<T> cloneObject(const T* obj, bool deepCopy) {
		RefTarget* p = cloneObjectImpl(obj, deepCopy);
		OVITO_ASSERT_MSG(!p || p->getOOClass().isDerivedFrom(T::OOClass()), "CloneHelper::cloneObject", qPrintable("The clone method of class " + obj->getOOClass().name() + " did not return an assignable instance of the class " + T::OOClass().name() + "."));
		return static_object_cast<T>(p);
	}

	/// \brief This creates a copy of a RefTarget derived object.
	/// \param obj The input object to be cloned. Can be a \c NULL pointer.
	/// \param deepCopy Specifies whether a deep or a shallow copy of the object should be created.
	///                 The exact interpretation of this parameter depends on the implementation
	///                 of the RefTarget::clone() method of the object. For a deep copy the complete
	///                 object graph is duplicated including sub-objects. For a shallow copy the clone
	///                 of the input object will reference the same sub-objects as the original one.
	/// \return The clone of the input object or \c NULL if \a obj was \c NULL.
	/// \note If this CloneHelper instance has already been used to create a copy of the
	///       input object \a obj, then the existing clone of this object is returned.
	template<class T>
	OORef<T> cloneObject(const OORef<T>& obj, bool deepCopy) {
		return cloneObject(obj.get(), deepCopy);
	}

	/// \brief Can be used to copy a sub-object reference.
	/// \param obj The sub-object to be cloned. Can be \c NULL.
	/// \param deepCopy Specifies whether a deep copy of the input object should be performed.
	/// \return A deep copy of the input object if \a deepCopy is \c true or just the unmodified input
	///         object \a obj if \a deepCopy is \c false.
	///
	/// This method creates a real copy of the source object only if \a deepCopy is \c true.
	/// Otherwise the original object is returned.
	///
	/// This method can be used in implementations of the RefTarget::clone() method
	/// to copy/transfer references to sub-objects for deep copies as well as shallow copies.
	template<class T>
	OORef<T> copyReference(const T* obj, bool deepCopy) {
		if(!deepCopy) return const_cast<T*>(obj);
		return cloneObject(obj, true);
	}

private:

	/// Untyped version of the clone function.
	RefTarget* cloneObjectImpl(const RefTarget* obj, bool deepCopy);

	/// The table of clones created by this helper object.
	QVarLengthArray<std::pair<const RefMaker*, OORef<RefTarget>>, 2> _cloneTable;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace



