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


#include <ovito/core/Core.h>
#include <ovito/core/oo/RefTarget.h>
#include <ovito/core/utilities/concurrent/Future.h>
#include "PipelineFlowState.h"
#include "PipelineStatus.h"
#include "ModifierClass.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * \brief Base class for algorithms that operate on a PipelineFlowState.
 *
 * \sa ModifierApplication
 */
class OVITO_CORE_EXPORT Modifier : public RefTarget
{
	OVITO_CLASS_META(Modifier, ModifierClass)
	Q_OBJECT

protected:

	/// \brief Constructor.
	Modifier(DataSet* dataset);

public:

	/// \brief Modifies the input data.
	/// \param time The animation at which the modifier is applied.
	/// \param modApp The application object for this modifier. It describes this particular usage of the
	///               modifier in the data pipeline.
	/// \param input The upstream data flowing down the pipeline.
	virtual Future<PipelineFlowState> evaluate(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input);

	/// \brief Modifies the input data in an immediate, preliminary way.
	virtual void evaluatePreliminary(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) {}

	/// \brief Asks the modifier for its validity interval at the given time.
	/// \param time The animation at which the validity interval should be computed.
	/// \return The maximum time interval that contains \a time and during which the modifier's
	///         parameters do not change. This does not include the validity interval of the
	///         modifier's input object.
	virtual TimeInterval modifierValidity(TimePoint time);

	/// \brief Lets the modifier render itself into a viewport.
	/// \param time The animation time at which to render the modifier.
	/// \param contextNode The node context used to render the modifier.
	/// \param modApp The modifier application specifies the particular application of this modifier in a geometry pipeline.
	/// \param renderer The scene renderer to use.
	/// \param renderOverlay Specifies the rendering pass. The method is called twice by the system: First with renderOverlay==false
	///                      to render any 3d representation of the modifier, and a second time with renderOverlay==true to render
	///                      any overlay graphics on top of the 3d scene.
	///
	/// The viewport transformation is already set up when this method is called
	/// The default implementation does nothing.
	virtual void renderModifierVisual(TimePoint time, PipelineSceneNode* contextNode, ModifierApplication* modApp, SceneRenderer* renderer, bool renderOverlay) {}

	/// \brief Returns the list of applications of this modifier in pipelines.
	/// \return The list of ModifierApplication objects that describe the particular applications of this Modifier.
	///
	/// One and the same modifier instance can be applied in several geometry pipelines.
	/// Each application of the modifier instance is associated with a instance of the ModifierApplication class.
	/// This method can be used to determine all applications of this Modifier instance.
	QVector<ModifierApplication*> modifierApplications() const;

	/// \brief Returns one of the applications of this modifier in a pipeline.
	ModifierApplication* someModifierApplication() const;

	/// \brief Create a new modifier application that refers to this modifier instance.
	OORef<ModifierApplication> createModifierApplication();

	/// \brief Returns the title of this modifier object.
	virtual QString objectTitle() const override {
		if(title().isEmpty()) return RefTarget::objectTitle();
		else return title();
	}

	/// \brief Changes the title of this modifier.
	/// \undoable
	void setObjectTitle(const QString& title) { setTitle(title); }

	/// \brief Returns the the current status of the modifier's applications.
	PipelineStatus globalStatus() const;

	/// \brief This method is called by the system when the modifier has been inserted into a data pipeline.
	/// \param modApp The ModifierApplication object that has been created for this modifier.
	virtual void initializeModifier(ModifierApplication* modApp) {}

	/// \brief Decides whether a preliminary viewport update is performed after the modifier has been
	///        evaluated but before the entire pipeline evaluation is complete.
	virtual bool performPreliminaryUpdateAfterEvaluation() { return true; }

	/// \brief Decides whether a preliminary viewport update is performed every time the modifier
	///        iself changes. This makes mostly sense for synchronous modifiers.
	virtual bool performPreliminaryUpdateAfterChange() { return true; }

	/// \brief Returns the number of animation frames this modifier can provide.
	virtual int numberOfSourceFrames(int inputFrames) const { return inputFrames; }

	/// \brief Given an animation time, computes the source frame to show.
	virtual int animationTimeToSourceFrame(TimePoint time, int inputFrame) const { return inputFrame; }

	/// \brief Given a source frame index, returns the animation time at which it is shown.
	virtual TimePoint sourceFrameToAnimationTime(int frame, TimePoint inputTime) const { return inputTime; }

	/// \brief Returns the human-readable labels associated with the animation frames (e.g. the simulation timestep numbers).
	virtual QMap<int, QString> animationFrameLabels(QMap<int, QString> inputLabels) const { return std::move(inputLabels); }

private:

	/// Flag that indicates whether the modifier is enabled.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, isEnabled, setEnabled);

	/// The user-defined title of this modifier.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, title, setTitle);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::Modifier*);
Q_DECLARE_TYPEINFO(Ovito::Modifier*, Q_PRIMITIVE_TYPE);
