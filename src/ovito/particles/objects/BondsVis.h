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


#include <ovito/particles/Particles.h>
#include <ovito/stdobj/properties/PropertyObject.h>
#include <ovito/core/dataset/data/DataVis.h>
#include <ovito/core/rendering/ArrowPrimitive.h>
#include <ovito/core/rendering/SceneRenderer.h>

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

	/// Constructor.
	Q_INVOKABLE BondsVis(DataSet* dataset);

	/// Renders the visual element.
	virtual void render(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode) override;

	/// Computes the bounding box of the visual element.
	virtual Box3 boundingBox(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval) override;

	/// Returns the display color used for selected bonds.
	Color selectionBondColor() const { return Color(1,0,0); }

	/// Determines the display colors of half-bonds.
	/// Returns an array with two colors per full bond, because the two half-bonds may have different colors.
	std::vector<ColorA> halfBondColors(size_t particleCount, const PropertyObject* topologyProperty,
			const PropertyObject* bondColorProperty, const PropertyObject* bondTypeProperty, const PropertyObject* bondSelectionProperty, const PropertyObject* transparencyProperty,
			const ParticlesVis* particleVis, const PropertyObject* particleColorProperty, const PropertyObject* particleTypeProperty) const;

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
