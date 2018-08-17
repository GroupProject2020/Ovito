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


#include <plugins/stdobj/StdObj.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/data/DataObject.h>
#include <core/dataset/pipeline/PipelineFlowState.h>
#include <core/oo/CloneHelper.h>

#include <boost/optional.hpp>

namespace Ovito { namespace StdObj {

/**
 * \brief Helper class that allows easy manipulation of properties.
 */
class OVITO_STDOBJ_EXPORT OutputHelper
{
public:

	/// Constructor.
	OutputHelper(DataSet* dataset, PipelineFlowState& output) : _dataset(dataset), _output(output) {}

	/// Creates a new data object of the desired type in the output flow state.
	/// If an object of the given type already exists, it is made sure that it is 
	/// returned after making sure it is exclusively owned by the flow state and safe to modify.
	template<class ObjectType>
	ObjectType* outputObject() {
		if(ObjectType* obj = output().findObject<ObjectType>()) {
			return cloneIfNeeded(obj);
		}
		else {
			OORef<ObjectType> newObj = new ObjectType(dataset());
			output().addObject(newObj);
			return newObj;
		}
	}

	/// Creates a standard property in the modifier's output.
	/// If the property already exists in the input, its contents are copied to the
	/// output property by this method.
	PropertyObject* outputStandardProperty(const PropertyClass& propertyClass, int typeId, bool initializeMemory = false);

	/// Creates a standard property in the modifier's output.
	/// If the property already exists in the input, its contents are copied to the
	/// output property by this method.
	template<class PropertyObjectType>
	PropertyObjectType* outputStandardProperty(int typeId, bool initializeMemory = false) {
		return static_object_cast<PropertyObjectType>(outputStandardProperty(PropertyObjectType::OOClass(), typeId, initializeMemory));
	}

	/// Creates a custom property in the modifier's output.
	/// If the property already exists in the input, its contents are copied to the
	/// output property by this method.
	PropertyObject* outputCustomProperty(const PropertyClass& propertyClass, const QString& name, int dataType, size_t componentCount, size_t stride, bool initializeMemory = false);

	/// Creates a custom property in the modifier's output.
	/// If the property already exists in the input, its contents are copied to the
	/// output property by this method.
	template<class PropertyObjectType>
	PropertyObjectType* outputCustomProperty(const QString& name, int dataType, size_t componentCount, size_t stride, bool initializeMemory = false) {
		return static_object_cast<PropertyObjectType>(outputCustomProperty(PropertyObjectType::OOClass(), name, dataType, componentCount, stride, initializeMemory));
	}
	
	/// Creates a property in the modifier's output and sets its content.
	PropertyObject* outputProperty(const PropertyClass& propertyClass, const PropertyPtr& storage);

	/// Creates a property in the modifier's output and sets its content.
	template<class PropertyObjectType>
	PropertyObjectType* outputProperty(const PropertyPtr& storage) {
		return static_object_cast<PropertyObjectType>(outputProperty(PropertyObjectType::OOClass(), storage));
	}

	/// Emits a new global attribute to the pipeline.
	void outputAttribute(const QString& key, QVariant value);

	/// Returns a unique identifier for a new data series object that does not collide with the 
	/// identifier of an existing data series in the same data collection.
	QString generateUniqueSeriesIdentifier(const QString& baseName) const;

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

	/// The context data set.
	DataSet* _dataset;
	
	/// The clone helper object that is used to create shallow and deep copies
	/// of the data objects.
	boost::optional<CloneHelper> _cloneHelper;

	/// The output state.
	PipelineFlowState& _output;
};

}	// End of namespace
}	// End of namespace
