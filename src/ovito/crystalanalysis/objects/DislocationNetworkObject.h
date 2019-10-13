////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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
#include <ovito/crystalanalysis/data/DislocationNetwork.h>
#include <ovito/crystalanalysis/objects/MicrostructurePhase.h>
#include <ovito/stdobj/simcell/PeriodicDomainDataObject.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>

namespace Ovito { namespace CrystalAnalysis {

/**
 * \brief Stores a collection of dislocation segments.
 */
class OVITO_CRYSTALANALYSIS_EXPORT DislocationNetworkObject : public PeriodicDomainDataObject
{
	Q_OBJECT
	OVITO_CLASS(DislocationNetworkObject)

public:

	/// \brief Constructor.
	Q_INVOKABLE DislocationNetworkObject(DataSet* dataset);

	/// Returns the title of this object.
	virtual QString objectTitle() const override { return tr("Dislocations"); }

	/// Returns the data encapsulated by this object after making sure it is not shared with other owners.
	const std::shared_ptr<DislocationNetwork>& modifiableStorage();

	/// Returns the list of dislocation segments.
	const std::vector<DislocationSegment*>& segments() const { return storage()->segments(); }

	/// Returns the list of dislocation segments.
	const std::vector<DislocationSegment*>& modifiableSegments() { return modifiableStorage()->segments(); }

	/// Adds a new crystal structures to the list.
	void addCrystalStructure(MicrostructurePhase* structure) { _crystalStructures.push_back(this, PROPERTY_FIELD(crystalStructures), structure); }

	/// Removes a crystal structure.
	void removeCrystalStructure(int index) { _crystalStructures.remove(this, PROPERTY_FIELD(crystalStructures), index); }

	/// Returns the crystal structure with the given ID, or null if no such structure exists.
	MicrostructurePhase* structureById(int id) const {
		for(MicrostructurePhase* stype : crystalStructures())
			if(stype->numericId() == id)
				return stype;
		return nullptr;
	}

	/// Returns whether this data object wants to be shown in the pipeline editor under the data source section.
	virtual bool showInPipelineEditor() const override { return true; }

private:

	/// The internal data.
	DECLARE_RUNTIME_PROPERTY_FIELD(std::shared_ptr<DislocationNetwork>, storage, setStorage);

	/// List of crystal structures.
	DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD(MicrostructurePhase, crystalStructures, setCrystalStructures);
};

}	// End of namespace
}	// End of namespace
