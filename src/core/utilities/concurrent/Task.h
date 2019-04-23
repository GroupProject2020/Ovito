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

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

/**
 * \brief The shared state of a Promise/Future pair.
 */
class OVITO_CORE_EXPORT Task : public std::enable_shared_from_this<Task>
{
public:

    /// The different states a task can be in.
    enum State {
        NoState    = 0,
        Started    = (1<<0),
        Finished   = (1<<1),
        Canceled   = (1<<2)
    };

    /// Constructor.
    Task(State initialState = NoState) : _state(initialState) {
#ifdef OVITO_DEBUG
        _instanceCounter.fetch_add(1);
#endif
    }

    /// Destructor.
    virtual ~Task();

    /// Returns whether this shared state has been canceled by a previous call to cancel().
    bool isCanceled() const { return (_state & Canceled); }

    /// Returns true if the promise is in the 'started' state.
    bool isStarted() const { return (_state & Started); }

    /// Returns true if the promise is in the 'finished' state.
    bool isFinished() const { return (_state & Finished); }

    /// Returns the maximum value for progress reporting.
    virtual qlonglong progressMaximum() const { return 0; }

    /// \brief Sets the current maximum value for progress reporting.
    virtual void setProgressMaximum(qlonglong maximum) {}

    /// \brief Returns the current progress of the task.
    /// \return A value in the range 0 to progressMaximum().
    virtual qlonglong progressValue() const { return 0; }

    /// \brief Sets the current progress value of the task.
    /// \param progressValue The new value, which must be in the range 0 to progressMaximum().
    /// \return false if the task has been canceled in the meantime.
    virtual bool setProgressValue(qlonglong progressValue) { return !isCanceled(); }

    /// \brief Increments the progress value of the task.
    /// \param increment The number of progress units to add to the current progress value.
    /// \return false if the task has been canceled in the meantime.
    virtual bool incrementProgressValue(qlonglong increment = 1) { return !isCanceled(); }

    /// \brief Sets the current progress value of the task, generating update events only occasionally.
    /// \param progressValue The new value, which must be in the range 0 to progressMaximum().
    /// \param updateEvery Generate an update event only after the method has been called this many times.
    /// \return false if the task has been canceled in the meantime.
    virtual bool setProgressValueIntermittent(qlonglong progressValue, int updateEvery = 2000) { return !isCanceled(); }

    /// \brief Returns the current status text of this task.
    /// \return A string describing the ongoing operation, which is displayed in the user interface.
    virtual QString progressText() const { return QString(); }

    /// \brief Changes the status text of this task.
    /// \param progressText The text string that will be displayed in the user interface to describe the operation in
    ///                     progress.
    virtual void setProgressText(const QString& progressText) {}

    // Progress reporting for tasks with sub-steps:

    /// \brief Starts a sequence of sub-steps in the progress range of this task.
    ///
    /// This is used for long and complex operation, which consist of several logical sub-steps, each with a separate
    /// duration.
    ///
    /// \param weights A vector of relative weights, one for each sub-step, which will be used to calculate the
    ///                the total progress as sub-steps are completed.
    virtual void beginProgressSubStepsWithWeights(std::vector<int> weights) {}

    /// \brief Convenience version of the function above, which creates *N* substeps, all with the same weight.
    /// \param nsteps The number of sub-steps in the sequence.
    void beginProgressSubSteps(int nsteps) { beginProgressSubStepsWithWeights(std::vector<int>(nsteps, 1)); }

    /// \brief Completes the current sub-step in the sequence started with beginProgressSubSteps() or
    ///        beginProgressSubStepsWithWeights() and moves to the next one.
    virtual void nextProgressSubStep() {}

    /// \brief Completes a sub-step sequence started with beginProgressSubSteps() or beginProgressSubStepsWithWeights().
    ///
    /// Call this method after the last sub-step has been completed.
    virtual void endProgressSubSteps() {}

    /// \brief Returns the maximum duration of this task (also taking into account any sub-steps).
    /// \return The number of work units this task consists of. The special value 0 indicates that the duration of the task is unknown.
    virtual qlonglong totalProgressMaximum() const { return 0; }

    /// \brief Returns the current progress value of this task (also taking into account any sub-steps).
    /// \return The number of work units of this task that have been completed so far. This is a value between 0 and totalProgressMaximum().
    virtual qlonglong totalProgressValue() const { return 0; }

    /// \brief Requests cancellation of the task.
    virtual void cancel() noexcept;

    /// \brief Switches the task into the 'started' state.
    /// \return false if the task has already been in the 'started' state.
    ///
    /// This should be called after creating a task to put it into the 'started' state.
    virtual bool setStarted();

    /// \brief Switches the task into the 'finished' state.
    ///
    /// This should be called after the task has been completed or when it has failed.
    virtual void setFinished();

    /// \brief Switches the task into the 'exception' state to signal that an exception has occurred.
    ///
    /// This method should be called from within an exception handler. It saves a copy of the current exception
    /// being handled into the task object.
    void captureException() { setException(std::current_exception()); }

    /// \brief Switches the task into the 'exception' state to signal that an exception has occurred.
    /// \param ex The exception to store into the task object.
    virtual void setException(std::exception_ptr&& ex);

    /// \brief Creates a child task that executes within the context of this parent task.
    /// \return A promise which should be used to control the child task.
    ///
    /// In case the child task gets canceled, this parent task gets canceled too -and vice versa.
	virtual Promise<> createSubTask();

    /// \brief Blocks execution until the given future enters the completed state.
    /// \param future The future to wait for.
    /// \return false if the either this task or the future have been canceled.
    ///
    /// If the future gets canceled for some reason while waiting for it, this task gets automatically canceled as well.
	virtual bool waitForFuture(const FutureBase& future);

#ifdef OVITO_DEBUG
    /// Returns the global number of Task instances that currently exist. Used to detect memory leaks.
    static size_t instanceCount() { return _instanceCounter.load(); }
#endif

protected:

    /// \brief Re-throws the exception stored in this task state if an exception was previously set via setException().
    /// \throw The exception stored in the Task (if any).
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

    virtual void registerWatcher(TaskWatcher* watcher);
    virtual void unregisterWatcher(TaskWatcher* watcher);
    virtual void registerTracker(TrackingTask* tracker);
    virtual void addContinuationImpl(std::function<void()>&& cont);

    void setFinishedNoSelfLock();

    /// Increments the count of futures that hold a strong reference to this shared state.
    void incrementShareCount() noexcept {
        _shareCount.fetch_add(1, std::memory_order_relaxed);
    }

    /// Decrements the count of futures that hold a strong reference to this shared state.
    /// If the count reaches zero, the shared state is automatically canceled.
    void decrementShareCount() noexcept;

    /// Cancels this task if there is only a single future that depends on it.
    /// This is an internal method used by TaskManager::waitForTask().
    void cancelIfSingleFutureLeft() noexcept;

    /// Linked list of PromiseWatchers that monitor this shared state.
    TaskWatcher* _watchers = nullptr;

    /// Linked list of tracking states that track this shared state.
    std::shared_ptr<TrackingTask> _trackers;

    /// Pointer to a std::tuple<R...> holding the results.
    void* _resultsTuple = nullptr;

    /// List of continuation functions that will be called when this shared state enters the 'finished' state.
    QVarLengthArray<std::function<void()>, 1> _continuations;

    /// The current state value.
    State _state;

    /// The number of Future objects currently referring to this shared state.
    std::atomic_int _shareCount{0};

    /// Holds the exception object when this shared state is in the failed state.
    std::exception_ptr _exceptionStore;

#ifdef OVITO_DEBUG
    /// Indicates whether the result value of shared state has been set.
    std::atomic<bool> _resultSet{false};

    /// Global counter of Task instance. Used to detect memory leaks.
    static std::atomic_size_t _instanceCounter;
#endif

    friend class TaskWatcher;
    friend class TaskManager;
    friend class FutureBase;
    friend class TrackingTask;
    friend class TaskDependency;
    template<typename... R2> friend class Future;
    template<typename... R2> friend class SharedFuture;
    template<typename... R2> friend class Promise;
};

/**
 * \brief Composite class template that packages a Task together with the storage for the task's results tuple.
 *
 * \tparam TaskType A Task derived class.
 * \tparam Tuple The std::tuple<...> type of the results storage.
 *
 * The Task gets automatically configured to use the internal results storage provided by this class.
 */
template <class TaskType, class Tuple>
class TaskWithResultStorage : public TaskType, private Tuple
{
public:
    /// A special tag parameter type used to differentiate the second TaskWithResultStorage constructor.
    struct no_result_init_t {
        explicit no_result_init_t() = default;
    };

public:
    /// \brief Constructor which sets the value of the results storage and initializes the Task with the extra arguments.
    /// \param initialResult The value to assign to the results storage tuple.
    /// \param args The extra arguments which will be passed to the constructor of the Task derived class.
    template <typename... Args>
    TaskWithResultStorage(Tuple initialResult, Args&&... args)
        : TaskType(std::forward<Args>(args)...), Tuple(std::move(initialResult))
    {
        // Inform the Task about the internal storage location for the task results.
        this->_resultsTuple = static_cast<Tuple*>(this);
#ifdef OVITO_DEBUG
        // This is used in debug builds to detect programming errors and explicitly keep track of whether a result has
        // been assigned to the task.
        this->_resultSet = true;
#endif
    }

    /// \brief Constructor which doesn't initialize the results storage (only the Task).
    /// \param ignored An unused tag parameter for discriminating this constructor from the other one.
    /// \param args The arguments to be passed to the constructor of the Task derived class.
    template <typename... Args>
    TaskWithResultStorage(no_result_init_t ignored, Args&&... args) : TaskType(std::forward<Args>(args)...)
    {
        // Inform the Task about the internal storage location for the task results,
        // unless this is a task not having any return value (i.e. empty tuple).
        if(std::tuple_size<Tuple>::value != 0) this->_resultsTuple = static_cast<Tuple*>(this);
    }
};

/**
 * \brief A smart pointer to a Task, implementing intrusive reference counting.
 *
 * This is used by the classes Future and SharedFuture to express their dependency on a Task. If the reference
 * count to the Task reaches zero, because no future depends on it anymore, the Task is automatically canceled.
 */
class OVITO_CORE_EXPORT TaskDependency
{
public:

#ifndef Q_CC_MSVC
    /// Default constructor, initializing the smart pointer to null.
    TaskDependency() noexcept = default;
#else  // Workaround for MSVC compiler deficiency:
    /// Default constructor, initializing the smart pointer to null.
    TaskDependency() noexcept {}
#endif

    /// Initialization constructor.
    TaskDependency(TaskPtr ptr) noexcept : _ptr(std::move(ptr)) {
        if(_ptr) _ptr->incrementShareCount();
    }

    /// Copy constructor.
    TaskDependency(const TaskDependency& other) noexcept : _ptr(other._ptr) {
        if(_ptr) _ptr->incrementShareCount();
    }

    /// Move constructor.
    TaskDependency(TaskDependency&& rhs) noexcept : _ptr(std::move(rhs._ptr)) {}

    /// Destructor.
    ~TaskDependency() noexcept {
        if(_ptr) _ptr->decrementShareCount();
    }

    // Copy assignment.
    TaskDependency& operator=(const TaskDependency& rhs) noexcept {
        TaskDependency(rhs).swap(*this);
        return *this;
    }

    // Move assignment.
    TaskDependency& operator=(TaskDependency&& rhs) noexcept {
        TaskDependency(std::move(rhs)).swap(*this);
        return *this;
    }

    // Access to pointer value.
    const TaskPtr& get() const noexcept {
        return _ptr;
    }

    void reset() noexcept {
        TaskDependency().swap(*this);
    }

    void reset(TaskPtr rhs) noexcept {
        TaskDependency(std::move(rhs)).swap(*this);
    }

    inline void swap(TaskDependency& rhs) noexcept {
        _ptr.swap(rhs._ptr);
    }

private:
    TaskPtr _ptr;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
