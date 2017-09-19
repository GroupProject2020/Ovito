///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2015) Alexander Stukowski
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
#include <plugins/crystalanalysis/objects/dislocations/DislocationDisplay.h>
#include <plugins/crystalanalysis/modifier/dxa/StructureAnalysis.h>
#include <plugins/particles/modifier/analysis/StructureIdentificationModifier.h>
#include <plugins/mesh/surface/SurfaceMesh.h>
#include <plugins/mesh/surface/SurfaceMeshDisplay.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/*
 * Extracts dislocation lines from a crystal.
 */
class OVITO_CRYSTALANALYSIS_EXPORT DislocationAnalysisModifier : public StructureIdentificationModifier
{
public:

	/// Constructor.
	Q_INVOKABLE DislocationAnalysisModifier(DataSet* dataset);

	/// Creates a new modifier application that refers to this modifier instance.
	virtual OORef<ModifierApplication> createModifierApplication() override;
	
protected:

	/// Handles reference events sent by reference targets of this object.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;
	
private:

	/// The type of crystal to be analyzed.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(StructureAnalysis::LatticeStructureType, inputCrystalStructure, setInputCrystalStructure);

	/// The maximum length of trial circuits.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, maxTrialCircuitSize, setMaxTrialCircuitSize);

	/// The maximum elongation of Burgers circuits while they are being advanced.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, circuitStretchability, setCircuitStretchability);

	/// Controls the output of the interface mesh.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, outputInterfaceMesh, setOutputInterfaceMesh);

	/// Restricts the identification to perfect lattice dislocations.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, onlyPerfectDislocations, setOnlyPerfectDislocations);

	/// The catalog of structure patterns.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(PatternCatalog, patternCatalog, setPatternCatalog);

	/// The display object for rendering the defect mesh.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(SurfaceMeshDisplay, defectMeshDisplay, setDefectMeshDisplay);

	/// The display object for rendering the interface mesh.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(SurfaceMeshDisplay, interfaceMeshDisplay, setInterfaceMeshDisplay);

	/// The display object for rendering the dislocations.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(DislocationDisplay, dislocationDisplay, setDislocationDisplay);

	/// The number of iterations of the mesh smoothing algorithm.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, defectMeshSmoothingLevel, setDefectMeshSmoothingLevel);
	
	/// Stores whether smoothing is enabled.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, lineSmoothingEnabled, setLineSmoothingEnabled);

	/// Controls the degree of smoothing applied to the dislocation lines.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, lineSmoothingLevel, setLineSmoothingLevel);

	/// Stores whether coarsening is enabled.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, lineCoarseningEnabled, setLineCoarseningEnabled);

	/// Controls the coarsening of dislocation lines.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, linePointInterval, setLinePointInterval);

	Q_OBJECT
	OVITO_CLASS

	Q_CLASSINFO("DisplayName", "Dislocation analysis (DXA)");
	Q_CLASSINFO("ModifierCategory", "Analysis");
};

/**
 * \brief The type of ModifierApplication created for a PolyhedralTemplateMatchingModifier 
 *        when it is inserted into in a data pipeline. It stores the last computation results
 *        so that they can be displayed in the modifier's user interface.
 */
 class OVITO_PARTICLES_EXPORT DislocationAnalysisModifierApplication : public StructureIdentificationModifierApplication
 {
 public:
 
	 /// Constructor.
	 Q_INVOKABLE DislocationAnalysisModifierApplication(DataSet* dataset) : StructureIdentificationModifierApplication(dataset) {}

	/// Returns the number of segments found per dislocation type.
	const std::map<BurgersVectorFamily*,int>& segmentCounts() const { return _segmentCounts; }
	
	/// Returns the total length of segments found per dislocation type.
	const std::map<BurgersVectorFamily*,FloatType>& dislocationLengths() const { return _dislocationLengths; }

	void setResults(std::map<BurgersVectorFamily*,int> segmentCounts, std::map<BurgersVectorFamily*,FloatType> dislocationLengths) {
		_segmentCounts = std::move(segmentCounts);
		_dislocationLengths = std::move(dislocationLengths);
		notifyDependents(ReferenceEvent::ObjectStatusChanged);
	}
	 
private:
 
	/// The number of segments found per dislocation type.
	std::map<BurgersVectorFamily*,int> _segmentCounts;
	
	/// The total length of segments found per dislocation type.
	std::map<BurgersVectorFamily*,FloatType> _dislocationLengths;
		
	Q_OBJECT
	OVITO_CLASS
};

}	// End of namespace
}	// End of namespace
}	// End of namespace


