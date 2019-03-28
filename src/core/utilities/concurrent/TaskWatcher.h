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
 * \brief Provides a Qt signal/slots interface to an asynchronous task.
 */
class OVITO_CORE_EXPORT TaskWatcher : public QObject
{
public:

	/// Constructor that creates a watcher that is not associated with
	/// any future/promise yet.
	TaskWatcher(QObject* parent = nullptr) : QObject(parent) {}

	/// Destructor.
	virtual ~TaskWatcher() {
		watch(nullptr, false);
	}

	/// Returns whether this watcher is currently monitoring a shared state.
	bool isWatching() const { return (bool)_task; }

	/// Returns the shared state being monitored by this watcher.
	const TaskPtr& task() const { return _task; }

	/// Makes this watcher monitor the given shared state.
	void watch(const TaskPtr& promiseState, bool pendingAssignment = true);

	/// Detaches this watcher from the shared state.
	void reset() { watch(nullptr); }

	/// Returns true if the shared state monitored by this object has been canceled.
	bool isCanceled() const;

	/// Returns true if the shared state monitored by this object has reached the 'finished' state.
	bool isFinished() const;

	/// Returns the maximum value for the progress of the shared state monitored by this object.
	qlonglong progressMaximum() const;

	/// Returns the current value for the progress of the shared state monitored by this object.
	qlonglong progressValue() const;

	/// Returns the status text of the shared state monitored by this object.
	QString progressText() const;

public Q_SLOTS:

	/// Cancels the operation being watched by this watcher.
	void cancel();

Q_SIGNALS:

	void canceled();
	void finished();
	void started();
	void progressRangeChanged(qlonglong maximum);
	void progressValueChanged(qlonglong progressValue);
	void progressTextChanged(const QString& progressText);

private Q_SLOTS:

	void promiseCanceled();
	void promiseFinished();
	void promiseStarted();
	void promiseProgressRangeChanged(qlonglong maximum);
	void promiseProgressValueChanged(qlonglong progressValue);
	void promiseProgressTextChanged(const QString& progressText);

private:

	/// The shared state being monitored.
	TaskPtr _task;

	/// Indicates that the shared state has reached the 'finished' state.
    bool _finished = false;

	/// Linked list pointer for list of registered watchers of the current shared state.
	TaskWatcher* _nextInList;

	Q_OBJECT

	friend class Task;
	friend class ProgressiveTask;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
