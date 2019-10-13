////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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
#include "ThreadSafeTask.h"
#include "Future.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

class OVITO_CORE_EXPORT AsynchronousTaskBase : public ThreadSafeTask, public QRunnable
{
public:

	/// Destructor.
	virtual ~AsynchronousTaskBase();

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

	/// This virtual function is responsible for computing the results of the task.
	virtual void perform() = 0;

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
class AsynchronousTask : public TaskWithResultStorage<AsynchronousTaskBase, std::tuple<R...>>
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
		TaskWithResultStorage<AsynchronousTaskBase, std::tuple<R...>>(typename TaskWithResultStorage<AsynchronousTaskBase, std::tuple<R...>>::no_result_init_t()) {}

#ifdef OVITO_DEBUG
	bool _futureCreated = false;
#endif
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
