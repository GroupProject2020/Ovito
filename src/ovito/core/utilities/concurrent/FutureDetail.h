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


#include <ovito/core/Core.h>
#include <type_traits>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

namespace detail
{

	template<class T>
	struct is_future : std::false_type {};

	template<typename... T>
	struct is_future<Future<T...>> : std::true_type {};

	template<typename... T>
	struct is_future<SharedFuture<T...>> : std::true_type {};

	/// Part of apply() implementation below.
	template<typename F, class Tuple, std::size_t... I>
	static constexpr decltype(auto) apply_impl(F&& f, Tuple&& t, std::index_sequence<I...>) {
		return f(std::get<I>(std::forward<Tuple>(t))...);
	}

	/// C++14 implementation of std::apply(), which is only available in C++17.
	template<typename F, class Tuple>
	static constexpr decltype(auto) apply(F&& f, Tuple&& t) {
		return apply_impl(
			std::forward<F>(f), std::forward<Tuple>(t),
			std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>{});
	}

	/// Determine the return type of a continuation function object.
	template<typename FC, typename Args>
	struct continuation_func_return_type {
		using type = decltype(apply(std::declval<FC>(), std::declval<Args>()));
	};

	/// Determines if the return type of a continuation function object is 'void'.
	template<typename FC, typename Args>
	struct is_void_continuation_func {
		static constexpr auto value = std::is_void<typename continuation_func_return_type<FC,Args>::type>::value;
	};

	/// For a value type, returns the corresponding Future<> type.
	template<typename T>
	struct future_type_from_value_type : public std::conditional<std::is_void<T>::value, Future<>, Future<T>> {};

	/// Determines the Future type that results from a continuation function.
	///
	///     Future<...> func(...)   ->   Future<...>
	///               T func(...)   ->   Future<T>
	///            void func(...)   ->   Future<>
	///
	template<typename FC, typename Args>
	struct resulting_future_type : public
		std::conditional<is_future<typename continuation_func_return_type<FC,Args>::type>::value,
						typename continuation_func_return_type<FC,Args>::type,
						typename future_type_from_value_type<typename continuation_func_return_type<FC,Args>::type>::type> {};

	/// Determines the type of shared state returned by Future::then() etc.
	///
	///     Future<...> func(...)   ->   TrackingTask
	///               T func(...)   ->   ContinuationTask<tuple<T>>
	///            void func(...)   ->   ContinuationTask<tuple<>>
	///
	template<typename FC, typename Args>
	struct continuation_state_type : public std::conditional<
		!is_future<typename continuation_func_return_type<FC,Args>::type>::value,
		ContinuationTask<typename resulting_future_type<FC,Args>::type::tuple_type>,
		TrackingTask> {};

	/// The simplest implementation of the Executor concept.
	/// The inline executor runs a work function immediately and in place.
	/// See OvitoObjectExecutor for another implementation of the executor concept.
	struct InlineExecutor {
		template<typename F>
		static constexpr decltype(auto) createWork(F&& f) {
			return std::bind(std::forward<F>(f), false);
		}
	};

} // End of namespace

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


