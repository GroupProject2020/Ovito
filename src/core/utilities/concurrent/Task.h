///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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
#include "ThreadSafePromiseState.h"
#include "Future.h"

#include <QRunnable>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

class AsynchronousTaskBase : public ThreadSafePromiseState, public QRunnable
{
public:

	/// Destructor.
	virtual ~AsynchronousTaskBase();

	/// This function must be implemented by subclasses to perform the actual task.
	virtual void perform() = 0;

	/// Runs the given function once this task has reached the 'finished' state.
	/// The function is run even if the task was canceled or produced an error state.
	template<typename FC, class Executor>
	void finally(Executor&& executor, FC&& cont)
	{
		addContinuation(
			executor.createWork([cont = std::forward<FC>(cont)](bool workCanceled) mutable {
				if(!workCanceled)
					std::move(cont)();
		}));
	}

protected:

	/// Constructor.
	AsynchronousTaskBase() {
		QRunnable::setAutoDelete(false);
	}

private:

	/// Implementation of QRunnable.
	virtual void run() override;
};

template<typename... R>
class AsynchronousTask : public PromiseStateWithResultStorage<AsynchronousTaskBase, std::tuple<R...>>
{
public:

	/// Returns a future that is associated with the same shared state as this task.
	Future<R...> future() {
#ifdef OVITO_DEBUG
		OVITO_ASSERT_MSG(!_futureCreated, "AsynchronousTask::future()", "Only a single Future may be created from a task.");
		_futureCreated = true;
#endif
		return Future<R...>(this->shared_from_this());
	}

	/// Sets the result value of the task.
	template<typename... R2>
	void setResult(R2&&... result) {
		this->template setResults<std::tuple<R...>>(std::forward_as_tuple(std::forward<R2>(result)...));
	}

protected:

	/// Constructor.
	AsynchronousTask() : 
		PromiseStateWithResultStorage<AsynchronousTaskBase, std::tuple<R...>>(PromiseState::no_result_init_t()) {}

#ifdef OVITO_DEBUG
	bool _futureCreated = false;
#endif
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


