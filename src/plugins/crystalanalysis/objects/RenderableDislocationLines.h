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


#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/crystalanalysis/data/ClusterVector.h>
#include <core/dataset/data/TransformedDataObject.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * \brief A non-periodic version of the dislocation lines that is generated from a periodic DislocationNetworkObject.
 */
class OVITO_CRYSTALANALYSIS_EXPORT RenderableDislocationLines : public TransformedDataObject
{
	Q_OBJECT
	OVITO_CLASS(RenderableDislocationLines)
	Q_CLASSINFO("DisplayName", "Renderable dislocations");

public:

	/// A linear segment of a dislocation line.
	struct Segment
	{
		/// The two vertices of the segment.
		std::array<Point3, 2> verts;

		/// The Burgers vector of the segment.
		Vector3 burgersVector;

		/// The crystallite the dislocation segment is embedded in.
		int region;

		/// Identifies the original dislocation line this segment is part of.
		int dislocationIndex;

		/// Equal comparison operator.
		bool operator==(const Segment& other) const { return verts == other.verts && dislocationIndex == other.dislocationIndex && burgersVector == other.burgersVector && region == other.region; }
	};

	/// \brief Standard constructor.
	Q_INVOKABLE RenderableDislocationLines(DataSet* dataset) : TransformedDataObject(dataset) {}

	/// \brief Initialization constructor.
	RenderableDislocationLines(TransformingDataVis* creator, const DataObject* sourceData) : TransformedDataObject(creator, sourceData) {}

private:

	/// The list of clipped and wrapped line segments.
	DECLARE_RUNTIME_PROPERTY_FIELD(std::vector<RenderableDislocationLines::Segment>, lineSegments, setLineSegments);
};

}	// End of namespace
}	// End of namespace
}	// End of namespace
