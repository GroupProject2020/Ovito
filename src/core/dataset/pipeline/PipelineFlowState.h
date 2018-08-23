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
#include <core/dataset/animation/TimeInterval.h>
#include <core/oo/RefMaker.h>
#include <core/dataset/pipeline/PipelineStatus.h>
#include <core/dataset/data/DataObject.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * \brief This data structure holds the list of data objects that flows down a data pipeline.
 */
class OVITO_CORE_EXPORT PipelineFlowState
{
public:

	/// \brief Default constructor that creates an empty state object.
	PipelineFlowState() = default;

	/// \brief Constructor that creates a state object and initializes it with a DataObject.
	/// \param status A status object that describes the outcome of the pipeline evaluation.
	/// \param validityInterval Specifies the time interval during which the returned object is valid.
	///                         For times outside this interval the geometry pipeline has to be re-evaluated.
	PipelineFlowState(const PipelineStatus& status, const TimeInterval& validityInterval = TimeInterval::infinite()) : _status(status), _stateValidity(validityInterval) {}

	/// \brief Constructor that creates a state object and initializes it with a DataObject.
	/// \param dataObject The object represents the current state of a geometry pipeline evaluation.
	/// \param validityInterval Specifies the time interval during which the returned object is valid.
	///                         For times outside this interval the geometry pipeline has to be re-evaluated.
	PipelineFlowState(DataObject* dataObject, const TimeInterval& validityInterval = TimeInterval::infinite()) : _stateValidity(validityInterval) {
		addObject(dataObject);
	}

	/// \brief Constructor that creates a state object and initializes it with several DataObjects.
	/// \param dataObjects The objects to be inserted.
	/// \param validityInterval Specifies the time interval during which the state is valid.
	/// \param status A status object that describes the outcome of the pipeline evaluation.
	PipelineFlowState(const QVector<DataObject*>& dataObjects, const TimeInterval& validityInterval = TimeInterval::infinite(), const PipelineStatus& status = PipelineStatus()) : 
			_stateValidity(validityInterval),
			_status(status) 
	{
		_objects.reserve(dataObjects.size());
		for(DataObject* obj : dataObjects)
			addObject(obj);
	}

	/// \brief Discards all contents of this state object.
	void clear() {
		clearObjects();
		_stateValidity.setEmpty();
		_status = PipelineStatus();
	}

	/// \brief Discards the data objects in this state object.
	void clearObjects() {
		_objects.clear();
	}

	/// \brief Returns true if the given object is part of this pipeline flow state.
	/// \note The method ignores the revision number of the object.
	bool contains(DataObject* obj) const;

	/// \brief Adds an additional data object to this state.
	void addObject(DataObject* obj);

	/// \brief Inserts an additional data object into this state.
	void insertObject(int index, DataObject* obj);
		
	/// \brief Replaces a data object with a new one.
	bool replaceObject(DataObject* oldObj, DataObject* newObj);

	/// \brief Removes a data object from this state.
	void removeObject(DataObject* dataObj) { replaceObject(dataObj, nullptr); }

	/// \brief Removes a data object from this state.
	void removeObjectByIndex(int index);
		
	/// \brief Returns the list of data objects stored in this flow state.
	const std::vector<DataObject*>& objects() const { 
		OVITO_STATIC_ASSERT(sizeof(StrongDataObjectRef) == sizeof(DataObject*));
		OVITO_STATIC_ASSERT(sizeof(std::vector<StrongDataObjectRef>) == sizeof(std::vector<DataObject*>));
		return reinterpret_cast<const std::vector<DataObject*>&>(_objects);
	}

	/// \brief Finds an object of the given type in the list of data objects stored in this flow state.
	template<class ObjectType>
	ObjectType* findObjectOfType() const {
		for(DataObject* o : objects()) {
			if(ObjectType* obj = dynamic_object_cast<ObjectType>(o))
				return obj;
		}
		return nullptr;
	}

	/// \brief Finds an object of the given type and with the given identifier in the list of data objects stored in this flow state.
	template<class DataObjectClass>
	DataObjectClass* findObject(const QString& identifier, PipelineObject* dataSource = nullptr) const {
		return static_object_cast<DataObjectClass>(findObject(DataObjectClass::OOClass(), identifier, dataSource));
	}

	/// \brief Finds an object of the given type and with the given identifier in the list of data objects stored in this flow state.
	DataObject* findObject(const DataObject::OOMetaClass& objectClass, const QString& identifier, PipelineObject* dataSource = nullptr) const;
	
	/// \brief Replaces the objects in this state with copies if there are multiple references to them.
	///
	/// After calling this method, none of the objects in the flow state is referenced by anybody else.
	/// Thus, it becomes safe to modify the data objects.
	void cloneObjectsIfNeeded(bool deepCopy);

	/// \brief Tries to convert one of the to data objects stored in this flow state to the given object type.
	OORef<DataObject> convertObject(const OvitoClass& objectClass, TimePoint time) const;

	/// \brief Tries to convert one of the to data objects stored in this flow state to the given object type.
	template<class ObjectType>
	OORef<ObjectType> convertObject(TimePoint time) const {
		return static_object_cast<ObjectType>(convertObject(ObjectType::OOClass(), time));
	}

	/// \brief Gets the validity interval for this pipeline state.
	/// \return The time interval during which the returned object is valid.
	///         For times outside this interval the geometry pipeline has to be re-evaluated.
	const TimeInterval& stateValidity() const { return _stateValidity; }

	/// \brief Gets a non-const reference to the validity interval of this pipeline state.
	TimeInterval& mutableStateValidity() { return _stateValidity; }
	
	/// \brief Specifies the validity interval for this pipeline state.
	/// \param newInterval The time interval during which the object set by setResult() is valid.
	/// \sa intersectStateValidity()
	void setStateValidity(const TimeInterval& newInterval) { _stateValidity = newInterval; }

	/// \brief Reduces the validity interval of this pipeline state to include only the given time interval.
	/// \param intersectionInterval The current validity interval is reduced to include only this time interval.
	/// \sa setStateValidity()
	void intersectStateValidity(const TimeInterval& intersectionInterval) { _stateValidity.intersect(intersectionInterval); }

	/// \brief Returns true if this state object has no valid contents.
	bool isEmpty() const { return _objects.empty(); }

	/// Returns the status of the pipeline evaluation.
	const PipelineStatus& status() const { return _status; }

	/// Sets the stored status.
	void setStatus(const PipelineStatus& status) { _status = status; }

	/// Sets the stored status.
	void setStatus(PipelineStatus&& status) { _status = std::move(status); }

	/// Returns the source frame number associated with this pipeline state.
	/// If the data does not originate from a pipeline with a FileSource, returns -1.
	int sourceFrame() const;

	/// Builds a list of the global attributes stored in this pipeline state.
	QVariantMap buildAttributesMap() const;

	/// Looks up the value for the given global attribute. 
	/// Returns a given default value if the attribute is not defined in this pipeline state. 
	QVariant getAttributeValue(const QString& attrName, const QVariant& defaultValue = QVariant()) const;

	/// Looks up the value for the global attribute with the given base name and creator. 
	/// Returns a given default value if the attribute is not defined in this pipeline state. 
	QVariant getAttributeValue(const QString& attrBaseName, PipelineObject* dataSource, const QVariant& defaultValue = QVariant()) const;

private:

	/// The data that has been output by the modification pipeline.
	std::vector<StrongDataObjectRef> _objects;

	/// Contains the validity interval for this pipeline flow state.
	TimeInterval _stateValidity = TimeInterval::empty();

	/// The status of the pipeline evaluation.
	PipelineStatus _status;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
