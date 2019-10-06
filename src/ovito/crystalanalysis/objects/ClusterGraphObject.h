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

#pragma once


#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/crystalanalysis/data/ClusterGraph.h>
#include <ovito/core/dataset/data/DataObject.h>

namespace Ovito { namespace CrystalAnalysis {

/**
 * \brief A graph of atomic clusters.
 */
class OVITO_CRYSTALANALYSIS_EXPORT ClusterGraphObject : public DataObject
{
	Q_OBJECT
	OVITO_CLASS(ClusterGraphObject)

public:

	/// \brief Constructor.
	Q_INVOKABLE ClusterGraphObject(DataSet* dataset);

	/// Returns the title of this object.
	virtual QString objectTitle() const override { return tr("Clusters"); }

	/// Returns the list of nodes in the cluster graph.
	const std::vector<Cluster*>& clusters() const { return storage()->clusters(); }

	/// Looks up the cluster with the given ID.
	Cluster* findCluster(int id) const { return storage()->findCluster(id); }

private:

	/// The internal data.
	DECLARE_RUNTIME_PROPERTY_FIELD(std::shared_ptr<ClusterGraph>, storage, setStorage);
};

}	// End of namespace
}	// End of namespace
