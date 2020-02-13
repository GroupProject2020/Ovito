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
#include <ovito/core/utilities/concurrent/Future.h>
#include <ovito/core/utilities/concurrent/Promise.h>
#include <ovito/core/utilities/concurrent/MainThreadTask.h>

namespace Ovito {

/**
 * \brief An executor that can be used with Future<>::then(), which runs the closure
 *        routine in the context (and in the thread) of this RefTarget.
 */
class OVITO_CORE_EXPORT RefTargetExecutor
{
private:

	/// Helper class that is used by this executor to transmit a callable object
	/// to the UI thread where it is executed in the context on an OvitoObject.
	class OVITO_CORE_EXPORT WorkEventBase : public QEvent
	{
	protected:

		/// Constructor.
		explicit WorkEventBase(const RefTarget* obj);

		/// Determines whether work can be executed in the context of the OvitoObject or not.
		bool needToCancelWork() const;

		/// Activates the original execution context under which the work was submitted.
		void activateExecutionContext();

		/// Restores the execution context as it was before the work was executed.
		void restoreExecutionContext();

		/// Weak pointer to the RefTarget which provides the context for the work
		/// to perform.
		QPointer<RefTarget> _obj;

		/// The execution context (interactive or scripting) under which the work has been submitted.
		int _executionContext;
	};

	/// Helper class that is used by this executor to transmit a callable object
	/// to the UI thread where it is executed in the context on a RefTarget.
	template<typename F>
	class WorkEvent : public WorkEventBase
	{
	public:

		/// Constructor.
		WorkEvent(const RefTarget* obj, F&& callable) :
			WorkEventBase(obj), _callable(std::move(callable)) {}

		/// Destructor.
		virtual ~WorkEvent() {
			// Qt events should only be destroyed in the main thread.
			OVITO_ASSERT(QCoreApplication::closingDown() || QThread::currentThread() == QCoreApplication::instance()->thread());
			if(!needToCancelWork()) {
				/// Activate the original execution context under which the work was submitted.
				activateExecutionContext();
				// Execute the work function.
				std::move(_callable)();
				/// Restore the execution context as it was before the work was executed.
				restoreExecutionContext();
			}
		}

	private:
		F _callable;
	};

public:

	/**
	 * Represents a work that will be scheduled for execution later
	 * by invoking the class' call operator.
	 */
	class OVITO_CORE_EXPORT Work
	{
	public:
		explicit Work(std::unique_ptr<WorkEventBase> event) : _event(std::move(event)) {}
		Work(Work&& other) = default;
#ifdef OVITO_DEBUG
		~Work() { OVITO_ASSERT_MSG(!_event, "RefTargetExecutor::Work", "Work has not been executed by invoking the call operator or the post() method."); }
#endif
		Work& operator=(Work&&) = default;

		/// Schedules the work function stored in this object for execution; or executes the work immediately if possible. 
		/// If defer==true, the work will be executed at a later time, even if an immediate execution
		/// would be possible. 
		void operator()(bool defer);

	private:

		std::unique_ptr<WorkEventBase> _event;
	};

public:

	/// \brief Constructor.
	RefTargetExecutor(const RefTarget* obj) noexcept : _obj(obj) { OVITO_ASSERT(obj); }

	/// \brief Create some work that can be submitted for execution later.
	template<typename F>
	Work createWork(F&& f) {
		OVITO_ASSERT(_obj != nullptr);
		return Work(std::make_unique<WorkEvent<F>>(_obj, std::forward<F>(f)));
	}

	/// \brief Returns the task manager that provides the context for tasks created by this executor.
	TaskManager* taskManager() const;

	/// Returns the RefTarget this executor is associated with.
	/// Work submitted to this executor will be executed in the context of the RefTarget.
	const RefTarget* object() const { return _obj; }

	/// Returns the unique Qt event type ID used by this class to schedule asynchronous work.
	static QEvent::Type workEventType() {
		static const int _workEventType = QEvent::registerEventType();
		return static_cast<QEvent::Type>(_workEventType);
	}

private:

	const RefTarget* _obj = nullptr;

	friend class Application;
};

}	// End of namespace
