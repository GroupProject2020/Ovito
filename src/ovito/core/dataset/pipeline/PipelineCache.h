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


#include <ovito/core/Core.h>
#include <ovito/core/dataset/animation/TimeInterval.h>
#include <ovito/core/dataset/pipeline/PipelineFlowState.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * \brief A local cache for PipelineFlowState objects.
 */
class OVITO_CORE_EXPORT PipelineCache
{
public:

	/// Determines whether the cache contains a cached pipeline state for the given animation time.
	bool contains(TimePoint time, bool onlyCurrentAnimTime = false) const;

	/// Returns a state from this cache that is valid at the given animation time.
	/// If the cache contains no state for the given animation time, then an empty pipeline state is returned.
	const PipelineFlowState& getAt(TimePoint time) const;

	/// This is a special function that can be used after a call to invalidate() to access the
	/// stale cache contents.
	const PipelineFlowState& getStaleContents() const { return _currentAnimState; }

	/// Puts the given pipeline state into the cache for later retrieval.
	/// The cache may decide not to cache the state, in which case the method returns false.
	bool insert(PipelineFlowState state, const RefTarget* ownerObject);

	/// Puts the given pipeline state into the cache when it comes available.
	/// Depending on the given state validity interval, the cache may decide not to cache the state.
	void insert(Future<PipelineFlowState>& stateFuture, const TimeInterval& validityInterval, const RefTarget* ownerObject);

	/// Marks the contents of the cache as outdated and throws away the stored data.
	///
	/// \param keepStaleContents Requests the cache to not throw away some of the data. This will mark the cached state as stale, but holds on to it.
	///                          The stored data can still be access via getStaleContents() after this method has been called.
	/// \param keepInterval An optional time interval over which the cached data should be retained. The validity interval of the cached contents
	///                     will be reduced to this interval.
	void invalidate(bool keepStaleContents = false, TimeInterval keepInterval = TimeInterval::empty());

	/// Requests that the validity interval of the next pipeline state being stored in the cache should be restriced to the given interval (which may be empty).
	/// This method can be called by the cache owner upon receiving a TargetChanged event while a pipeline evaluation is in progress.
	void restrictValidityOfNextInsertedState(TimeInterval iv = TimeInterval::empty()) { _restrictedValidityOfNextInsertedState.intersect(iv); }

	/// Resets the validity interval restriction set by restrictValidityOfNextInsertedState().
	/// This method should be called prior to a new pipeline evaluation in order to accept the generated pipeline state into the cache.
	void resetValidityRestriction() { _restrictedValidityOfNextInsertedState.setInfinite(); }

private:

	/// Keeps the most recently inserted state.
	PipelineFlowState _mostRecentState;

	/// Keeps the state for the current animation time.
	PipelineFlowState _currentAnimState;

	/// This interval is set to a finite or empty range if a TargetChanged signal is received by the cache owner while a pipeline evaluation is in progress.
	/// It restricts the validity of (or completely invalidates) the next pipeline state that will be inserted into the cache.
	TimeInterval _restrictedValidityOfNextInsertedState = TimeInterval::infinite();
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
