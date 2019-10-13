////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 Alexander Stukowski
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
#include <ovito/core/oo/RefTarget.h>
#include <ovito/core/dataset/animation/TimeInterval.h>
#include <ovito/core/dataset/data/DataVis.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * \brief Abstract base class for all objects that represent data.
 */
class OVITO_CORE_EXPORT DataObject : public RefTarget
{
	Q_OBJECT
	OVITO_CLASS(DataObject)

protected:

	/// \brief Constructor.
	DataObject(DataSet* dataset);

public:

	/// \brief Asks the object for its validity interval at the given time.
	/// \param time The animation time at which the validity interval should be computed.
	/// \return The maximum time interval that contains \a time and during which the object is valid.
	///
	/// When computing the validity interval of the object, an implementation of this method
	/// should take validity intervals of all sub-objects and sub-controllers into account.
	///
	/// The default implementation returns TimeInterval::infinite().
	virtual TimeInterval objectValidity(TimePoint time) { return TimeInterval::infinite(); }

	/// \brief Attaches a visualization elements to this data object that will be responsible for rendering the
	///        data.
	void addVisElement(DataVis* vis) {
		OVITO_ASSERT(vis != nullptr);
		_visElements.push_back(this, PROPERTY_FIELD(visElements), vis);
	}

	/// \brief Attaches a visualization element to this data object that will be responsible for rendering the
	///        data.
	void insertVisElement(int index, DataVis* vis) {
		OVITO_ASSERT(vis != nullptr);
		_visElements.insert(this, PROPERTY_FIELD(visElements), index, vis);
	}

	/// \brief Detaches a visualization element from this data object.
	void removeVisElement(int index) {
		_visElements.remove(this, PROPERTY_FIELD(visElements), index);
	}

	/// \brief Attaches a visual element to this data object that will be responsible for rendering the
	///        data. Any existing visual elements will be replaced.
	void setVisElement(DataVis* vis) {
		OVITO_ASSERT(vis != nullptr);
		_visElements.clear(this, PROPERTY_FIELD(visElements));
		_visElements.push_back(this, PROPERTY_FIELD(visElements), vis);
	}

	/// \brief Returns the first visualization element attached to this data object or NULL if there is
	///        no element attached.
	DataVis* visElement() const {
		return !visElements().empty() ? visElements().front() : nullptr;
	}

	/// \brief Returns the first visualization element of the given type attached to this data object or NULL if there is
	///        no such vis element attached.
	template<class DataVisType>
	DataVisType* visElement() const {
		for(DataVis* vis : visElements()) {
			if(DataVisType* typedVis = dynamic_object_cast<DataVisType>(vis))
				return typedVis;
		}
		return nullptr;
	}

	/// \brief Returns the number of strong references to this data object.
	///        Strong references are either RefMaker derived classes that hold a reference to this data object
	///        or PipelineFlowState instances that contain this data object.
	int numberOfStrongReferences() const {
		return _referringFlowStates + dependents().size();
	}

	/// Determines if it is safe to modify this data object without unwanted side effects.
	/// Returns true if there is only one exclusive owner of this data object (if any).
	/// Returns false if there are multiple references to this data object from several
	/// data collections or other container data objects.
	bool isSafeToModify() const;

	/// \brief Returns the current value of the revision counter of this scene object.
	/// This counter is increment every time the object changes.
	unsigned int revisionNumber() const Q_DECL_NOTHROW { return _revisionNumber; }

	/// Returns the pipeline object that created this data object (may be NULL).
	PipelineObject* dataSource() const;

	/// Sets the internal pointer to the pipeline object that created this data object.
	void setDataSource(PipelineObject* dataSource);

	/// Returns whether this data object wants to be shown in the pipeline editor
	/// under the data source section. The default implementation returns false.
	virtual bool showInPipelineEditor() const { return false; }

	/// \brief Visits the direct sub-objects of this data object
	///        and invokes the given visitor function for every sub-objects.
	///
	/// \param fn A functor that takes a DataObject pointer as argument and returns a bool to
	///           indicate whether visiting of further sub-objects should be stopped.
	template<class Function>
	bool visitSubObjects(Function fn) const {
		for(const PropertyFieldDescriptor* field : getOOMetaClass().propertyFields()) {
			if(field->isReferenceField() && !field->isWeakReference() && field->targetClass()->isDerivedFrom(DataObject::OOClass()) && !field->flags().testFlag(PROPERTY_FIELD_NO_SUB_ANIM)) {
				if(!field->isVector()) {
					if(const DataObject* subObject = static_object_cast<DataObject>(getReferenceField(*field).getInternal())) {
						if(fn(subObject))
							return true;
					}
				}
				else {
					const QVector<RefTarget*>& list = getVectorReferenceField(*field);
					for(const RefTarget* target : list) {
						if(const DataObject* subObject = static_object_cast<DataObject>(target)) {
							if(fn(subObject))
								return true;
						}
					}
				}
			}
		}
		return false;
	}

	/// Duplicates the given sub-object from this container object if it is shared with others.
	/// After this method returns, the returned sub-object will be exclusively owned by this container and
	/// can be safely modified without unwanted side effects.
	DataObject* makeMutable(const DataObject* subObject);

	/// Duplicates the given sub-object from this container object if it is shared with others.
	/// After this method returns, the returned sub-object will be exclusively owned by this container and
	/// can be safely modified without unwanted side effects.
	template<class DataObjectClass>
	DataObjectClass* makeMutable(const DataObjectClass* subObject) {
		return static_object_cast<DataObjectClass>(makeMutable(static_cast<const DataObject*>(subObject)));
	}

protected:

	/// \brief Sends an event to all dependents of this RefTarget.
	/// \param event The notification event to be sent to all dependents of this RefTarget.
	virtual void notifyDependentsImpl(const ReferenceEvent& event) override;

	/// Handles reference events sent by reference targets of this object.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

private:

	/// The unique identifier of the data object by which it can be referred to from Python, for example.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, identifier, setIdentifier);

	/// The attached visual elements that are responsible for rendering this object's data.
	DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD_FLAGS(DataVis, visElements, setVisElements, PROPERTY_FIELD_DONT_PROPAGATE_MESSAGES | PROPERTY_FIELD_MEMORIZE);

	/// The revision counter of this object.
	/// The counter is increment every time the object changes.
	/// See the VersionedDataObjectRef class for more information.
	unsigned int _revisionNumber = 0;

	/// Counts the current number of PipelineFlowState containers that contain this data object.
	int _referringFlowStates = 0;

	/// Pointer to the pipeline object that created this data object (may be NULL).
	QPointer<PipelineObject> _dataSource;

	template<typename DataObjectClass> friend class StrongDataObjectRef;
};

/// A pointer to a DataObject-derived metaclass.
using DataObjectClassPtr = const DataObject::OOMetaClass*;

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace

#include <ovito/core/dataset/data/StrongDataObjectRef.h>
