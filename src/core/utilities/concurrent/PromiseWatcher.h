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

/******************************************************************************
* A utility class that integrates the Promise class into the Qt signal/slots
* framework. It allows to have signals generated when the shared state of a 
* Future or a Promise changes.
******************************************************************************/
class OVITO_CORE_EXPORT PromiseWatcher : public QObject
{
public:

	/// Constructor that creates a watcher that is not associated with 
	/// any future/promise yet.
	PromiseWatcher(QObject* parent = nullptr) : QObject(parent) {}

	/// Destructor.
	virtual ~PromiseWatcher() {
		watch(nullptr, false);
	}

	/// Returns whether this watcher is currently monitoring a shared state.
	bool isWatching() const { return (bool)_sharedState; }

	/// Returns the shared state being monitored by this watcher.
	const PromiseStatePtr& sharedState() const { return _sharedState; }

	/// Makes this watcher monitor the given shared state.
	void watch(const PromiseStatePtr& promiseState, bool pendingAssignment = true);

	/// Detaches this watcher from the shared state.
	void reset() { watch(nullptr); }

	/// Returns true if the shared state monitored by this object has been canceled.
	bool isCanceled() const;

	/// Returns true if the shared state monitored by this object has reached the 'finished' state.
	bool isFinished() const;

	/// Returns the maximum value for the progress of the shared state monitored by this object.
	int progressMaximum() const;

	/// Returns the current value for the progress of the shared state monitored by this object.
	int progressValue() const;

	/// Returns the status text of the shared state monitored by this object.
	QString progressText() const;

Q_SIGNALS:

	void canceled();
	void finished();
	void started();
	void progressRangeChanged(int maximum);
	void progressValueChanged(int progressValue);
	void progressTextChanged(const QString& progressText);

private Q_SLOTS:

	void promiseCanceled();
	void promiseFinished();
	void promiseStarted();
	void promiseProgressRangeChanged(int maximum);
	void promiseProgressValueChanged(int progressValue);
	void promiseProgressTextChanged(QString progressText);

private:

	/// The shared state being monitored.
	PromiseStatePtr _sharedState;

	/// Indicates that the shared state has reached the 'finished' state.
    bool _finished = false;

	/// Linked list pointer for list of registered watchers of the current shared state.
	PromiseWatcher* _nextInList;	

	Q_OBJECT

	friend class PromiseState;
	friend class PromiseStateWithProgress;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


