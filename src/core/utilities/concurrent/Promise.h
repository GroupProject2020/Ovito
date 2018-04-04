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
#include "PromiseState.h"
#include "SynchronousPromiseState.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

class OVITO_CORE_EXPORT PromiseBase
{
public:

	/// Destructor.
	~PromiseBase() { reset(); }

	/// A promise is not copy constructible.
	PromiseBase(const PromiseBase& other) = delete;

	/// Returns whether this promise object points to a valid shared state.
	bool isValid() const { return (bool)_sharedState; }

	/// Detaches this promise from its shared state and makes sure that it reached the 'finished' state.
	/// If the promise wasn't already finished when thsi function is called, it is automatically canceled.
	void reset() {
		if(isValid()) {
			if(!isFinished()) {
				sharedState()->cancel();
				sharedState()->setStarted();
				sharedState()->setFinished();
			}
			_sharedState.reset(); 
		}
	}

	/// Returns whether this promise has been canceled by a previous call to cancel().
	bool isCanceled() const { return sharedState()->isCanceled(); }

	/// Returns true if the promise is in the 'started' state.
	bool isStarted() const { return sharedState()->isStarted(); }

	/// Returns true if the promise is in the 'finished' state.
	bool isFinished() const { return sharedState()->isFinished(); }

	/// Returns the maximum value for progress reporting. 
    int progressMaximum() const { return sharedState()->progressMaximum(); }

	/// Sets the current maximum value for progress reporting.
    void setProgressMaximum(int maximum) const { sharedState()->setProgressMaximum(maximum); }
    
	/// Returns the current progress value (in the range 0 to progressMaximum()).
	int progressValue() const { return sharedState()->progressValue(); }

	/// Sets the current progress value (must be in the range 0 to progressMaximum()).
	/// Returns false if the promise has been canceled.
    bool setProgressValue(int progressValue) const { return sharedState()->setProgressValue(progressValue); }

	/// Increments the progress value by 1.
	/// Returns false if the promise has been canceled.
    bool incrementProgressValue(int increment = 1) const { return sharedState()->incrementProgressValue(increment); }

	/// Sets the progress value of the promise but generates an update event only occasionally.
	/// Returns false if the promise has been canceled.
    bool setProgressValueIntermittent(int progressValue, int updateEvery = 2000) const { return sharedState()->setProgressValueIntermittent(progressValue, updateEvery); }

	// Progress reporting for tasks with sub-steps:

	/// Begins a sequence of sub-steps in the progress range of this promise.
	/// This is used for long and complex computations, which consist of several logical sub-steps, each with
	/// a separate progress range.
    void beginProgressSubStepsWithWeights(std::vector<int> weights) const { sharedState()->beginProgressSubStepsWithWeights(std::move(weights)); }
	
	/// Convenience version of the function above, which creates N substeps, all with the same weight.
	void beginProgressSubSteps(int nsteps) const { sharedState()->beginProgressSubSteps(nsteps); }

	/// Changes to the next sub-step in the sequence started with beginProgressSubSteps().
	void nextProgressSubStep() const { sharedState()->nextProgressSubStep(); }

	/// Must be called at the end of a sub-step sequence started with beginProgressSubSteps().
	void endProgressSubSteps() const { sharedState()->endProgressSubSteps(); }
		
	/// Return the current status text set for this promise.
    QString progressText() const { return sharedState()->progressText(); }

	/// Changes the status text of this promise.
	void setProgressText(const QString& progressText) const { sharedState()->setProgressText(progressText); }

	/// Cancels this promise.
	void cancel() const { sharedState()->cancel(); }

	/// This must be called after creating a promise to put it into the 'started' state.
	/// Returns false if the promise is or was already in the 'started' state.
	bool setStarted() const { return sharedState()->setStarted(); }

	/// This must be called after the promise has been fulfilled (even if an exception occurred).
	void setFinished() const { sharedState()->setFinished(); }

	/// Sets the promise into the 'exception' state to signal that an exception has occurred 
	/// while trying to fulfill it. This method should be called from a catch(...) exception handler.
    void captureException() const { sharedState()->captureException(); }

	/// Sets the promise into the 'exception' state to signal that an exception has occurred 
	/// while trying to fulfill it. 
    void setException(std::exception_ptr&& ex) const { sharedState()->setException(std::move(ex)); }

	/// Move assignment operator.
	PromiseBase& operator=(PromiseBase&& p) = default;

	/// A promise is not copy assignable.
	PromiseBase& operator=(const PromiseBase& other) = delete;

	/// Returns the shared state of this promise.
	const PromiseStatePtr& sharedState() const {
		OVITO_ASSERT(isValid());
		return _sharedState;
	}

	/// Calls TaskManager::registerTask().
	void registerWithTaskManager(TaskManager& taskManager);

protected:

	/// Default constructor.
#ifndef Q_CC_MSVC
	PromiseBase() noexcept = default;
#else
	PromiseBase() noexcept {}
#endif

	/// Move constructor.
	PromiseBase(PromiseBase&& p) noexcept = default;

	/// Constructor.
	PromiseBase(PromiseStatePtr&& p) noexcept : _sharedState(std::move(p)) {}

	/// Pointer to the state, which is shared with futures.
	PromiseStatePtr _sharedState;

	template<typename... R2> friend class Future;
	template<typename... R2> friend class SharedFuture;
};

template<typename... R>
class Promise : public PromiseBase
{
public:

	using tuple_type = std::tuple<R...>;
	using future_type = Future<R...>;

	/// Default constructor.
#ifndef Q_CC_MSVC
	Promise() noexcept = default;
#else
	Promise() noexcept {}
#endif

	/// Creates a promise that is used to report progress in the main thread.
	static Promise createSynchronous(TaskManager* taskManager, bool startedState, bool registerWithManager) {
		Promise promise(std::make_shared<PromiseStateWithResultStorage<SynchronousPromiseState, tuple_type>>(
			PromiseState::no_result_init_t(), 
			startedState ? PromiseState::State(PromiseState::Started) : PromiseState::NoState, taskManager));
		if(registerWithManager && taskManager) promise.registerWithTaskManager(*taskManager);
		return promise;
	}

	/// Create a promise that is ready and provides an immediate result.
	template<typename... R2>
	static Promise createImmediate(R2&&... result) {
		return Promise(std::make_shared<PromiseStateWithResultStorage<PromiseState, tuple_type>>(
			std::forward_as_tuple(std::forward<R2>(result)...), 
			PromiseState::State(PromiseState::Started | PromiseState::Finished)));
	}

	/// Create a promise that is ready and provides an immediate result.
	template<typename... Args>
	static Promise createImmediateEmplace(Args&&... args) {
		using first_type = typename std::tuple_element<0, tuple_type>::type;
		return Promise(std::make_shared<PromiseStateWithResultStorage<PromiseState, tuple_type>>(
			std::forward_as_tuple(first_type(std::forward<Args>(args)...)), 
			PromiseState::State(PromiseState::Started | PromiseState::Finished)));
	}

	/// Creates a promise that is in the 'exception' state.
	static Promise createFailed(const Exception& ex) {
		Promise promise(std::make_shared<PromiseState>(PromiseState::State(PromiseState::Started | PromiseState::Finished)));
		promise.sharedState()->_exceptionStore = std::make_exception_ptr(ex);
		return promise;
	}

	/// Creates a promise that is in the 'exception' state.
	static Promise createFailed(std::exception_ptr ex_ptr) {
		Promise promise(std::make_shared<PromiseState>(PromiseState::State(PromiseState::Started | PromiseState::Finished)));
		promise.sharedState()->_exceptionStore = std::move(ex_ptr);
		return promise;
	}

	/// Creates a promise without results that is in the canceled state.
	static Promise createCanceled() {
		return Promise(std::make_shared<PromiseState>(PromiseState::State(PromiseState::Started | PromiseState::Canceled | PromiseState::Finished)));
	}

	/// Returns a future that is associated with the same shared state as this promise.
	future_type future() {
#ifdef OVITO_DEBUG
		OVITO_ASSERT_MSG(!_futureCreated, "Promise::future()", "Only a single Future may be created from a Promise.");
		_futureCreated = true;
#endif
		return future_type(PromiseStatePtr(sharedState()));
	}

	/// Sets the result value of the promise.
	template<typename... R2>
	void setResults(R2&&... result) {
		sharedState()->template setResults<tuple_type>(std::forward_as_tuple(std::forward<R2>(result)...));
	}

private:

	Promise(PromiseStatePtr p) noexcept : PromiseBase(std::move(p)) {}

#ifdef OVITO_DEBUG
	bool _futureCreated = false;
#endif
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
