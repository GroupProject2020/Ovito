////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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
#include <ovito/core/dataset/data/TransformingDataVis.h>
#include <ovito/core/rendering/SceneRenderer.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/crystalanalysis/objects/DislocationNetworkObject.h>
#include <ovito/crystalanalysis/objects/Microstructure.h>

namespace Ovito { namespace CrystalAnalysis {

/**
 * \brief This information record is attached to the dislocation segments by the DislocationVis when rendering
 * them in the viewports. It facilitates the picking of dislocations with the mouse.
 */
class OVITO_CRYSTALANALYSIS_EXPORT DislocationPickInfo : public ObjectPickInfo
{
	Q_OBJECT
	OVITO_CLASS(DislocationPickInfo)

public:

	/// Constructor.
	DislocationPickInfo(DislocationVis* visElement, const DislocationNetworkObject* dislocationObj, std::vector<int>&& subobjToSegmentMap) :
		_visElement(visElement), _dislocationObj(dislocationObj), _subobjToSegmentMap(std::move(subobjToSegmentMap)) {}

	/// Constructor.
	DislocationPickInfo(DislocationVis* visElement, const Microstructure* microstructureObj, std::vector<int>&& subobjToSegmentMap) :
		_visElement(visElement), _microstructureObj(microstructureObj), _subobjToSegmentMap(std::move(subobjToSegmentMap)) {}

	/// The data object containing the dislocations.
	const DislocationNetworkObject* dislocationObj() const { return _dislocationObj; }

	/// The data object containing the dislocations.
	const Microstructure* microstructureObj() const { return _microstructureObj; }

	/// Returns the vis element that rendered the dislocations.
	DislocationVis* visElement() const { return _visElement; }

	/// \brief Given an sub-object ID returned by the Viewport::pick() method, looks up the
	/// corresponding dislocation segment.
	int segmentIndexFromSubObjectID(quint32 subobjID) const {
		if(subobjID < _subobjToSegmentMap.size())
			return _subobjToSegmentMap[subobjID];
		else
			return -1;
	}

	/// Returns a human-readable string describing the picked object, which will be displayed in the status bar by OVITO.
	virtual QString infoString(PipelineSceneNode* objectNode, quint32 subobjectId) override;

private:

	/// The data object containing the dislocations.
	OORef<DislocationNetworkObject> _dislocationObj;

	/// The data object containing the dislocations.
	OORef<Microstructure> _microstructureObj;

	/// The vis element that rendered the dislocations.
	OORef<DislocationVis> _visElement;

	/// This array is used to map sub-object picking IDs back to dislocation segments.
	std::vector<int> _subobjToSegmentMap;
};

/**
 * \brief A visualization element rendering dislocation lines.
 */
class OVITO_CRYSTALANALYSIS_EXPORT DislocationVis : public TransformingDataVis
{
	Q_OBJECT
	OVITO_CLASS(DislocationVis)

	Q_CLASSINFO("DisplayName", "Dislocations");

public:

	enum LineColoringMode {
		ColorByDislocationType,
		ColorByBurgersVector,
		ColorByCharacter
	};
	Q_ENUMS(LineColoringMode);

public:

	/// \brief Constructor.
	Q_INVOKABLE DislocationVis(DataSet* dataset);

	/// \brief Lets the vis element render a data object.
	virtual void render(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode) override;

	/// Computes the bounding box of the object.
	virtual Box3 boundingBox(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval) override;

	/// \brief Renders an overlay marker for a single dislocation segment.
	void renderOverlayMarker(TimePoint time, const DataObject* dataObject, const PipelineFlowState& flowState, int segmentIndex, SceneRenderer* renderer, const PipelineSceneNode* contextNode);

	/// \brief Generates a pretty string representation of a Burgers vector.
	static QString formatBurgersVector(const Vector3& b, const MicrostructurePhase* structure);

public:

	Q_PROPERTY(Ovito::ArrowPrimitive::ShadingMode shadingMode READ shadingMode WRITE setShadingMode);

protected:

	/// Lets the vis element transform a data object in preparation for rendering.
	virtual Future<PipelineFlowState> transformDataImpl(TimePoint time, const DataObject* dataObject, PipelineFlowState&& flowState, const PipelineFlowState& cachedState, const PipelineSceneNode* contextNode) override;

	/// Clips a dislocation line at the periodic box boundaries.
	void clipDislocationLine(const std::deque<Point3>& line, const SimulationCell& simulationCell, const QVector<Plane3>& clippingPlanes, const std::function<void(const Point3&, const Point3&, bool)>& segmentCallback);

protected:

	/// The rendering width for dislocation lines.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, lineWidth, setLineWidth, PROPERTY_FIELD_MEMORIZE);

	/// The shading mode for dislocation lines.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(ArrowPrimitive::ShadingMode, shadingMode, setShadingMode, PROPERTY_FIELD_MEMORIZE);

	/// The rendering width for Burgers vectors.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, burgersVectorWidth, setBurgersVectorWidth, PROPERTY_FIELD_MEMORIZE);

	/// The scaling factor Burgers vectors.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, burgersVectorScaling, setBurgersVectorScaling, PROPERTY_FIELD_MEMORIZE);

	/// Display color for Burgers vectors.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(Color, burgersVectorColor, setBurgersVectorColor, PROPERTY_FIELD_MEMORIZE);

	/// Controls the display of Burgers vectors.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, showBurgersVectors, setShowBurgersVectors);

	/// Controls the display of the line directions.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, showLineDirections, setShowLineDirections);

	/// Controls how the display color of dislocation lines is chosen.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(LineColoringMode, lineColoringMode, setLineColoringMode);
};

}	// End of namespace
}	// End of namespace
