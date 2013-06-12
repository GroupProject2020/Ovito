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

#ifndef __OVITO_FUTURE_H
#define __OVITO_FUTURE_H

#include <core/Core.h>
#include "FutureInterface.h"

namespace Ovito {

template<typename R>
class Future {
public:
	typedef FutureInterface<R> Interface;

	Future() {}
	Future(const R& result) : _interface(std::make_shared<Interface>()) { interface()->setResult(result); }
	Future(R&& result) : _interface(std::make_shared<Interface>()) { interface()->setResult(std::move(result)); }
	bool isCanceled() const { return interface()->isCanceled(); }
	void cancel() { interface()->cancel(); }
	const R& result() const {
		interface()->waitForResult();
		return interface()->_result;
	}
	void waitForFinished() const {
		interface()->waitForFinished();
	}
	void abort() {
		cancel();
		waitForFinished();
	}
	bool isValid() const { return (bool)_interface; }
private:
	Future(const std::shared_ptr<Interface>& p) : _interface(p) {}

	std::shared_ptr<Interface>& interface() {
		OVITO_ASSERT(isValid());
		return _interface;
	}
	const std::shared_ptr<Interface>& interface() const {
		OVITO_ASSERT(isValid());
		return _interface;
	}
	std::shared_ptr<Interface> _interface;

	template<typename R2, typename Function> friend class Task;
	template<typename R2> friend class FutureInterface;
};

};

#endif // __OVITO_BACKGROUND_OPERATION_H