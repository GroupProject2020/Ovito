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
#include <core/dataset/data/TransformingDataVis.h>
#include <core/dataset/data/VersionedDataObjectRef.h>
#include <core/dataset/data/CacheStateHelper.h>
#include <core/rendering/ParticlePrimitive.h>
#include <core/rendering/ArrowPrimitive.h>
#include <core/rendering/SceneRenderer.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationNetworkObject.h>
#include <plugins/crystalanalysis/objects/patterns/PatternCatalog.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

class DislocationVis;	// defined below

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
	DislocationPickInfo(DislocationVis* visElement, DislocationNetworkObject* dislocationObj, PatternCatalog* patternCatalog, std::vector<int>&& subobjToSegmentMap) :
		_visElement(visElement), _dislocationObj(dislocationObj), _patternCatalog(patternCatalog), _subobjToSegmentMap(std::move(subobjToSegmentMap)) {}

	/// The data object containing the dislocations.
	DislocationNetworkObject* dislocationObj() const { return _dislocationObj; }

	/// Returns the vis element that rendered the dislocations.
	DislocationVis* visElement() const { return _visElement; }

	/// Returns the associated pattern catalog.
	PatternCatalog* patternCatalog() const { return _patternCatalog; }

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

	/// The vis element that rendered the dislocations.
	OORef<DislocationVis> _visElement;

	/// The data object containing the lattice structure.
	OORef<PatternCatalog> _patternCatalog;

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
	virtual void render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, PipelineSceneNode* contextNode) override;

	/// Computes the bounding box of the object.
	virtual Box3 boundingBox(TimePoint time, DataObject* dataObject, PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval) override;
	
	/// \brief Renders an overlay marker for a single dislocation segment.
	void renderOverlayMarker(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, int segmentIndex, SceneRenderer* renderer, PipelineSceneNode* contextNode);

	/// \brief Generates a pretty string representation of a Burgers vector.
	static QString formatBurgersVector(const Vector3& b, StructurePattern* structure);

public:

	Q_PROPERTY(Ovito::ArrowPrimitive::ShadingMode shadingMode READ shadingMode WRITE setShadingMode);

protected:

	/// Lets the vis element transform a data object in preparation for rendering.
	virtual Future<PipelineFlowState> transformDataImpl(TimePoint time, DataObject* dataObject, PipelineFlowState&& flowState, const PipelineFlowState& cachedState, PipelineSceneNode* contextNode) override;
	
	/// Clips a dislocation line at the periodic box boundaries.
	void clipDislocationLine(const std::deque<Point3>& line, const SimulationCell& simulationCell, const QVector<Plane3>& clippingPlanes, const std::function<void(const Point3&, const Point3&, bool)>& segmentCallback);

protected:

	/// The geometry buffer used to render the dislocation segments.
	std::shared_ptr<ArrowPrimitive> _segmentBuffer;

	/// The geometry buffer used to render the segment corners.
	std::shared_ptr<ParticlePrimitive> _cornerBuffer;

	/// The buffered geometry used to render the Burgers vectors.
	std::shared_ptr<ArrowPrimitive> _burgersArrowBuffer;

	/// This helper structure is used to detect any changes in the input data
	/// that require updating the geometry buffers.
	CacheStateHelper<
		VersionedDataObjectRef,	// Source object + revision number
		VersionedDataObjectRef,	// Renderable object + revision number
		SimulationCell,			// Simulation cell geometry
		VersionedDataObjectRef,	// The pattern catalog
		FloatType,				// Line width
		bool,					// Burgers vector display
		FloatType,				// Burgers vectors scaling
		FloatType,				// Burgers vector width
		Color,					// Burgers vector color
		LineColoringMode		// Way to color lines
		> _geometryCacheHelper;

	/// The cached bounding box.
	Box3 _cachedBoundingBox;

	/// This helper structure is used to detect changes in the input
	/// that require recalculating the bounding box.
	CacheStateHelper<
		VersionedDataObjectRef,	// Source object + revision number
		SimulationCell,			// Simulation cell geometry
		FloatType,				// Line width
		bool,					// Burgers vector display
		FloatType,				// Burgers vectors scaling
		FloatType				// Burgers vector width
		> _boundingBoxCacheHelper;

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

	/// The data record used for picking dislocations in the viewports.
	OORef<DislocationPickInfo> _pickInfo;
};

}	// End of namespace
}	// End of namespace
}	// End of namespace


