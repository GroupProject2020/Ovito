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

// PromiseState uses std::function<> internally for storing the list of continuation functions.
#include <functional>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

/******************************************************************************
* Base class for shared states in the promise/future framework.
******************************************************************************/
class OVITO_CORE_EXPORT PromiseState : public std::enable_shared_from_this<PromiseState>
{
public:

    /// The different states the promise can be in.
    enum State {
        NoState    = 0,
        Started    = (1<<0),
        Finished   = (1<<1),
        Canceled   = (1<<2)
    };

    /// Constructor.
    PromiseState(State initialState = NoState) : _state(initialState) {
#ifdef OVITO_DEBUG
        _instanceCounter.fetch_add(1);
#endif
    }

    /// Destructor.
    virtual ~PromiseState();

    /// Returns whether this shared state has been canceled by a previous call to cancel().
    bool isCanceled() const { return (_state & Canceled); }

    /// Returns true if the promise is in the 'started' state.
    bool isStarted() const { return (_state & Started); }

    /// Returns true if the promise is in the 'finished' state.
    bool isFinished() const { return (_state & Finished); }

    /// Returns the maximum value for progress reporting. 
    virtual qlonglong progressMaximum() const { return 0; }

    /// Sets the current maximum value for progress reporting.
    virtual void setProgressMaximum(qlonglong maximum) {}
    
    /// Returns the current progress value (in the range 0 to progressMaximum()).
    virtual qlonglong progressValue() const { return 0; }

    /// Sets the current progress value (must be in the range 0 to progressMaximum()).
    /// Returns false if the promise has been canceled.
    virtual bool setProgressValue(qlonglong progressValue) { return !isCanceled(); }

    /// Increments the progress value by 1.
    /// Returns false if the promise has been canceled.
    virtual bool incrementProgressValue(qlonglong increment = 1) { return !isCanceled(); }

    /// Sets the progress value of the promise but generates an update event only occasionally.
    /// Returns false if the promise has been canceled.
    virtual bool setProgressValueIntermittent(qlonglong progressValue, int updateEvery = 2000) { return !isCanceled(); }

    /// Return the current status text set for this promise.
    virtual QString progressText() const { return QString(); }

    /// Changes the status text of this promise.
    virtual void setProgressText(const QString& progressText) {}
    
    // Progress reporting for tasks with sub-steps:

    /// Begins a sequence of sub-steps in the progress range of this promise.
    /// This is used for long and complex computations, which consist of several logical sub-steps, each with
    /// a separate progress range.
    virtual void beginProgressSubStepsWithWeights(std::vector<int> weights) {}

    /// Convenience version of the function above, which creates N substeps, all with the same weight.
    void beginProgressSubSteps(int nsteps) { beginProgressSubStepsWithWeights(std::vector<int>(nsteps, 1)); }

    /// Changes to the next sub-step in the sequence started with beginProgressSubSteps().
    virtual void nextProgressSubStep() {}

    /// Must be called at the end of a sub-step sequence started with beginProgressSubSteps().
    virtual void endProgressSubSteps() {}

    /// Returns the maximum progress value that can be reached (taking into account sub-steps).
    virtual qlonglong totalProgressMaximum() const { return 0; }

    /// Returns the current progress value (taking into account sub-steps).
    virtual qlonglong totalProgressValue() const { return 0; }

    /// Cancels this shared state.
    virtual void cancel() noexcept;

    // Implementation details:

    /// This must be called after creating a promise to put it into the 'started' state.
    /// Returns false if the promise is or was already in the 'started' state.
    virtual bool setStarted();

    /// This must be called after the promise has been fulfilled (even if an exception occurred).
    virtual void setFinished();

    /// Sets the promise into the 'exception' state to signal that an exception has occurred 
    /// while trying to fulfill it. This method should be called from a catch(...) exception handler.
    void captureException() { setException(std::current_exception()); }

    /// Sets the promise into the 'exception' state to signal that an exception has occurred 
    /// while trying to fulfill it. 
    virtual void setException(std::exception_ptr&& ex);

#ifdef OVITO_DEBUG
    /// Returns the global number of PromiseState instances that currently exist. Used to detect memory leaks.
    static size_t instanceCount() { return _instanceCounter.load(); }
#endif

protected:

    /// Re-throws the exception stored in this promise if an exception was previously set via setException().
    void throwPossibleException() {
        if(_exceptionStore)
            std::rethrow_exception(_exceptionStore);
    }

    /// Accessor function for the internal results storage.
    template<typename tuple_type, typename source_tuple_type, typename = std::enable_if_t<std::tuple_size<tuple_type>::value != 0>>
    void setResults(source_tuple_type&& value) {
        OVITO_ASSERT(_resultsTuple != nullptr);
#ifdef OVITO_DEBUG
        OVITO_ASSERT(!(bool)_resultSet);
        _resultSet = true;
#endif
        *static_cast<tuple_type*>(_resultsTuple) = std::forward<source_tuple_type>(value);
    }

    /// Accessor function for the internal results storage.
    template<typename tuple_type>
    const std::enable_if_t<std::tuple_size<tuple_type>::value != 0, tuple_type>& getResults() const {
        OVITO_ASSERT(_resultsTuple != nullptr);
#ifdef OVITO_DEBUG
        OVITO_ASSERT((bool)_resultSet);
#endif
        return *static_cast<const tuple_type*>(_resultsTuple);
    }

    /// Accessor function for the internal results storage.
    template<typename tuple_type>
    std::enable_if_t<std::tuple_size<tuple_type>::value == 0, tuple_type> getResults() const {
        return {};
    }

    /// Accessor function for the internal results storage.
    template<typename tuple_type>
    std::enable_if_t<std::tuple_size<tuple_type>::value != 0, tuple_type> takeResults() {
        OVITO_ASSERT(_resultsTuple != nullptr);
#ifdef OVITO_DEBUG
        OVITO_ASSERT((bool)_resultSet);
        _resultSet = false;
#endif
        return std::move(*static_cast<tuple_type*>(_resultsTuple));
    }

    /// Accessor function for the internal results storage.
    template<typename tuple_type>
    std::enable_if_t<std::tuple_size<tuple_type>::value == 0, tuple_type> takeResults() {
        return {};
    }

    template<class F>
    void addContinuation(F&& cont) {
        addContinuationImpl(std::function<void()>(std::forward<F>(cont)));
    }

    virtual void registerWatcher(PromiseWatcher* watcher);
    virtual void unregisterWatcher(PromiseWatcher* watcher);
    virtual void registerTracker(TrackingPromiseState* tracker);
    virtual void addContinuationImpl(std::function<void()>&& cont);

    void setFinishedNoSelfLock();
    
    /// Increments the count of futures that hold a strong reference to this shared state. 
    void incrementShareCount() noexcept {
        _shareCount.fetch_add(1, std::memory_order_relaxed);
    }

    /// Decrements the count of futures that hold a strong reference to this shared state. 
    /// If the count reaches zero, the shared state is automatically canceled.
    void decrementShareCount() noexcept;

    /// Linked list of PromiseWatchers that monitor this shared state.
    PromiseWatcher* _watchers = nullptr;

    /// Linked list of tracking states that track this shared state.
    std::shared_ptr<TrackingPromiseState> _trackers;

    /// Pointer to a std::tuple<R...> holding the results.
    void* _resultsTuple = nullptr; 

    /// List of continuation functions that will be called when this shared state enters the 'finished' state.
    QVarLengthArray<std::function<void()>, 1> _continuations;

    /// The current state value.
    State _state;

    /// The number of Future objects currently referring to this shared state.
    std::atomic_uint _shareCount{0};

    /// Holds the exception object when this shared state is in the failed state.
    std::exception_ptr _exceptionStore;

#ifdef OVITO_DEBUG
    /// Indicates whether the result value of shared state has been set. 
    std::atomic<bool> _resultSet{false};

    /// Global counter of PromiseState instance. Used to detect memory leaks.
    static std::atomic_size_t _instanceCounter;
#endif

    /// An empty struct tag type used by the PromiseStateWithResultStorage constructor.
    struct no_result_init_t { explicit no_result_init_t() = default; };

    friend class PromiseWatcher;
    friend class TaskManager;
    friend class FutureBase;
    friend class TrackingPromiseState;
    friend class PromiseStateCountedPtr;
    template<typename... R2> friend class Future;
    template<typename... R2> friend class SharedFuture;
    template<typename... R2> friend class Promise;
};

/******************************************************************************
* Adapter class template that packages the results storage with the PromiseState.  
******************************************************************************/
template<class BaseState, class tuple_type>
class PromiseStateWithResultStorage : public BaseState, private tuple_type
{
public:
    template<typename... Args>
    PromiseStateWithResultStorage(tuple_type initialResult, Args&&... args) : 
            BaseState(std::forward<Args>(args)...), 
            tuple_type(std::move(initialResult)) 
    {
        this->_resultsTuple = static_cast<tuple_type*>(this);
#ifdef OVITO_DEBUG
        this->_resultSet = true;
#endif
    }

    template<typename... Args>
    PromiseStateWithResultStorage(PromiseState::no_result_init_t, Args&&... args) : 
            BaseState(std::forward<Args>(args)...)
    {
        if(std::tuple_size<tuple_type>::value != 0)
            this->_resultsTuple = static_cast<tuple_type*>(this);
    }	
};

/******************************************************************************
* A smart pointer to a PromiseState that performs reference counting.
* This is used by Future/SharedFuture to hold strong references to the
* results of the PromiseState. If the reference count goes to zero,
* the PromiseState is automatically canceled.
******************************************************************************/
class OVITO_CORE_EXPORT PromiseStateCountedPtr 
{
public:

    /// Default constructor.
#ifndef Q_CC_MSVC
    PromiseStateCountedPtr() noexcept = default;
#else
    PromiseStateCountedPtr() noexcept {}
#endif

    /// Initialization constructor.
    PromiseStateCountedPtr(PromiseStatePtr ptr) noexcept : _ptr(std::move(ptr)) {
        if(_ptr) _ptr->incrementShareCount();
    }
    
    /// Copy constructor.
    PromiseStateCountedPtr(const PromiseStateCountedPtr& other) noexcept : _ptr(other._ptr) {
        if(_ptr) _ptr->incrementShareCount();
    }

    /// Move constructor.
    PromiseStateCountedPtr(PromiseStateCountedPtr&& rhs) noexcept : _ptr(std::move(rhs._ptr)) {}

    /// Destructor.
    ~PromiseStateCountedPtr() noexcept {
        if(_ptr) _ptr->decrementShareCount();
    }

    // Copy assignment.
    PromiseStateCountedPtr& operator=(const PromiseStateCountedPtr& rhs) noexcept {
        PromiseStateCountedPtr(rhs).swap(*this);
        return *this;
    }

    // Move assignment.
    PromiseStateCountedPtr& operator=(PromiseStateCountedPtr&& rhs) noexcept {
        PromiseStateCountedPtr(std::move(rhs)).swap(*this);
        return *this;
    }	

    // Access to pointer value.
    const PromiseStatePtr& get() const noexcept {
        return _ptr;
    }	

    void reset() noexcept {
        PromiseStateCountedPtr().swap(*this);
    }

    void reset(PromiseStatePtr rhs) noexcept {
        PromiseStateCountedPtr(std::move(rhs)).swap(*this);
    }

    inline void swap(PromiseStateCountedPtr& rhs) noexcept {
        _ptr.swap(rhs._ptr);
    }

private:
    PromiseStatePtr _ptr;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


