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

#ifndef __OVITO_CA_DISLOCATION_DISPLAY_H
#define __OVITO_CA_DISLOCATION_DISPLAY_H

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <core/scene/display/DisplayObject.h>
#include <core/rendering/ParticleGeometryBuffer.h>
#include <core/rendering/ArrowGeometryBuffer.h>
#include <core/gui/properties/PropertiesEditor.h>
#include <plugins/particles/data/SimulationCell.h>

namespace CrystalAnalysis {

using namespace Ovito;
using namespace Particles;

/**
 * \brief A display object for the dislocation lines.
 */
class OVITO_CRYSTALANALYSIS_EXPORT DislocationDisplay : public DisplayObject
{
public:

	/// \brief Constructor.
	Q_INVOKABLE DislocationDisplay(DataSet* dataset);

	/// \brief Lets the display object render a scene object.
	virtual void render(TimePoint time, SceneObject* sceneObject, const PipelineFlowState& flowState, SceneRenderer* renderer, ObjectNode* contextNode) override;

	/// \brief Computes the bounding box of the object.
	virtual Box3 boundingBox(TimePoint time, SceneObject* sceneObject, ObjectNode* contextNode, const PipelineFlowState& flowState) override;

	/// \brief Returns the title of this object.
	virtual QString objectTitle() override { return tr("Dislocations"); }

	/// \brief Returns the line width used for dislocation rendering.
	FloatType lineWidth() const { return _lineWidth; }

	/// \brief Sets the line width used for dislocation rendering.
	void setLineWidth(FloatType width) { _lineWidth = width; }

	/// \brief Returns the selected shading mode for dislocation lines.
	ArrowGeometryBuffer::ShadingMode shadingMode() const { return _shadingMode; }

	/// \brief Sets the shading mode for dislocation lines.
	void setShadingMode(ArrowGeometryBuffer::ShadingMode mode) { _shadingMode = mode; }

	/// \brief Given an sub-object ID returned by the Viewport::pick() method, looks up the
	/// corresponding dislocation segment.
	int segmentIndexFromSubObjectID(quint32 subobjID) const {
		if(subobjID < _subobjToSegmentMap.size())
			return _subobjToSegmentMap[subobjID];
		else
			return -1;
	}

	/// \brief Renders an overlay marker for a single dislocation segment.
	void renderOverlayMarker(TimePoint time, SceneObject* sceneObject, const PipelineFlowState& flowState, int segmentIndex, SceneRenderer* renderer, ObjectNode* contextNode);

public:

	Q_PROPERTY(FloatType lineWidth READ lineWidth WRITE setLineWidth)
	Q_PROPERTY(Ovito::ArrowGeometryBuffer::ShadingMode shadingMode READ shadingMode WRITE setShadingMode)

protected:

	/// Clips a dislocation line at the periodic box boundaries.
	void clipDislocationLine(const QVector<Point3>& line, const SimulationCellData& simulationCell, const std::function<void(const Point3&, const Point3&, bool)>& segmentCallback);

protected:

	/// The geometry buffer used to render the dislocation segments.
	OORef<ArrowGeometryBuffer> _segmentBuffer;

	/// The geometry buffer used to render the segment corners.
	OORef<ParticleGeometryBuffer> _cornerBuffer;

	/// This helper structure is used to detect any changes in the input data
	/// that require updating the geometry buffers.
	SceneObjectCacheHelper<
		QPointer<SceneObject>, unsigned int,		// Source object + revision number
		SimulationCellData,							// Simulation cell geometry
		FloatType									// Line width
		> _geometryCacheHelper;

	/// This array is used to map sub-object picking IDs back to dislocation segments.
	std::vector<int> _subobjToSegmentMap;

	/// The cached bounding box.
	Box3 _cachedBoundingBox;

	/// This helper structure is used to detect changes in the input
	/// that require recalculating the bounding box.
	SceneObjectCacheHelper<
		QPointer<SceneObject>, unsigned int,		// Source object + revision number
		SimulationCellData,							// Simulation cell geometry
		FloatType									// Line width
		> _boundingBoxCacheHelper;

	/// Controls the rendering width for dislocation lines.
	PropertyField<FloatType> _lineWidth;

	/// Controls the shading mode for dislocation lines.
	PropertyField<ArrowGeometryBuffer::ShadingMode, int> _shadingMode;

private:

	Q_OBJECT
	OVITO_OBJECT

	DECLARE_PROPERTY_FIELD(_lineWidth);
	DECLARE_PROPERTY_FIELD(_shadingMode);
};

/**
 * \brief A properties editor for the DislocationDisplay class.
 */
class OVITO_CRYSTALANALYSIS_EXPORT DislocationDisplayEditor : public PropertiesEditor
{
public:

	/// Constructor.
	Q_INVOKABLE DislocationDisplayEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

	Q_OBJECT
	OVITO_OBJECT
};

};	// End of namespace

#endif // __OVITO_CA_DISLOCATION_DISPLAY_H