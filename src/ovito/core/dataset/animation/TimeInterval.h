////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2014 Alexander Stukowski
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
#include <ovito/core/utilities/io/SaveStream.h>
#include <ovito/core/utilities/io/LoadStream.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Anim)

/**
 * \brief A point in animation time.
 *
 * One animation time unit is 1/4800 of a second in real time.
 *
 * Note that this is an integer data type. Times are measured
 * in discrete steps of 1/4800 of a second to avoid rounding errors.
 */
typedef int TimePoint;

/// The number of time ticks per second.
enum { TICKS_PER_SECOND = 4800 };

/// Returns the smallest possible time value.
Q_DECL_CONSTEXPR inline TimePoint TimeNegativeInfinity() noexcept {
	return std::numeric_limits<TimePoint>::lowest();
}

/// Returns the largest possible time value.
Q_DECL_CONSTEXPR inline TimePoint TimePositiveInfinity() noexcept {
	return std::numeric_limits<TimePoint>::max();
}

/// Converts time tick units to seconds.
Q_DECL_CONSTEXPR inline FloatType TimeToSeconds(TimePoint t) noexcept {
	return (FloatType)t / TICKS_PER_SECOND;
}

/// Converts seconds to internal time ticks.
inline TimePoint TimeFromSeconds(FloatType timeInSeconds) noexcept {
	return (TimePoint)std::ceil(timeInSeconds * TICKS_PER_SECOND + FloatType(0.5));
}

/**
 * \brief An interval in (animation) time, which is defined by a start and an end time.
 */
class TimeInterval
{
public:

	/// \brief Creates an empty time interval.
	///
	/// Both start time and end time are initialized to negative infinity.
	Q_DECL_CONSTEXPR TimeInterval() noexcept : _start(TimeNegativeInfinity()), _end(TimeNegativeInfinity()) {}

	/// \brief Initializes the interval with start and end values.
	/// \param start The start time of the time interval.
	/// \param end The end time (including) of the time interval.
	Q_DECL_CONSTEXPR TimeInterval(TimePoint start, TimePoint end) noexcept : _start(start), _end(end) {}

	/// \brief Initializes the interval to an instant time.
	/// \param time The time where the interval starts and ends.
	Q_DECL_CONSTEXPR TimeInterval(TimePoint time) noexcept : _start(time), _end(time) {}

	/// \brief Returns the start time of the interval.
	/// \return The beginning of the time interval.
	Q_DECL_CONSTEXPR TimePoint start() const noexcept { return _start; }

	/// \brief Returns the end time of the interval.
	/// \return The time at which the interval end.
	Q_DECL_CONSTEXPR TimePoint end() const noexcept { return _end; }

	/// \brief Sets the start time of the interval.
	/// \param start The new start time.
	void setStart(TimePoint start) noexcept { _start = start; }

	/// \brief Sets the end time of the interval.
	/// \param end The new end time.
	void setEnd(TimePoint end) noexcept { _end = end; }

	/// \brief Checks if this is an empty time interval.
	/// \return \c true if the start time of the interval is behind the end time or if the
	///         end time is negative infinity (TimeNegativeInfinity);
	///         \c false otherwise.
	/// \sa setEmpty()
	Q_DECL_CONSTEXPR bool isEmpty() const noexcept { return (end() == TimeNegativeInfinity() || start() > end()); }

	/// \brief Returns whether this is the infinite time interval.
	/// \return \c true if the start time is negative infinity and the end time of the interval is positive infinity.
	/// \sa setInfinite()
	Q_DECL_CONSTEXPR bool isInfinite() const noexcept { return (end() == TimePositiveInfinity() && start() == TimeNegativeInfinity()); }

	/// \brief Returns the duration of the time interval.
	/// \return The difference between the end and the start time.
	/// \sa setDuration()
	Q_DECL_CONSTEXPR TimePoint duration() const noexcept { return end() - start(); }

	/// \brief Sets the duration of the time interval.
	/// \param duration The new duration of the interval.
	///
	/// This method changes the end time of the interval to be
	/// start() + duration().
	///
	/// \sa duration()
	void setDuration(TimePoint duration) noexcept { setEnd(start() + duration); }

	/// \brief Sets this interval's start time to negative infinity and it's end time to positive infinity.
	/// \sa isInfinite()
	void setInfinite() noexcept {
		setStart(TimeNegativeInfinity());
		setEnd(TimePositiveInfinity());
	}

	/// \brief Sets this interval's start and end time to negative infinity.
	/// \sa isEmpty()
	void setEmpty() noexcept {
		setStart(TimeNegativeInfinity());
		setEnd(TimeNegativeInfinity());
	}

	/// \brief Sets this interval's start and end time to the instant time given.
	/// \param time This value is assigned to both, the start and the end time of the interval.
	void setInstant(TimePoint time) noexcept {
		setStart(time);
		setEnd(time);
	}

	/// \brief Compares two intervals for equality.
	/// \param other The interval to compare with.
	/// \return \c true if start and end time of both intervals are equal.
	bool operator==(const TimeInterval& other) const noexcept { return (other.start() == start() && other.end() == end()); }

	/// \brief Compares two intervals for inequality.
	/// \param other The interval to compare with.
	/// \return \c true if start or end time of both intervals are not equal.
	bool operator!=(const TimeInterval& other) const noexcept { return (other.start() != start() || other.end() != end()); }

	/// \brief Assignment operator.
	/// \param other The interval to copy.
	/// \return This interval instance.
	TimeInterval& operator=(const TimeInterval& other) noexcept {
		setStart(other.start());
		setEnd(other.end());
		return *this;
	}

	/// \brief Returns whether a time lies between start and end time of this interval.
	/// \param time The time to check.
	/// \return \c true if \a time is equal or larger than start() and smaller or equal than end().
	Q_DECL_CONSTEXPR bool contains(TimePoint time) const noexcept {
		return (start() <= time && time <= end());
	}

	/// \brief Intersects this interval with the another one.
	/// \param other Another time interval.
	///
	/// Start and end time of this interval are such that they include the interval \a other as well as \c this interval.
	void intersect(const TimeInterval& other) noexcept {
		if(end() < other.start()
			|| start() > other.end()
			|| other.isEmpty()) {
			setEmpty();
		}
		else if(!other.isInfinite()) {
			setStart(std::max(start(), other.start()));
			setEnd(std::min(end(), other.end()));
			OVITO_ASSERT(start() <= end());
		}
	}

	/// Tests if two time interval overlap (either full or partially).
	Q_DECL_CONSTEXPR bool overlap(const TimeInterval& iv) const noexcept {
		if(isEmpty() || iv.isEmpty()) return false;
		if(start() >= iv.start() && start() <= iv.end()) return true;
		if(end() >= iv.start() && end() <= iv.end()) return true;
		return (iv.start() >= start() && iv.start() <= end());
	}

	/// Return the infinite time interval that contains all time values.
	static Q_DECL_CONSTEXPR TimeInterval infinite() noexcept { return TimeInterval(TimeNegativeInfinity(), TimePositiveInfinity()); }

	/// Return the empty time interval that contains no time values.
	static Q_DECL_CONSTEXPR TimeInterval empty() noexcept { return TimeInterval(TimeNegativeInfinity()); }

private:

	TimePoint _start, _end;

	friend LoadStream& operator>>(LoadStream& stream, TimeInterval& iv);
};

/// \brief Writes a time interval to a binary output stream.
/// \param stream The output stream.
/// \param iv The time interval to write to the output stream \a stream.
/// \return The output stream \a stream.
/// \relates TimeInterval
inline SaveStream& operator<<(SaveStream& stream, const TimeInterval& iv)
{
	return stream << iv.start() << iv.end();
}

/// \brief Reads a time interval from a binary input stream.
/// \param stream The input stream.
/// \param iv Reference to a variable where the parsed data will be stored.
/// \return The input stream \a stream.
/// \relates TimeInterval
inline LoadStream& operator>>(LoadStream& stream, TimeInterval& iv)
{
	stream >> iv._start >> iv._end;
	return stream;
}

/// \brief Writes a time interval to the debug stream.
/// \relates TimeInterval
inline QDebug operator<<(QDebug stream, const TimeInterval& iv)
{
	stream.nospace() << "[" << iv.start() << ", " << iv.end() << "]";
	return stream.space();
}

/**
 * This data structure manages the union of multiple, non-overlapping animation time intervals.
 */
class TimeIntervalUnion
{
private:

	/// This underlying list of non-overlapping time intervals.
	QVarLengthArray<TimeInterval, 2> _intervals;

public:

	using const_iterator = decltype(_intervals)::const_iterator;
	using reverse_iterator = decltype(_intervals)::reverse_iterator;
	using iterator = decltype(_intervals)::iterator;
	using reference = decltype(_intervals)::reference;
	using size_type = decltype(_intervals)::size_type;
	using value_type = decltype(_intervals)::value_type;

	/// Constructs an empty union of intervals.
	TimeIntervalUnion() = default;

	/// Constructs a union that includes only the given animation time instant.
	explicit TimeIntervalUnion(TimePoint time) : _intervals{{ TimeInterval(time) }} {}

	/// Add a time interval to the union.
	void add(TimeInterval iv) {
		if(iv.isEmpty()) return;

		// Subtract existing intervals from interval to be added.
		for(iterator iter = _intervals.begin(); iter != _intervals.end(); ) {
			// Erase existing intervals that are completely contained in the interval to be added.
			if(iv.start() <= iter->start() && iv.end() >= iter->end()) {
				iter = _intervals.erase(iter);
			}
			else {
				if(iv.start() >= iter->start() && iv.start() <= iter->end())
					iv.setStart(iter->end() + 1);
				if(iv.end() >= iter->start() && iv.end() <= iter->end())
					iv.setEnd(iter->start() - 1);
				if(iv.start() > iv.end())
					return;
				++iter;
			}
		}
		_intervals.push_back(iv);

		// TODO: Merge adjacent time intervals.
	}

	// Inherited const methods from QVarLengthArray.
	auto begin() const { return _intervals.begin(); }
	auto end() const { return _intervals.end(); }
	auto cbegin() const { return _intervals.cbegin(); }
	auto cend() const { return _intervals.cend(); }
	auto rbegin() const { return _intervals.rbegin(); }
	auto rend() const { return _intervals.rend(); }
	auto crbegin() const { return _intervals.crbegin(); }
	auto crend() const { return _intervals.crend(); }
	void clear() { _intervals.clear(); }
	size_type size() { return _intervals.size(); }
	bool empty() const { return _intervals.empty(); }
	const value_type& front() const { return _intervals.front(); }
	const value_type& back() const { return _intervals.back(); }
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::TimeInterval);
Q_DECLARE_TYPEINFO(Ovito::TimeInterval, Q_MOVABLE_TYPE);
