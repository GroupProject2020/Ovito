///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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


#include <plugins/stdmod/StdMod.h>
#include <core/dataset/animation/controller/Controller.h>
#include <core/dataset/pipeline/DelegatingModifier.h>

namespace Ovito { namespace StdMod {

/**
 * \brief Base class for SliceModifier delegates that operate on different kinds of data.
 */
class OVITO_STDMOD_EXPORT SliceModifierDelegate : public ModifierDelegate
{
	Q_OBJECT
	OVITO_CLASS(SliceModifierDelegate)

protected:

	/// Abstract class constructor.
	SliceModifierDelegate(DataSet* dataset) : ModifierDelegate(dataset) {}
};
	
/**
 * \brief The slice modifier performs a cut through a dataset.
 */
class OVITO_STDMOD_EXPORT SliceModifier : public MultiDelegatingModifier
{
	/// Give this modifier class its own metaclass.
	class SliceModifierClass : public MultiDelegatingModifier::OOMetaClass 
	{
	public:

		/// Inherit constructor from base class.
		using MultiDelegatingModifier::OOMetaClass::OOMetaClass;

		/// Return the metaclass of delegates for this modifier type. 
		virtual const ModifierDelegate::OOMetaClass& delegateMetaclass() const override { return SliceModifierDelegate::OOClass(); }
	};

	OVITO_CLASS_META(SliceModifier, SliceModifierClass)
	Q_OBJECT

	Q_CLASSINFO("DisplayName", "Slice");
	Q_CLASSINFO("ModifierCategory", "Modification");

public:

	/// Constructor.
	Q_INVOKABLE SliceModifier(DataSet* dataset);

	/// Asks the modifier for its validity interval at the given time.
	virtual TimeInterval modifierValidity(TimePoint time) override;

	/// Lets the modifier render itself into the viewport.
	virtual void renderModifierVisual(TimePoint time, ObjectNode* contextNode, ModifierApplication* modApp, SceneRenderer* renderer, bool renderOverlay) override;

	// Property access functions:

	/// Returns the plane's distance from the origin.
	FloatType distance() const { return distanceController() ? distanceController()->currentFloatValue() : 0; }

	/// Sets the plane's distance from the origin.
	void setDistance(FloatType newDistance) { if(distanceController()) distanceController()->setCurrentFloatValue(newDistance); }

	/// Returns the plane's normal vector.
	Vector3 normal() const { return normalController() ? normalController()->currentVector3Value() : Vector3(0,0,1); }

	/// Sets the plane's distance from the origin.
	void setNormal(const Vector3& newNormal) { if(normalController()) normalController()->setCurrentVector3Value(newNormal); }

	/// Returns the slice width.
	FloatType sliceWidth() const { return widthController() ? widthController()->currentFloatValue() : 0; }

	/// Sets the slice width.
	void setSliceWidth(FloatType newWidth) { if(widthController()) widthController()->setCurrentFloatValue(newWidth); }

	/// Returns the slicing plane and slice width.
	std::tuple<Plane3, FloatType> slicingPlane(TimePoint time, TimeInterval& validityInterval);

protected:

	/// This method is called by the system when the modifier has been inserted into a data pipeline.
	virtual void initializeModifier(ModifierApplication* modApp) override;

	/// \brief Renders the modifier's visual representation and computes its bounding box.
	void renderVisual(TimePoint time, ObjectNode* contextNode, SceneRenderer* renderer);

	/// Renders the plane in the viewport.
	void renderPlane(SceneRenderer* renderer, const Plane3& plane, const Box3& box, const ColorA& color) const;

	/// Computes the intersection lines of a plane and a quad.
	void planeQuadIntersection(const Point3 corners[8], const std::array<int,4>& quadVerts, const Plane3& plane, QVector<Point3>& vertices) const;

	/// This controller stores the normal of the slicing plane.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(Controller, normalController, setNormalController);

	/// This controller stores the distance of the slicing plane from the origin.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(Controller, distanceController, setDistanceController);

	/// Controls the slice width.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(Controller, widthController, setWidthController);

	/// Controls whether the data elements should only be selected instead of being deleted.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, createSelection, setCreateSelection);

	/// Controls whether the plane's orientation should be reversed.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, inverse, setInverse);

	/// Controls whether the modifier should only be applied to the currently selected data elements.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, applyToSelection, setApplyToSelection);
};

}	// End of namespace
}	// End of namespace
