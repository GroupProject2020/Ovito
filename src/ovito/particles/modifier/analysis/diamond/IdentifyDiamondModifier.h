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


#include <ovito/particles/Particles.h>
#include <ovito/particles/modifier/analysis/StructureIdentificationModifier.h>

namespace Ovito { namespace Particles {

/**
 * \brief A modifier that identifies local diamond structures.
 */
class OVITO_PARTICLES_EXPORT IdentifyDiamondModifier : public StructureIdentificationModifier
{
	Q_OBJECT
	OVITO_CLASS(IdentifyDiamondModifier)

	Q_CLASSINFO("DisplayName", "Identify diamond structure");
	Q_CLASSINFO("ModifierCategory", "Structure identification");

public:

	/// The structure types recognized by the modifier.
	enum StructureType {
		OTHER = 0,					//< Unidentified structure
		CUBIC_DIAMOND,				//< Cubic diamond structure
		CUBIC_DIAMOND_FIRST_NEIGH,	//< First neighbor of a cubic diamond atom
		CUBIC_DIAMOND_SECOND_NEIGH,	//< Second neighbor of a cubic diamond atom
		HEX_DIAMOND,				//< Hexagonal diamond structure
		HEX_DIAMOND_FIRST_NEIGH,	//< First neighbor of a hexagonal diamond atom
		HEX_DIAMOND_SECOND_NEIGH,	//< Second neighbor of a hexagonal diamond atom

		NUM_STRUCTURE_TYPES 	//< This just counts the number of defined structure types.
	};
	Q_ENUMS(StructureType);

public:

	/// Constructor.
	Q_INVOKABLE IdentifyDiamondModifier(DataSet* dataset);

protected:

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input) override;

private:

	/// Analysis engine that performs the structure identification
	class DiamondIdentificationEngine : public StructureIdentificationEngine
	{
	public:

		/// Constructor.
		DiamondIdentificationEngine(ParticleOrderingFingerprint fingerprint, ConstPropertyPtr positions, const SimulationCell& simCell, QVector<bool> typesToIdentify, ConstPropertyPtr selection) :
			StructureIdentificationEngine(std::move(fingerprint), std::move(positions), simCell, std::move(typesToIdentify), std::move(selection)) {}

		/// Computes the modifier's results.
		virtual void perform() override;

		/// Injects the computed results into the data pipeline.
		virtual void emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;
	};
};

}	// End of namespace
}	// End of namespace
