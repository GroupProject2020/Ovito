///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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
SharedFuture<PipelineFlowState> StaticSource::evaluate(TimePoint time, bool breakOnError)
{
    // Note that the PipelineFlowState constructor creates deep copy of the data collection.
    // We always pass a copy of the data to the pipeline to avoid unexpected side effects when
    // modifying the objects in this source's data collection.
    return Future<PipelineFlowState>::createImmediateEmplace(dataCollection(), PipelineStatus::Success);
}

/******************************************************************************
* Asks the object for the result of the data pipeline.
******************************************************************************/
PipelineFlowState StaticSource::evaluatePreliminary()
{
    // Note that the PipelineFlowState constructor creates deep copy of the data collection.
    // We always pass a copy of the data to the pipeline to avoid unexpected side effects when
    // modifying the objects in this source's data collection.
    return PipelineFlowState(dataCollection(), PipelineStatus::Success);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
