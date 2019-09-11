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
#include <ovito/core/oo/OORef.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem)

/**
 * \brief Internal class used by PipelineFlowState to keep track of how many flow states
 *        refer to a particular DataObject instance.
 */
template<typename DataObjectClass>
class StrongDataObjectRef
{
private:

    OORef<DataObjectClass> _ref;

public:

    /// Default constructor.
#ifndef MSVC
    StrongDataObjectRef() noexcept = default;
#else
    StrongDataObjectRef() noexcept {}
#endif

    /// Null constructor.
    StrongDataObjectRef(std::nullptr_t) noexcept {}

    /// Initialization constructor.
    StrongDataObjectRef(const DataObjectClass* p) noexcept : _ref(p) {
        if(_ref) _ref->_referringFlowStates++;
    }

    /// Copy constructor.
    StrongDataObjectRef(const StrongDataObjectRef& rhs) noexcept : _ref(rhs._ref) {
        if(_ref) _ref->_referringFlowStates++;
    }

    /// Move constructor.
    StrongDataObjectRef(StrongDataObjectRef&& rhs) noexcept : _ref(std::move(rhs._ref)) {
        OVITO_ASSERT(!rhs._ref);
    }

    /// Move constructor from standard OORef.
    StrongDataObjectRef(OORef<DataObjectClass>&& rhs) noexcept : _ref(std::move(rhs)) {
        if(_ref) _ref->_referringFlowStates++;
    }

    /// Destructor.
    ~StrongDataObjectRef() {
        if(_ref) {
            OVITO_ASSERT(_ref->_referringFlowStates > 0);
            _ref->_referringFlowStates--;
        }
    }

    /// Copy assignment operator.
    StrongDataObjectRef& operator=(const DataObjectClass* rhs) {
    	StrongDataObjectRef(rhs).swap(*this);
    	return *this;
    }

    /// Copy assignment operator.
    StrongDataObjectRef& operator=(const StrongDataObjectRef& rhs) {
    	StrongDataObjectRef(rhs).swap(*this);
    	return *this;
    }

    /// Move assignment operator.
    StrongDataObjectRef& operator=(StrongDataObjectRef&& rhs) noexcept {
    	StrongDataObjectRef(std::move(rhs)).swap(*this);
    	return *this;
    }

    /// Move assignment operator with standard OORef.
    StrongDataObjectRef& operator=(OORef<DataObjectClass>&& rhs) noexcept {
    	StrongDataObjectRef(std::move(rhs)).swap(*this);
    	return *this;
    }

    void reset() {
    	StrongDataObjectRef().swap(*this);
    }

    void reset(DataObjectClass* rhs) {
    	StrongDataObjectRef(rhs).swap(*this);
    }

    inline DataObjectClass* get() const noexcept {
    	return _ref.get();
    }

    inline operator DataObjectClass*() const noexcept {
    	return _ref.get();
    }

    inline DataObjectClass& operator*() const noexcept {
    	return *_ref;
    }

    inline DataObjectClass* operator->() const noexcept {
    	OVITO_ASSERT(_ref);
    	return _ref.get();
    }

    inline void swap(StrongDataObjectRef& rhs) noexcept {
    	_ref.swap(rhs._ref);
    }
};

#if 0
template<class U> inline bool operator==(const StrongDataObjectRef& a, const OORef<U>& b) noexcept
{
    return a.get() == b.get();
}

template<class U> inline bool operator!=(const StrongDataObjectRef& a, const OORef<U>& b) noexcept
{
    return a.get() != b.get();
}

template<class U> inline bool operator==(const OORef<U>& a, const StrongDataObjectRef& b) noexcept
{
    return a.get() == b.get();
}

template<class U> inline bool operator!=(const OORef<U>& a, const StrongDataObjectRef& b) noexcept
{
    return a.get() != b.get();
}

template<class U> inline bool operator==(const StrongDataObjectRef& a, U* b) noexcept
{
    return a.get() == b;
}

template<class U> inline bool operator!=(const StrongDataObjectRef& a, U* b) noexcept
{
    return a.get() != b;
}

template<class T> inline bool operator==(T* a, const StrongDataObjectRef& b) noexcept
{
    return a == b.get();
}

template<class T> inline bool operator!=(T* a, const StrongDataObjectRef& b) noexcept
{
    return a != b.get();
}

inline bool operator==(const StrongDataObjectRef& p, std::nullptr_t) noexcept
{
    return p.get() == nullptr;
}

inline bool operator==(std::nullptr_t, const StrongDataObjectRef& p) noexcept
{
    return p.get() == nullptr;
}

inline bool operator!=(const StrongDataObjectRef& p, std::nullptr_t) noexcept
{
    return p.get() != nullptr;
}

inline bool operator!=(std::nullptr_t, const StrongDataObjectRef& p) noexcept
{
    return p.get() != nullptr;
}

inline bool operator<(const StrongDataObjectRef& a, const StrongDataObjectRef& b) noexcept
{
    return std::less<DataObject*>()(a.get(), b.get());
}

inline void swap(StrongDataObjectRef& lhs, StrongDataObjectRef& rhs) noexcept
{
	lhs.swap(rhs);
}

inline DataObject* get_pointer(const StrongDataObjectRef& p) noexcept
{
    return p.get();
}

inline QDebug operator<<(QDebug debug, const StrongDataObjectRef& p)
{
	return debug << p.get();
}
#endif

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
