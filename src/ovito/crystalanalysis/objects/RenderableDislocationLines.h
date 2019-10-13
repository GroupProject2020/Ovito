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


#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/crystalanalysis/data/ClusterVector.h>
#include <ovito/core/dataset/data/TransformedDataObject.h>

namespace Ovito { namespace CrystalAnalysis {

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
