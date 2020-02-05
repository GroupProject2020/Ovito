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
#include <ovito/core/dataset/data/DataVis.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * \brief A type of DataVis that first transforms data into another form before
 *        rendering it. The transformation process typically happens asynchronously.
 */
class OVITO_CORE_EXPORT TransformingDataVis : public DataVis
{
	Q_OBJECT
	OVITO_CLASS(TransformingDataVis)

protected:

	/// \brief Constructor.
	TransformingDataVis(DataSet* dataset);

public:

	/// \brief Determines the time interval over which a computed pipeline state will remain valid.
	virtual TimeInterval validityInterval(const PipelineEvaluationRequest& request, PipelineSceneNode* pipeline) const { return TimeInterval::infinite(); }

	/// Lets the vis element transform a data object in preparation for rendering.
	Future<PipelineFlowState> transformData(const PipelineEvaluationRequest& request, const DataObject* dataObject, PipelineFlowState&& flowState, const std::vector<OORef<TransformedDataObject>>& cachedTransformedDataObjects);

	/// Returns the revision counter of this vis element, which is incremented each time one of its parameters changes.
	unsigned int revisionNumber() const { return _revisionNumber; }

	/// Bumps up the internal revision number of this DataVis in order to mark
	/// all transformed data objects as outdated which have been generated so far.
	void invalidateTransformedObjects() {
		_revisionNumber++;
	}

protected:

	/// Lets the vis element transform a data object in preparation for rendering.
	virtual Future<PipelineFlowState> transformDataImpl(const PipelineEvaluationRequest& request, const DataObject* dataObject, PipelineFlowState&& flowState) = 0;

private:

	/// The number of data transformations that are currently in progress.
	int _activeTransformationsCount = 0;

	/// The revision counter of this element.
	/// The counter is incremented every time one of the object's parameters changes that
	/// trigger a regeneration of the transformed data object from the input data.
	unsigned int _revisionNumber = 0;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
