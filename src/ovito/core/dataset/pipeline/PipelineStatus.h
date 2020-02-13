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

namespace Ovito {

/**
 * \brief Stores status information associated with an evaluation of the modification pipeline.
 */
class OVITO_CORE_EXPORT PipelineStatus
{
public:

	enum StatusType {
		Success,		//< Indicates that the evaluation was success full.
		Warning,		//< Indicates that a modifier has issued a warning.
		Error			//< Indicates that the evaluation failed.
	};

	/// Default constructor that creates a status object with status StatusType::Success and an empty info text.
	PipelineStatus() = default;

	/// Constructs a status object with the given status and optional text string describing the status.
	PipelineStatus(StatusType t, const QString& text = {}) : _type(t), _text(text) {}

	/// Constructs a status object with success status and a text string describing the status.
	PipelineStatus(const QString& text) : _text(text) {}

	/// Returns the type of status stores in this object.
	StatusType type() const { return _type; }

	/// Changes the type of the status.
	void setType(StatusType type) { _type = type; }

	/// Returns a text string describing the status.
	const QString& text() const { return _text; }

	/// Changes the text string describing the status.
	void setText(const QString& text) { _text = text; }

	/// Tests two status objects for equality.
	bool operator==(const PipelineStatus& other) const {
		return (_type == other._type) && (_text == other._text);
	}

	/// Tests two status objects for inequality.
	bool operator!=(const PipelineStatus& other) const { return !(*this == other); }

private:

	/// The status.
	StatusType _type = Success;

	/// A human-readable string describing the status.
	QString _text;

	friend SaveStream& operator<<(SaveStream& stream, const PipelineStatus& s);
	friend LoadStream& operator>>(LoadStream& stream, PipelineStatus& s);
};

/// \brief Writes a status object to a file stream.
/// \param stream The output stream.
/// \param s The status to write to the output stream \a stream.
/// \return The output stream \a stream.
/// \relates PipelineStatus
inline SaveStream& operator<<(SaveStream& stream, const PipelineStatus& s)
{
	stream.beginChunk(0x02);
	stream << s._type;
	stream << s._text;
	stream.endChunk();
	return stream;
}

/// \brief Reads a status object from a binary input stream.
/// \param stream The input stream.
/// \param s Reference to a variable where the parsed data will be stored.
/// \return The input stream \a stream.
/// \relates PipelineStatus
inline LoadStream& operator>>(LoadStream& stream, PipelineStatus& s)
{
	quint32 version = stream.expectChunkRange(0x0, 0x02);
	stream >> s._type;
	stream >> s._text;
	if(version <= 0x01)
		stream >> s._text;
	stream.closeChunk();
	return stream;
}

/// \brief Writes a status object to the log stream.
/// \relates PipelineStatus
inline QDebug operator<<(QDebug debug, const PipelineStatus& s)
{
	switch(s.type()) {
	case PipelineStatus::Success: debug << "Success"; break;
	case PipelineStatus::Warning: debug << "Warning"; break;
	case PipelineStatus::Error: debug << "Error"; break;
	}
	if(s.text().isEmpty() == false)
		debug << s.text();
	return debug;
}

}	// End of namespace
