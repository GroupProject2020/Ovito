////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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

#include <ovito/core/Core.h>
#include <ovito/core/dataset/pipeline/StaticSource.h>
#include <ovito/core/utilities/concurrent/SharedFuture.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

IMPLEMENT_OVITO_CLASS(StaticSource);
DEFINE_REFERENCE_FIELD(StaticSource, dataCollection);
SET_PROPERTY_FIELD_LABEL(StaticSource, dataCollection, "Data");

/******************************************************************************
* Constructor.
******************************************************************************/
StaticSource::StaticSource(DataSet* dataset, DataCollection* data) : PipelineObject(dataset)
{
    setDataCollection(data);
}

/******************************************************************************
* Asks the object for the result of the data pipeline.
******************************************************************************/
SharedFuture<PipelineFlowState> StaticSource::evaluate(const PipelineEvaluationRequest& request)
{
    // Note that the PipelineFlowState constructor creates deep copy of the data collection.
    // We always pass a copy of the data to the pipeline to avoid unexpected side effects when
    // modifying the objects in this source's data collection.
    return Future<PipelineFlowState>::createImmediateEmplace(dataCollection(), PipelineStatus::Success);
}

/******************************************************************************
* Asks the object for the result of the data pipeline.
******************************************************************************/
PipelineFlowState StaticSource::evaluateSynchronous(TimePoint time)
{
    // Note that the PipelineFlowState constructor creates deep copy of the data collection.
    // We always pass a copy of the data to the pipeline to avoid unexpected side effects when
    // modifying the objects in this source's data collection.
    return PipelineFlowState(dataCollection(), PipelineStatus::Success);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
