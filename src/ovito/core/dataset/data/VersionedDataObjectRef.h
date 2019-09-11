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


#include <ovito/core/Core.h>
#include <ovito/core/dataset/data/DataObject.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem)

/**
 * \brief A weak reference (a.k.a. guarded pointer) that refers to a particular revision of a DataObject.
 *
 * Data objects can be modified and typically undergo changes. To make it possible for observers to detect such changes,
 * OVITO uses the system of 'object revision numbers'.
 *
 * Each object possesses an internal revision counter that is automatically incremented every time
 * the object is being modified in some way. This allows detecting changes made to an object without
 * explicitly comparing the stored data. In particular, this approach avoids saving a copy of the old data
 * to detect any changes to the object's internal state.
 *
 * The VersionedDataObjectRef class stores an ordinary guarded pointer (QPointer) to a DataObject instance and,
 * in addition, a revision number, which refers to a particular version (or state in time) of that object.
 *
 * Two VersionedDataObjectRef instances compare equal only when both the object pointers as well as the
 * object revision numbers match exactly.
 */
class VersionedDataObjectRef
{
public:

	/// Default constructor.
	VersionedDataObjectRef() Q_DECL_NOTHROW : _revision(std::numeric_limits<unsigned int>::max()) {}

	/// Initialization constructor.
	VersionedDataObjectRef(const DataObject* p) : _ref(const_cast<DataObject*>(p)), _revision(p ? p->revisionNumber() : std::numeric_limits<unsigned int>::max()) {}

	/// Initialization constructor with explicit revision number.
	VersionedDataObjectRef(const DataObject* p, unsigned int revision) : _ref(const_cast<DataObject*>(p)), _revision(revision) {}

	VersionedDataObjectRef& operator=(const DataObject* rhs) {
		_ref = const_cast<DataObject*>(rhs);
		_revision = rhs ? rhs->revisionNumber() : std::numeric_limits<unsigned int>::max();
		return *this;
	}

	void reset() Q_DECL_NOTHROW {
		_ref.clear();
		_revision = std::numeric_limits<unsigned int>::max();
	}

	void reset(const DataObject* rhs) {
		_ref = const_cast<DataObject*>(rhs);
		_revision = rhs ? rhs->revisionNumber() : std::numeric_limits<unsigned int>::max();
	}

	inline const DataObject* get() const Q_DECL_NOTHROW {
		return _ref.data();
	}

	inline operator const DataObject*() const Q_DECL_NOTHROW {
		return _ref.data();
	}

	inline const DataObject& operator*() const {
		return *_ref;
	}

	inline const DataObject* operator->() const {
		return _ref.data();
	}

	inline void swap(VersionedDataObjectRef& rhs) Q_DECL_NOTHROW {
		std::swap(_ref, rhs._ref);
		std::swap(_revision, rhs._revision);
	}

	inline unsigned int revisionNumber() const Q_DECL_NOTHROW { return _revision; }

	inline void updateRevisionNumber() Q_DECL_NOTHROW {
		if(_ref) _revision = _ref->revisionNumber();
	}

private:

	// The internal guarded pointer.
	QPointer<DataObject> _ref;

	// The referenced revision of the object.
	unsigned int _revision;
};

inline bool operator==(const VersionedDataObjectRef& a, const VersionedDataObjectRef& b) Q_DECL_NOTHROW {
	return a.get() == b.get() && a.revisionNumber() == b.revisionNumber();
}

inline bool operator!=(const VersionedDataObjectRef& a, const VersionedDataObjectRef& b) Q_DECL_NOTHROW {
	return a.get() != b.get() || a.revisionNumber() != b.revisionNumber();
}

inline bool operator==(const VersionedDataObjectRef& a, const DataObject* b) Q_DECL_NOTHROW {
	return a.get() == b && (b == nullptr || a.revisionNumber() == b->revisionNumber());
}

inline bool operator!=(const VersionedDataObjectRef& a, const DataObject* b) Q_DECL_NOTHROW {
	return a.get() != b || (b != nullptr && a.revisionNumber() != b->revisionNumber());
}

inline bool operator==(const DataObject* a, const VersionedDataObjectRef& b) Q_DECL_NOTHROW {
	return a == b.get() && (a == nullptr || a->revisionNumber() == b.revisionNumber());
}

inline bool operator!=(const DataObject* a, const VersionedDataObjectRef& b) Q_DECL_NOTHROW {
	return a != b.get() || (a != nullptr && a->revisionNumber() != b.revisionNumber());
}

inline bool operator==(const VersionedDataObjectRef& p, std::nullptr_t) Q_DECL_NOTHROW {
	return p.get() == nullptr;
}

inline bool operator==(std::nullptr_t, const VersionedDataObjectRef& p) Q_DECL_NOTHROW {
	return p.get() == nullptr;
}

inline bool operator!=(const VersionedDataObjectRef& p, std::nullptr_t) Q_DECL_NOTHROW {
	return p.get() != nullptr;
}

inline bool operator!=(std::nullptr_t, const VersionedDataObjectRef& p) Q_DECL_NOTHROW {
	return p.get() != nullptr;
}

inline void swap(VersionedDataObjectRef& lhs, VersionedDataObjectRef& rhs) Q_DECL_NOTHROW {
	lhs.swap(rhs);
}

inline const DataObject* get_pointer(const VersionedDataObjectRef& p) Q_DECL_NOTHROW {
	return p.get();
}

inline QDebug operator<<(QDebug debug, const VersionedDataObjectRef& p) {
	return debug << p.get();
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace


