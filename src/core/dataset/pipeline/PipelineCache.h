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
#include <core/dataset/animation/TimeInterval.h>
#include <core/dataset/pipeline/PipelineFlowState.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * \brief A local cache for PipelineFlowState objects.
 */
class OVITO_CORE_EXPORT PipelineCache
{
public:

	/// Determines whether the cache contains a cached pipeline state for the given animation time. 
	bool contains(TimePoint time) const;

	/// Returns a state from this cache that is valid at the given animation time.
	/// If the cache contains no state for the given animation time, then an empty pipeline state is returned. 
	const PipelineFlowState& getAt(TimePoint time) const;

	/// This is a special function that can be used after a call to invalidate() to access the
	/// stale cache contents.
	const PipelineFlowState& getStaleContents() const;

	/// Puts the given pipeline state into the cache for later retrieval. 
	/// The cache may decide not to cache the state, in which case the method returns false.
	bool insert(PipelineFlowState state, RefTarget* ownerObject);

	/// Puts the given pipeline state into the cache when it comes available. 
	/// Depending on the given state validity interval, the cache may decide not to cache the state, 
	/// in which case the method returns false.
	bool insert(Future<PipelineFlowState>& stateFuture, const TimeInterval& validityInterval, RefTarget* ownerObject);

	/// Marks the contents of the cache as outdated and throws away the stored data.
	///
	/// \param keepStaleContents Requests the cache to not throw away some of the data. This will mark the cached state as stale, but holds on to it.
	///                          The stored data can still be access via getStaleContents() after this method has been called.
	/// \param keepInterval An optional time interval over which the cached data should be retained. The validity interval of the cached contents 
	///                     will be reduced to this interval.
	void invalidate(bool keepStaleContents = false, TimeInterval keepInterval = TimeInterval::empty());

private:

	/// Keeps the most recently inserted state.
	PipelineFlowState _mostRecentState;

	/// Keeps the state for the current animation time.
	PipelineFlowState _currentAnimState;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
