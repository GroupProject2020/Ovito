////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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
#include <ovito/core/dataset/pipeline/ActiveObject.h>
#include <ovito/core/dataset/animation/TimeInterval.h>

namespace Ovito {

/**
 * \brief Abstract base class for display objects that are responsible
 *        for rendering DataObject-derived classes.
 */
class OVITO_CORE_EXPORT DataVis : public ActiveObject
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
	/// \param objectStack The data object to be rendered and its parent objects.
	/// \param flowState The pipeline evaluation results of the object node.
	/// \param renderer The renderer that should be used to produce the visualization.
	/// \param contextNode The pipeline node that is being rendered.
	///
	/// The world transformation matrix is already set up when this method is called by the
	/// system. The data has to be rendered in the local object coordinate system.
	virtual void render(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode) = 0;

	/// \brief Computes the view-independent bounding box of the given data object.
	/// \param time The animation time for which the bounding box should be computed.
	/// \param objectStack The data object for which to compute the bounding box and all its parent objects.
	/// \param contextNode The pipeline node which this vis element belongs to.
	/// \param flowState The pipeline evaluation result of the object node.
	/// \param validityInterval The time interval to be reduced by the method to report the duration of validity of the computed box.
	/// \return The bounding box of the visual element in local object coordinates.
	virtual Box3 boundingBox(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval) = 0;

	/// \brief Indicates whether this visual element should be surrounded by a selection marker in the viewports when it is selected.
	/// \return \c true to let the system render a selection marker around the object when it is selected.
	///
	/// The default implementation returns \c true.
	virtual bool showSelectionMarker() { return true; }

	/// \brief Returns all pipeline nodes whose pipeline produced this visualization element.
	/// \param onlyScenePipelines If true, pipelines which are currently not part of the scene are ignored.
	QSet<PipelineSceneNode*> pipelines(bool onlyScenePipelines) const;
};

}	// End of namespace
