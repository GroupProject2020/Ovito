///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * This helper class can be used for implementing a simple data cache.
 * It helps in detecting changes in input parameters or data that would require a recalculating of the outputs.
 */
template<class... Types>
class CacheStateHelper
{
public:

	/// Compares the stored state to the new input state before replacing it with the
	/// new state. Returns true if the new input state differs from the old one. This indicates
	/// that the cached data is invalid and needs to be regenerated.
	bool updateState(const Types&... args) {
		bool hasChanged = (_oldState != std::tuple<Types...>(args...));
		_oldState = std::tuple<Types...>(args...);
		return hasChanged;
	}

	/// Compares the stored state with the given state.
	bool hasChanged(const Types&... args) const {
		return (_oldState != std::tuple<Types...>(args...));
	}

private:

	// The previous input state.
	std::tuple<Types...> _oldState;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
