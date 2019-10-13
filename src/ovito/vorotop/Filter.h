////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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


#include <ovito/vorotop/VoroTopPlugin.h>

namespace Ovito { namespace VoroTop {

/**
 * A filter is a specification of topological types, recorded with Weinberg codes.
 */
class OVITO_VOROTOP_EXPORT Filter
{
public:

	/// Data type holding a single Weinberg vector.
	using WeinbergVector = std::vector<int>;

    /// Maximum edges and vertices of type in Filter
    int maximumEdges;
    int maximumVertices;

public:

	/// Loads the filter definition from the given input stream.
	bool load(CompressedTextReader& stream, bool readHeaderOnly, Task& operation);

	/// Returns the comment text loaded from the filter definition file.
	const QString& filterDescription() const { return _filterDescription; }

	/// Returns the number of Weinberg vectors of this filter.
	size_t size() const { return _entries.size(); }

	/// Looks up the structure type associated with the given Weinberg vector.
	/// Return 0 if Weinberg vector is not in filter.
	int findType(const WeinbergVector& wvector) const {
		auto iter = _entries.find(wvector);
		if(iter != _entries.end())
			return iter->second;
		else
			return 0;
	}

	/// Returns the number of structure types defined in this filter (including the "other" type).
	int structureTypeCount() const { return _structureTypeLabels.size(); }

	/// Returns the name of the structure type with the given index.
	const QString& structureTypeLabel(int index) const { return _structureTypeLabels[index]; }

	/// Returns the description string of the structure type with the given index.
	const QString& structureTypeDescription(int index) const { return _structureTypeDescriptions[index]; }

private:

	/// Names of the structures types this filter maps to, e.g. "FCC", "FCC-HCP", "BCC", etc.
	QStringList _structureTypeLabels;

	/// Descriptions strings of the structures types.
	QStringList _structureTypeDescriptions;

	/// Mapping from Weinberg vectors to structure types.
	std::map<WeinbergVector, int> _entries;

	/// Comment text loaded from the filter definition file.
	QString _filterDescription;
};

}	// End of namespace
}	// End of namespace
