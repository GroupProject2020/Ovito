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
#include <core/dataset/pipeline/DelegatingModifier.h>
#include <core/dataset/data/properties/PropertyReference.h>
#include <core/dataset/animation/controller/Controller.h>
#include <core/dataset/animation/AnimationSettings.h>
#include "ColormapsData.h"

namespace Ovito { namespace StdMod {

/**
 * \brief Abstract base class for color gradients that can be used with a ColorCodingModifier.
 *
 * Implementations of this class convert a scalar value in the range [0,1] to a color value.
 */
class OVITO_STDMOD_EXPORT ColorCodingGradient : public RefTarget
{
	Q_OBJECT
	OVITO_CLASS(ColorCodingGradient)
	
protected:

	/// Constructor.
	using RefTarget::RefTarget;

public:

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) = 0;
};

/**
 * \brief Converts a scalar value to a color using the HSV color system.
 */
class OVITO_STDMOD_EXPORT ColorCodingHSVGradient : public ColorCodingGradient
{
	OVITO_CLASS(ColorCodingHSVGradient)
	Q_CLASSINFO("DisplayName", "Rainbow");
	Q_OBJECT
	
public:

	/// Constructor.
	Q_INVOKABLE ColorCodingHSVGradient(DataSet* dataset) : ColorCodingGradient(dataset) {}

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) override { return Color::fromHSV((FloatType(1) - t) * FloatType(0.7), 1, 1); }
};

/**
 * \brief Converts a scalar value to a color using a gray-scale ramp.
 */
class OVITO_STDMOD_EXPORT ColorCodingGrayscaleGradient : public ColorCodingGradient
{
	OVITO_CLASS(ColorCodingGrayscaleGradient)
	Q_CLASSINFO("DisplayName", "Grayscale");
	Q_OBJECT
	
public:

	/// Constructor.
	Q_INVOKABLE ColorCodingGrayscaleGradient(DataSet* dataset) : ColorCodingGradient(dataset) {}

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) override { return Color(t, t, t); }
};

/**
 * \brief Converts a scalar value to a color.
 */
class OVITO_STDMOD_EXPORT ColorCodingHotGradient : public ColorCodingGradient
{
	OVITO_CLASS(ColorCodingHotGradient)
	Q_CLASSINFO("DisplayName", "Hot");
	Q_OBJECT
	
public:

	/// Constructor.
	Q_INVOKABLE ColorCodingHotGradient(DataSet* dataset) : ColorCodingGradient(dataset) {}

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) override {
		// Interpolation black->red->yellow->white.
		OVITO_ASSERT(t >= 0 && t <= 1);
		return Color(std::min(t / FloatType(0.375), FloatType(1)), std::max(FloatType(0), std::min((t-FloatType(0.375))/FloatType(0.375), FloatType(1))), std::max(FloatType(0), t*4 - FloatType(3)));
	}
};

/**
 * \brief Converts a scalar value to a color.
 */
class OVITO_STDMOD_EXPORT ColorCodingJetGradient : public ColorCodingGradient
{
	OVITO_CLASS(ColorCodingJetGradient)
	Q_CLASSINFO("DisplayName", "Jet");
	Q_OBJECT
	
public:

	/// Constructor.
	Q_INVOKABLE ColorCodingJetGradient(DataSet* dataset) : ColorCodingGradient(dataset) {}

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) override {
	    if(t < FloatType(0.125)) return Color(0, 0, FloatType(0.5) + FloatType(0.5) * t / FloatType(0.125));
	    else if(t < FloatType(0.125) + FloatType(0.25)) return Color(0, (t - FloatType(0.125)) / FloatType(0.25), 1);
	    else if(t < FloatType(0.125) + FloatType(0.25) + FloatType(0.25)) return Color((t - FloatType(0.375)) / FloatType(0.25), 1, FloatType(1) - (t - FloatType(0.375)) / FloatType(0.25));
	    else if(t < FloatType(0.125) + FloatType(0.25) + FloatType(0.25) + FloatType(0.25)) return Color(1, FloatType(1) - (t - FloatType(0.625)) / FloatType(0.25), 0);
	    else return Color(FloatType(1) - FloatType(0.5) * (t - FloatType(0.875)) / FloatType(0.125), 0, 0);
	}
};

/**
 * \brief Converts a scalar value to a color.
 */
class OVITO_STDMOD_EXPORT ColorCodingBlueWhiteRedGradient : public ColorCodingGradient
{
	OVITO_CLASS(ColorCodingBlueWhiteRedGradient)
	Q_CLASSINFO("DisplayName", "Blue-White-Red");
	Q_OBJECT
	
public:

	/// Constructor.
	Q_INVOKABLE ColorCodingBlueWhiteRedGradient(DataSet* dataset) : ColorCodingGradient(dataset) {}

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) override {
		if(t <= FloatType(0.5))
			return Color(t * 2, t * 2, 1);
		else
			return Color(1, (FloatType(1)-t) * 2, (FloatType(1)-t) * 2);
	}
};

/**
 * \brief Converts a scalar value to a color.
 */
class OVITO_STDMOD_EXPORT ColorCodingViridisGradient : public ColorCodingGradient
{
	OVITO_CLASS(ColorCodingViridisGradient)
	Q_CLASSINFO("DisplayName", "Viridis");
	Q_OBJECT
	
public:

	/// Constructor.
	Q_INVOKABLE ColorCodingViridisGradient(DataSet* dataset) : ColorCodingGradient(dataset) {}

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) override {
		int index = t * (sizeof(colormap_viridis_data)/sizeof(colormap_viridis_data[0]) - 1);
		return Color(colormap_viridis_data[index][0], colormap_viridis_data[index][1], colormap_viridis_data[index][2]);
	}
};

/**
 * \brief Converts a scalar value to a color.
 */
class OVITO_STDMOD_EXPORT ColorCodingMagmaGradient : public ColorCodingGradient
{
	OVITO_CLASS(ColorCodingMagmaGradient)
	Q_CLASSINFO("DisplayName", "Magma");
	Q_OBJECT
	
public:

	/// Constructor.
	Q_INVOKABLE ColorCodingMagmaGradient(DataSet* dataset) : ColorCodingGradient(dataset) {}

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) override {
		int index = t * (sizeof(colormap_magma_data)/sizeof(colormap_magma_data[0]) - 1);
		return Color(colormap_magma_data[index][0], colormap_magma_data[index][1], colormap_magma_data[index][2]);
	}
};

/**
 * \brief Converts a scalar value to a color based on a user-defined image.
 */
class OVITO_STDMOD_EXPORT ColorCodingImageGradient : public ColorCodingGradient
{
	OVITO_CLASS(ColorCodingImageGradient)
	Q_CLASSINFO("DisplayName", "User image");
	Q_OBJECT
	
public:

	/// Constructor.
	Q_INVOKABLE ColorCodingImageGradient(DataSet* dataset) : ColorCodingGradient(dataset) {}

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) override;

	/// Loads the given image file from disk.
	void loadImage(const QString& filename);

private:

	/// The user-defined color map image.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QImage, image, setImage);
};

/**
 * \brief Base class for ColorCodingModifier delegates that operate on different kinds of data.
 */
class OVITO_STDMOD_EXPORT ColorCodingModifierDelegate : public ModifierDelegate
{
	OVITO_CLASS(ColorCodingModifierDelegate)
	Q_OBJECT
	
public:
	
	/// \brief Applies the modifier operation to the data in a pipeline flow state.
	virtual PipelineStatus apply(Modifier* modifier, const PipelineFlowState& input, PipelineFlowState& output, TimePoint time, ModifierApplication* modApp) override;

	/// \brief Returns the class of properties that can serve as input for the color coding.
	virtual const PropertyClass& propertyClass() const = 0;
	
protected:

	/// Abstract class constructor.
	using ModifierDelegate::ModifierDelegate;
	
	/// \brief Creates the property object that will receive the computed colors.
	virtual PropertyObject* createOutputColorProperty(TimePoint time, InputHelper& ih, OutputHelper& oh, bool initializeWithExistingColors) = 0;
};

/**
 * \brief This modifier assigns a colors to data elements based on the value of a property.
 */
class OVITO_STDMOD_EXPORT ColorCodingModifier : public DelegatingModifier
{
public:

	/// Give this modifier class its own metaclass.
	class ColorCodingModifierClass : public DelegatingModifier::OOMetaClass 
	{
	public:

		/// Inherit constructor from base class.
		using DelegatingModifier::OOMetaClass::OOMetaClass;

		/// Return the metaclass of delegates for this modifier type. 
		virtual const ModifierDelegate::OOMetaClass& delegateMetaclass() const override { return ColorCodingModifierDelegate::OOClass(); }
	};

	OVITO_CLASS_META(ColorCodingModifier, ColorCodingModifierClass)
	Q_CLASSINFO("DisplayName", "Color coding");
	Q_CLASSINFO("ModifierCategory", "Coloring");
	Q_OBJECT
	
public:

	/// Constructor.
	Q_INVOKABLE ColorCodingModifier(DataSet* dataset);

	/// Loads the user-defined default values of this object's parameter fields from the
	/// application's settings store.
	virtual void loadUserDefaults() override;

	/// Asks the modifier for its validity interval at the given time.
	virtual TimeInterval modifierValidity(TimePoint time) override;

	/// Returns the range start value.
	FloatType startValue() const { return startValueController() ? startValueController()->currentFloatValue() : 0; }

	/// Sets the range start value.
	void setStartValue(FloatType value) { if(startValueController()) startValueController()->setCurrentFloatValue(value); }

	/// Returns the range end value.
	FloatType endValue() const { return endValueController() ? endValueController()->currentFloatValue() : 0; }

	/// Sets the range end value.
	void setEndValue(FloatType value) { if(endValueController()) endValueController()->setCurrentFloatValue(value); }

	/// Sets the start and end value to the minimum and maximum value of the selected input property
	/// determined over the entire animation sequence.
	bool adjustRangeGlobal(TaskManager& taskManager);

public Q_SLOTS:

	/// Sets the start and end value to the minimum and maximum value of the selected input property.
	/// Returns true if successful.
	bool adjustRange();

protected:

	/// This method is called by the system after the modifier has been inserted into a data pipeline.
	virtual void initializeModifier(ModifierApplication* modApp) override;

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// Determines the range of values in the input data for the selected property.
	bool determinePropertyValueRange(const PipelineFlowState& state, FloatType& min, FloatType& max);

	/// Is called when the value of a reference field of this RefMaker changes.
	virtual void referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget) override;
	
private:

	/// This controller stores the start value of the color scale.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(Controller, startValueController, setStartValueController);

	/// This controller stores the end value of the color scale.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(Controller, endValueController, setEndValueController);

	/// This object converts property values to colors.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(ColorCodingGradient, colorGradient, setColorGradient);

	/// The input property that is used as data source for the coloring.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(PropertyReference, sourceProperty, setSourceProperty);

	/// Controls whether the modifier assigns a color only to selected elements.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, colorOnlySelected, setColorOnlySelected);

	/// Controls whether the input selection is preserved. If false, the selection is cleared by the modifier.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, keepSelection, setKeepSelection);
};

}	// End of namespace
}	// End of namespace
