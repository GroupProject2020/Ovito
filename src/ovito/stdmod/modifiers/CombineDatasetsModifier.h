///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2019) Alexander Stukowski
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


#include <ovito/stdmod/StdMod.h>
#include <ovito/core/dataset/pipeline/DelegatingModifier.h>
#include <ovito/core/dataset/pipeline/PipelineObject.h>

namespace Ovito { namespace StdMod {

/**
 * \brief Base class for CombineDatasetsModifier delegates that operate on different kinds of data.
 */
class OVITO_STDMOD_EXPORT CombineDatasetsModifierDelegate : public ModifierDelegate
{
	Q_OBJECT
	OVITO_CLASS(CombineDatasetsModifierDelegate)

protected:

	/// Abstract class constructor.
	using ModifierDelegate::ModifierDelegate;
};

/**
 * \brief Merges two separate datasets into one.
 */
class OVITO_STDMOD_EXPORT CombineDatasetsModifier : public MultiDelegatingModifier
{
	/// Give this modifier class its own metaclass.
	class CombineDatasetsModifierClass : public MultiDelegatingModifier::OOMetaClass
	{
	public:

		/// Inherit constructor from base metaclass.
		using MultiDelegatingModifier::OOMetaClass::OOMetaClass;

		/// Return the metaclass of delegates for this modifier type.
		virtual const ModifierDelegate::OOMetaClass& delegateMetaclass() const override { return CombineDatasetsModifierDelegate::OOClass(); }
	};

	Q_OBJECT
	OVITO_CLASS_META(CombineDatasetsModifier, CombineDatasetsModifierClass)

	Q_CLASSINFO("DisplayName", "Combine datasets");
	Q_CLASSINFO("ModifierCategory", "Modification");

public:

	/// Constructor.
	Q_INVOKABLE CombineDatasetsModifier(DataSet* dataset);

	/// Modifies the input data in an immediate, preliminary way.
	virtual void evaluatePreliminary(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

	/// Modifies the input data.
	virtual Future<PipelineFlowState> evaluate(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

	/// Returns the number of animation frames this modifier can provide.
	virtual int numberOfSourceFrames(int inputFrames) const override {
		return secondaryDataSource() ? std::max(secondaryDataSource()->numberOfSourceFrames(), inputFrames) : inputFrames;
	}

	/// Given an animation time, computes the source frame to show.
	virtual int animationTimeToSourceFrame(TimePoint time, int inputFrame) const override {
		return inputFrame;
	}

	/// Given a source frame index, returns the animation time at which it is shown.
	virtual TimePoint sourceFrameToAnimationTime(int frame, TimePoint inputTime) const override {
		return inputTime;
	}

	/// Returns the human-readable labels associated with the animation frames (e.g. the simulation timestep numbers).
	virtual QMap<int, QString> animationFrameLabels(QMap<int, QString> inputLabels) const override {
		if(secondaryDataSource())
			inputLabels.unite(secondaryDataSource()->animationFrameLabels());
		return std::move(inputLabels);
	}

protected:

	/// \brief Is called when a RefTarget referenced by this object has generated an event.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

	/// Is called when the value of a reference field of this object changes.
	virtual void referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget) override;

private:

	/// The source for particle data to be merged into the pipeline.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(PipelineObject, secondaryDataSource, setSecondaryDataSource, PROPERTY_FIELD_NO_SUB_ANIM);
};

}	// End of namespace
}	// End of namespace
