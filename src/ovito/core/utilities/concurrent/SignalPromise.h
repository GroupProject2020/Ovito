////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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
#include "Task.h"
#include "Promise.h"
#include "Future.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

/**
 * A promise that is used for signaling the completion of an operation, but which
 * doesn't provide access to the results of the operation nor does it report the progress.
 */
class OVITO_CORE_EXPORT SignalPromise : public Promise<>
{
public:

	/// Default constructor.
#ifndef Q_CC_MSVC
	SignalPromise() noexcept = default;
#else
	SignalPromise() noexcept {}
#endif

	/// Creates a signal promise.
	static SignalPromise create(bool startedState) {
		return std::make_shared<Task>(startedState ? Task::State(Task::Started) : Task::NoState);
	}

private:

	/// Initialization constructor.
	SignalPromise(TaskPtr p) noexcept : Promise(std::move(p)) {}
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
