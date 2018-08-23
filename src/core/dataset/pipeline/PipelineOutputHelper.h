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

#pragma once


#include <core/Core.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/data/DataObject.h>
#include <core/dataset/pipeline/PipelineFlowState.h>
#include <core/oo/CloneHelper.h>

#include <boost/optional.hpp>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * \brief Utility class for assembling an output PipelineFlowState.
 */
class OVITO_CORE_EXPORT PipelineOutputHelper
{
public:

	/// Constructor.
	PipelineOutputHelper(DataSet* dataset, PipelineFlowState& output, PipelineObject* dataSource) 
		: _dataset(dataset), _output(output), _dataSource(dataSource) {}

	/// Returns a new unique data object identifier that does not collide with the 
	/// identifiers of any existing data object of the given type in the same data 
	/// collection.
	QString generateUniqueIdentifier(const QString& baseName, const OvitoClass& dataObjectClass) const;

	/// Returns a new unique data object identifier that does not collide with the 
	/// identifiers of any existing data object of the given type in the same data 
	/// collection.
	template<class DataObjectClass>
	QString generateUniqueIdentifier(const QString& baseName) const {
		return generateUniqueIdentifier(baseName, DataObjectClass::OOClass());
	}

	/// Creates a new data object of the desired type in the output flow state.
	/// If an object of the given type already exists, it is made sure that it is 
	/// returned after making sure it is exclusively owned by the flow state and safe to modify.
	template<class DataObjectClass>
	DataObjectClass* outputSingletonObject() {
		if(DataObjectClass* obj = output().findObjectOfType<DataObjectClass>()) {
			return cloneIfNeeded(obj);
		}
		else {
			DataObjectClass* newObj = new DataObjectClass(dataset());
			outputObject(newObj);
			return newObj;
		}
	}

	/// Adds a new data object to the output state.
	void outputObject(DataObject* obj) {
		if(!obj->dataSource()) obj->setDataSource(_dataSource);
		output().addObject(obj);
	}

	/// Replaces an existing data object with a new one.
	bool replaceObject(DataObject* oldObj, DataObject* newObj) {
		if(!newObj->dataSource()) newObj->setDataSource(_dataSource);
		return output().replaceObject(oldObj, newObj);
	}

	/// Emits a new global attribute to the pipeline.
	void outputAttribute(const QString& key, QVariant value);
	
	/// Enures that a DataObject from this flow state is not shared with others and is safe to modify.
	template<class ObjectType>
	ObjectType* cloneIfNeeded(ObjectType* obj, bool deepCopy = false) {
		OVITO_ASSERT(output().contains(obj));
		OVITO_ASSERT(obj->numberOfStrongReferences() >= 1);
		if(obj->numberOfStrongReferences() > 1) {
			OORef<ObjectType> clone = cloneHelper().cloneObject(obj, deepCopy);
			if(output().replaceObject(obj, clone)) {
				OVITO_ASSERT(clone->numberOfStrongReferences() == 1);
				return clone;
			}
		}
		return obj;
	}

	/// Returns a reference to the output state.
	PipelineFlowState& output() { return _output; }

	/// Returns a const-reference to the output state.
	const PipelineFlowState& output() const { return _output; }

	/// Returns a clone helper for creating shallow and deep copies of data objects.
	CloneHelper& cloneHelper() {
		if(_cloneHelper == boost::none) _cloneHelper.emplace();
		return *_cloneHelper;
	}

	/// Returns the DataSet that provides a context for all performed operations.
	DataSet* dataset() const { return _dataset; }
	
private:

	/// The context dataset.
	DataSet* _dataset;
	
	/// The clone helper object that is used to create shallow and deep copies
	/// of the data objects.
	boost::optional<CloneHelper> _cloneHelper;

	/// The output state.
	PipelineFlowState& _output;

	/// The object creating the pipeline output.
	PipelineObject* _dataSource;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
