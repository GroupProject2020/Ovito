////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 Alexander Stukowski
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
