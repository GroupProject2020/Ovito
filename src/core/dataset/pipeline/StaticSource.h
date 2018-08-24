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
#include <core/dataset/data/DataObject.h>
#include "PipelineObject.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * \brief This is a PipelineObject that returns a static DataObject.
 */
class OVITO_CORE_EXPORT StaticSource : public PipelineObject
{
	Q_OBJECT
	OVITO_CLASS(StaticSource)
	Q_CLASSINFO("DisplayName", "Pipeline source");

public:

	/// \brief Standard constructor.
	Q_INVOKABLE StaticSource(DataSet* dataset);

	/// \brief Constructor that inserts a single data object into the new source.
	StaticSource(DataSet* dataset, DataObject* dataObject) : StaticSource(dataset) {
		OVITO_ASSERT(dataObject);
		_dataObjects.push_back(this, PROPERTY_FIELD(dataObjects), dataObject);
	}

	/// \brief Constructor that inserts the content of a PipelineFlowState into the new source.
	StaticSource(DataSet* dataset, const PipelineFlowState& state) : StaticSource(dataset) {
		for(DataObject* obj : state.objects())
			_dataObjects.push_back(this, PROPERTY_FIELD(dataObjects), obj);
	}
	
	/// \brief Asks the object for the result of the data pipeline.
	virtual SharedFuture<PipelineFlowState> evaluate(TimePoint time) override;

	/// \brief Returns the results of an immediate and preliminary evaluation of the data pipeline.
	virtual PipelineFlowState evaluatePreliminary() override;

	/// \brief Adds an additional data object to this source.
	void addDataObject(DataObject* obj) { OVITO_ASSERT(!dataObjects().contains(obj)); _dataObjects.push_back(this, PROPERTY_FIELD(dataObjects), obj); }

	/// \brief Inserts an additional data object into this source.
	void insertDataObject(int index, DataObject* obj) { OVITO_ASSERT(!dataObjects().contains(obj)); _dataObjects.insert(this, PROPERTY_FIELD(dataObjects), index, obj); }
	
	/// \brief Removes a data object from this source.
	void removeDataObject(int index) { _dataObjects.remove(this, PROPERTY_FIELD(dataObjects), index); }
	
	/// \brief Finds an object of the given type in the list of data objects stored in this source.
	template<class ObjectType>
	ObjectType* findObject() const {
		for(DataObject* o : dataObjects()) {
			if(ObjectType* obj = dynamic_object_cast<ObjectType>(o))
				return obj;
		}
		return nullptr;
	}
	
private:

	/// The list of data objects owned by this source.
	DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD_FLAGS(DataObject, dataObjects, setDataObjects, PROPERTY_FIELD_ALWAYS_DEEP_COPY);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


