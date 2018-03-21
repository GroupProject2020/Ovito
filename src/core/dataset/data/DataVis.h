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


#include <core/Core.h>
#include <core/oo/RefTarget.h>
#include <core/dataset/animation/TimeInterval.h>
#include <core/dataset/pipeline/PipelineStatus.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * \brief Abstract base class for display objects that are responsible
 *        for rendering DataObject-derived classes.
 */
class OVITO_CORE_EXPORT DataVis : public RefTarget
{
	Q_OBJECT
	OVITO_CLASS(DataVis)
	
protected:

	/// \brief Constructor.
	DataVis(DataSet* dataset);

public:

	/// \brief Lets the vis element render a data object.
	///
	/// \param time The animation time at which to render the object
	/// \param dataObject The data object that should be rendered.
	/// \param flowState The pipeline evaluation results of the object node.
	/// \param renderer The renderer that should be used to produce the visualization.
	/// \param contextNode The pipeline node that is being rendered.
	///
	/// The world transformation matrix is already set up when this method is called by the
	/// system. The data has to be rendered in the local object coordinate system.
	virtual void render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, PipelineSceneNode* contextNode) = 0;

	/// \brief Computes the view-independent bounding box of the given data object.
	/// \param time The animation time for which the bounding box should be computed.
	/// \param dataObject The data object for which to compute the bounding box.
	/// \param contextNode The pipeline node which this vis element belongs to.
	/// \param flowState The pipeline evaluation result of the object node.
	/// \param validityInterval The time interval to be reduced by the method to report the duration of validity of the computed box.
	/// \return The bounding box of the visual element in local object coordinates.
	virtual Box3 boundingBox(TimePoint time, DataObject* dataObject, PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval) = 0;

	/// \brief Indicates whether this visual element should be surrounded by a selection marker in the viewports when it is selected.
	/// \return \c true to let the system render a selection marker around the object when it is selected.
	///
	/// The default implementation returns \c true.
	virtual bool showSelectionMarker() { return true; }

	/// \brief Returns a structure that describes the current status of the vis element.
	virtual PipelineStatus status() const { return _status; }

	/// \brief Sets the current status of the vis element.
	void setStatus(const PipelineStatus& status);

	/// \brief Returns the title of this object.
	virtual QString objectTitle() override {
		if(title().isEmpty()) return RefTarget::objectTitle();
		else return title();
	}

	/// \brief Changes the title of this object.
	/// \undoable
	void setObjectTitle(const QString& title) { setTitle(title); }

	/// \brief Returns all pipeline nodes whose pipeline produced this visualization element.
	/// \param onlyScenePipelines If true, pipelines which are currently not part of the scene are ignored.
	QSet<PipelineSceneNode*> pipelines(bool onlyScenePipelines) const;

private:

	/// Controls whether the visualization element is enabled.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, isEnabled, setEnabled);

	/// The title of this visualization element.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, title, setTitle);

	/// The current status of this visualization element.
	PipelineStatus _status;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
