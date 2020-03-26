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
#include <ovito/core/app/Application.h>
#include "Task.h"

#include <future>

namespace Ovito {

template<class Function, typename T>
bool parallelFor(
		T loopCount,
		Task& promise,
		Function kernel,
		T progressChunkSize = 1024)
{
	promise.setProgressMaximum(loopCount / progressChunkSize);
	promise.setProgressValue(0);

	std::vector<std::future<void>> workers;
	size_t num_threads = Application::instance()->idealThreadCount();
	T chunkSize = loopCount / num_threads;
	T startIndex = 0;
	T endIndex = chunkSize;
	for(size_t t = 0; t < num_threads; t++) {
		if(t == num_threads - 1)
			endIndex += loopCount % num_threads;
		workers.push_back(std::async(std::launch::async, [&promise, &kernel, startIndex, endIndex, progressChunkSize]() {
			for(T i = startIndex; i < endIndex;) {
				// Execute kernel.
				kernel(i);

				i++;

				// Update progress indicator.
				if((i % progressChunkSize) == 0) {
					OVITO_ASSERT(i != 0);
					promise.incrementProgressValue();
				}
				if(promise.isCanceled())
					return;
			}
		}));
		startIndex = endIndex;
		endIndex += chunkSize;
	}

	for(auto& t : workers)
		t.wait();
	for(auto& t : workers)
		t.get();

	promise.incrementProgressValue(loopCount % progressChunkSize);
	return !promise.isCanceled();
}

template<class Function, typename T>
void parallelFor(T loopCount, Function kernel)
{
	std::vector<std::future<void>> workers;
	size_t num_threads = Application::instance()->idealThreadCount();
	if(num_threads > loopCount) {
		if(loopCount <= 0) return;
		num_threads = loopCount;
	}
	T chunkSize = loopCount / num_threads;
	T startIndex = 0;
	T endIndex = chunkSize;
	for(size_t t = 0; t < num_threads; t++) {
		if(t == num_threads - 1) {
			OVITO_ASSERT(endIndex + (loopCount % num_threads) == loopCount);
			endIndex = loopCount;
			for(T i = startIndex; i < endIndex; ++i) {
				kernel(i);
			}
		}
		else {
			OVITO_ASSERT(endIndex <= loopCount);
			workers.push_back(std::async(std::launch::async, [&kernel, startIndex, endIndex]() {
				for(T i = startIndex; i < endIndex; ++i) {
					kernel(i);
				}
			}));
		}
		startIndex = endIndex;
		endIndex += chunkSize;
	}

	for(auto& t : workers)
		t.wait();
	for(auto& t : workers)
		t.get();
}

template<class Function>
bool parallelForChunks(size_t loopCount, Task& promise, Function kernel)
{
	std::vector<std::future<void>> workers;
	size_t num_threads = Application::instance()->idealThreadCount();
	if(num_threads > loopCount) {
		if(loopCount <= 0) return true;
		num_threads = loopCount;
	}
	size_t chunkSize = loopCount / num_threads;
	size_t startIndex = 0;
	for(size_t t = 0; t < num_threads; t++) {
		if(t == num_threads - 1) {
			chunkSize += loopCount % num_threads;
			OVITO_ASSERT(startIndex + chunkSize == loopCount);
			kernel(startIndex, chunkSize, promise);
		}
		else {
			workers.push_back(std::async(std::launch::async, [&kernel, startIndex, chunkSize, &promise]() {
				kernel(startIndex, chunkSize, promise);
			}));
		}
		startIndex += chunkSize;
	}
	for(auto& t : workers)
		t.wait();
	for(auto& t : workers)
		t.get();

	return !promise.isCanceled();
}


template<class Function>
void parallelForChunks(size_t loopCount, Function kernel)
{
	std::vector<std::future<void>> workers;
	size_t num_threads = Application::instance()->idealThreadCount();
	if(num_threads > loopCount) {
		if(loopCount <= 0) return;
		num_threads = loopCount;
	}
	size_t chunkSize = loopCount / num_threads;
	size_t startIndex = 0;
	for(size_t t = 0; t < num_threads; t++) {
		if(t == num_threads - 1) {
			chunkSize += loopCount % num_threads;
			OVITO_ASSERT(startIndex + chunkSize == loopCount);
			kernel(startIndex, chunkSize);
		}
		else {
			workers.push_back(std::async(std::launch::async, [&kernel, startIndex, chunkSize]() {
				kernel(startIndex, chunkSize);
			}));
		}
		startIndex += chunkSize;
	}
	for(auto& t : workers)
		t.wait();
	for(auto& t : workers)
		t.get();
}

}	// End of namespace
