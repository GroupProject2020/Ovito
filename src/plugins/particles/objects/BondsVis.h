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


#include <plugins/particles/Particles.h>
#include <core/dataset/data/DataVis.h>
#include <core/dataset/data/VersionedDataObjectRef.h>
#include <core/dataset/data/CacheStateHelper.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <core/rendering/ArrowPrimitive.h>
#include <core/rendering/SceneRenderer.h>
#include "ParticleProperty.h"
#include "BondProperty.h"

namespace Ovito { namespace Particles {

/**
 * \brief A visualization element for rendering bonds.
 */
class OVITO_PARTICLES_EXPORT BondsVis : public DataVis
{
	Q_OBJECT
	OVITO_CLASS(BondsVis)
	Q_CLASSINFO("DisplayName", "Bonds");
	
public:

	/// \brief Constructor.
	Q_INVOKABLE BondsVis(DataSet* dataset);

	/// \brief Renders the associated data object.
	virtual void render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, PipelineSceneNode* contextNode) override;

	/// \brief Computes the display bounding box of the data object.
	virtual Box3 boundingBox(TimePoint time, DataObject* dataObject, PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval) override;

	/// Returns the display color used for selected bonds.
	Color selectionBondColor() const { return Color(1,0,0); }

	/// Determines the display colors of half-bonds.
	/// Returns an array with two colors per full bond, because the two half-bonds may have different colors.
	std::vector<Color> halfBondColors(size_t particleCount, BondProperty* topologyProperty,
			BondProperty* bondColorProperty, BondProperty* bondTypeProperty, BondProperty* bondSelectionProperty,
			ParticlesVis* particleDisplay, ParticleProperty* particleColorProperty, ParticleProperty* particleTypeProperty);

public:

    Q_PROPERTY(Ovito::ArrowPrimitive::ShadingMode shadingMode READ shadingMode WRITE setShadingMode);
    Q_PROPERTY(Ovito::ArrowPrimitive::RenderingQuality renderingQuality READ renderingQuality WRITE setRenderingQuality);

protected:

	/// Controls the display width of bonds.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, bondWidth, setBondWidth, PROPERTY_FIELD_MEMORIZE);

	/// Controls the color of the bonds.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(Color, bondColor, setBondColor, PROPERTY_FIELD_MEMORIZE);

	/// Controls whether bonds colors are derived from particle colors.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, useParticleColors, setUseParticleColors, PROPERTY_FIELD_MEMORIZE);

	/// Controls the shading mode for bonds.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(ArrowPrimitive::ShadingMode, shadingMode, setShadingMode, PROPERTY_FIELD_MEMORIZE);

	/// Controls the rendering quality mode for bonds.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ArrowPrimitive::RenderingQuality, renderingQuality, setRenderingQuality);

	/// The buffered geometry used to render the bonds.
	std::shared_ptr<ArrowPrimitive> _buffer;

	/// This helper structure is used to detect any changes in the input data
	/// that require updating the geometry buffer.
	CacheStateHelper<
		VersionedDataObjectRef,		// Bond topology property + revision number
		VersionedDataObjectRef,		// Bond PBC vector property + revision number
		VersionedDataObjectRef,		// Particle position property + revision number
		VersionedDataObjectRef,		// Particle color property + revision number
		VersionedDataObjectRef,		// Particle type property + revision number
		VersionedDataObjectRef,		// Bond color property + revision number
		VersionedDataObjectRef,		// Bond type property + revision number
		VersionedDataObjectRef,		// Bond selection property + revision number
		VersionedDataObjectRef,		// Simulation cell + revision number
		FloatType,					// Bond width
		Color,						// Bond color
		bool						// Use particle colors
	> _geometryCacheHelper;

	/// The bounding box that includes all bonds.
	Box3 _cachedBoundingBox;

	/// This helper structure is used to detect changes in the input data
	/// that require recomputing the bounding box.
	CacheStateHelper<
		VersionedDataObjectRef,		// Bond topology property + revision number
		VersionedDataObjectRef,		// Bond PBC vector property + revision number
		VersionedDataObjectRef,		// Particle position property + revision number
		VersionedDataObjectRef,		// Simulation cell + revision number
		FloatType					// Bond width
	> _boundingBoxCacheHelper;
};

/**
 * \brief This information record is attached to the bonds by the BondsVis when rendering
 * them in the viewports. It facilitates the picking of bonds with the mouse.
 */
class OVITO_PARTICLES_EXPORT BondPickInfo : public ObjectPickInfo
{
	Q_OBJECT
	OVITO_CLASS(BondPickInfo)

public:

	/// Constructor.
	BondPickInfo(const PipelineFlowState& pipelineState) : _pipelineState(pipelineState) {}

	/// The pipeline flow state containing the bonds.
	const PipelineFlowState& pipelineState() const { return _pipelineState; }

	/// Returns a human-readable string describing the picked object, which will be displayed in the status bar by OVITO.
	virtual QString infoString(PipelineSceneNode* objectNode, quint32 subobjectId) override;

private:

	/// The pipeline flow state containing the bonds.
	PipelineFlowState _pipelineState;
};

}	// End of namespace
}	// End of namespace


