///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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
#include <core/oo/RefTarget.h>
#include <core/dataset/animation/TimeInterval.h>
#include <core/dataset/data/DataVis.h>

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

	/// \brief This asks the object whether it supports the conversion to another object type.
	/// \param objectClass The destination type. This must be a DataObject derived class.
	/// \return \c true if this object can be converted to the requested type given by \a objectClass or any sub-class thereof.
	///         \c false if the conversion is not possible.
	///
	/// The default implementation returns \c true if the class \a objectClass is the source object type or any derived type.
	/// This is the trivial case: It requires no real conversion at all.
	///
	/// Sub-classes should override this method to allow the conversion to a MeshObject, for example.
	/// When overriding, the base implementation of this method should always be called.
	virtual bool canConvertTo(const OvitoClass& objectClass) {
		// Can always convert to itself.
		return this->getOOClass().isDerivedFrom(objectClass);
	}

	/// \brief Lets the object convert itself to another object type.
	/// \param objectClass The destination type. This must be a DataObject derived class.
	/// \param time The time at which to convert the object.
	/// \return The newly created object or \c NULL if no conversion is possible.
	///
	/// Whether the object can be converted to the desired destination type can be checked in advance using
	/// the canConvertTo() method.
	///
	/// Sub-classes should override this method to allow the conversion to a MeshObject for example.
	/// When overriding, the base implementation of this method should always be called.
	virtual OORef<DataObject> convertTo(const OvitoClass& objectClass, TimePoint time) {
		// Trivial conversion.
		if(this->getOOClass().isDerivedFrom(objectClass))
			return this;
		else
			return {};
	}

	/// \brief Lets the object convert itself to another object type.
	/// \param time The time at which to convert the object.
	///
	/// This is a wrapper of the function above using C++ templates.
	/// It just casts the conversion result to the given class.
	template<class T>
	OORef<T> convertTo(TimePoint time) {
		return static_object_cast<T>(convertTo(T::OOClass(), time));
	}

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

	/// \brief Returns the number of strong references to this data object.
	///        Strong references are either RefMaker derived classes that hold a reference to this data object
	///        or PipelineFlowState instances that contain this data object.
	int numberOfStrongReferences() const { 
		return _referringFlowStates + dependents().size();
	}

	/// \brief Returns the current value of the revision counter of this scene object.
	/// This counter is increment every time the object changes.
	unsigned int revisionNumber() const Q_DECL_NOTHROW { return _revisionNumber; }

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

	/// The attached visual elements that are responsible for rendering this object's data.
	DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD_FLAGS(DataVis, visElements, setVisElements, PROPERTY_FIELD_DONT_PROPAGATE_MESSAGES);

	/// The revision counter of this object.
	/// The counter is increment every time the object changes.
	/// See the VersionedDataObjectRef class for more information.
	unsigned int _revisionNumber = 0;

	/// Counts the current number of PipelineFlowState containers that contain this data object.
	int _referringFlowStates = 0;

	friend class StrongDataObjectRef;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


