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


#include <ovito/core/Core.h>
#include <ovito/core/dataset/data/DataCollection.h>
#include "PipelineObject.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * \brief This is a PipelineObject that returns a static data collection.
 */
class OVITO_CORE_EXPORT StaticSource : public PipelineObject
{
	Q_OBJECT
	OVITO_CLASS(StaticSource)
	Q_CLASSINFO("DisplayName", "Pipeline source");

public:

	/// \brief Standard constructor.
	Q_INVOKABLE StaticSource(DataSet* dataset, DataCollection* data = nullptr);

	/// \brief Asks the object for the result of the data pipeline.
	virtual SharedFuture<PipelineFlowState> evaluate(TimePoint time, bool breakOnError = false) override;

	/// \brief Returns the results of an immediate and preliminary evaluation of the data pipeline.
	virtual PipelineFlowState evaluatePreliminary() override;

	/// Returns the list of data objects that are managed by this data source.
	/// The returned data objects will be displayed as sub-objects of the data source in the pipeline editor.
	virtual DataCollection* getSourceDataCollection() const override { return dataCollection(); }

private:

	/// The data collection owned by this source.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(DataCollection, dataCollection, setDataCollection, PROPERTY_FIELD_ALWAYS_DEEP_COPY);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


