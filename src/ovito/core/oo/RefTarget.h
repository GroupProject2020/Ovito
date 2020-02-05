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
#include <ovito/core/oo/NativePropertyFieldDescriptor.h>
#include <ovito/core/oo/PropertyField.h>
#include <ovito/core/oo/RefTargetExecutor.h>
#include "RefMaker.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem)

/**
 * \brief Base class for objects that are referenced by RefMaker objects.
 */
class OVITO_CORE_EXPORT RefTarget : public RefMaker
{
	Q_OBJECT
	OVITO_CLASS(RefTarget)

public:

	/// \brief This light-weight array class holds the list of dependents of a RefTarget.
	class DependentsList : public QVarLengthArray<RefMaker*, 4> {
	private:
		typedef QVarLengthArray<RefMaker*, 4> BaseClass;
	public:
		/// Checks whether an object is in the list.
		inline bool contains(RefMaker* o) const {
			for(auto iter : *this)
				if(iter == o) return true;
			return false;
		}
		/// Adds a dependent to the list.
		inline void push_back(RefMaker* o) {
			OVITO_ASSERT(!contains(o));
			BaseClass::append(o);
		}
		/// Removes a dependent from the list.
		inline void remove(RefMaker* o) {
			for(auto iter = begin(); ; ++iter) {
				OVITO_ASSERT(iter != end());
				if(*iter == o) {
					erase(iter);
					return;
				}
			}
			OVITO_ASSERT(false);
		}
	};

protected:

	/// \brief Constructor.
	/// \param dataset The dataset this object will belong to.
	RefTarget(DataSet* dataset) : RefMaker(dataset) {
		OVITO_CHECK_POINTER(dataset);
	}

#ifdef OVITO_DEBUG
	/// \brief The virtual destructor.
	virtual ~RefTarget() {
		// Make sure there are no more dependents left.
		OVITO_ASSERT_MSG(dependents().empty(), "RefTarget destructor", "RefTarget object has not been correctly deleted.");
	}
#endif

	//////////////////////////////// from OvitoObject //////////////////////////////////////

	/// This method is called after the reference counter of this object has reached zero
	/// and before the object is being deleted.
	virtual void aboutToBeDeleted() override;

	//////////////////////////// Reference event handling ////////////////////////////////

	/// \brief Sends an event to all dependents of this RefTarget.
	/// \param event The notification event to be sent to all dependents of this RefTarget.
	virtual void notifyDependentsImpl(const ReferenceEvent& event);

	/// \brief Is called when the value of a reference field of this RefMaker changes.
	/// \param field Specifies the reference field of this RefMaker that has been changed.
	///              This is always a single reference ReferenceField.
	/// \param oldTarget The old target that was referenced by the ReferenceField. This can be \c NULL.
	/// \param newTarget The new target that is now referenced by the ReferenceField. This can be \c NULL.
	///
	/// This method can by overridden by derived classes that want to be informed when
	/// any of their reference fields are changed.
	///
	/// \note When this method is overridden in sub-classes then the base implementation of this method
	///       should always be called from the new implementation to allow the base classes to handle
	///       messages for their specific reference fields.
	///
	/// The RefTarget implementation of this virtual method generates a ReferenceEvent::ReferenceChanged notification event
	virtual void referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget) override {
		notifyDependentsImpl(ReferenceFieldEvent(ReferenceEvent::ReferenceChanged, this, &field, oldTarget, newTarget));
	}

	/// \brief Is called when a RefTarget has been added to a VectorReferenceField of this RefMaker.
	/// \param field Specifies the reference field of this RefMaker to which a new entry has been added.
	///              This is always a VectorReferenceField.
	/// \param newTarget The new target added to the list of referenced objects.
	/// \param listIndex The index into the VectorReferenceField at which the new entry has been inserted.
	///
	/// This method can by overridden by derived classes that want to be informed when
	/// a reference has been added to one of its vector reference fields.
	///
	/// \note When this method is overridden in sub-classes then the base implementation of this method
	///       should always be called from the new implementation to allow the base classes to handle
	///       messages for their specific reference fields.
	///
	/// The RefTarget implementation of this virtual method generates a ReferenceEvent::ReferenceAdded notification event
	virtual void referenceInserted(const PropertyFieldDescriptor& field, RefTarget* newTarget, int listIndex) override {
		notifyDependentsImpl(ReferenceFieldEvent(ReferenceEvent::ReferenceAdded, this, &field, nullptr, newTarget, listIndex));
	}

	/// \brief Is called when a RefTarget has been removed from a VectorReferenceField of this RefMaker.
	/// \param field Specifies the reference field of this RefMaker from which an entry has been removed.
	///              This is always a VectorReferenceField.
	/// \param oldTarget The old target that was reference before it has been removed from the vector reference field.
	/// \param listIndex The index into the VectorReferenceField at which the old entry was stored.
	///
	/// This method can by overridden by derived classes that want to be informed when
	/// a reference has been removed from one of its vector reference fields.
	///
	/// \note When this method is overridden in sub-classes then the base implementation of this method
	///       should always be called from the new implementation to allow the base classes to handle
	///       messages for their specific reference fields.
	///
	/// The RefTarget implementation of this virtual method generates a ReferenceEvent::ReferenceRemoved notification event
	virtual void referenceRemoved(const PropertyFieldDescriptor& field, RefTarget* oldTarget, int listIndex) override {
		notifyDependentsImpl(ReferenceFieldEvent(ReferenceEvent::ReferenceRemoved, this, &field, oldTarget, nullptr, listIndex));
	}

	/// \brief Handles a notification event from a RefTarget referenced by this object.
	/// \param source Specifies the RefTarget that delivered the event.
	/// \param event The notification event.
	/// \return If \c true then the message is passed on to all dependents of this object.
	virtual bool handleReferenceEvent(RefTarget* source, const ReferenceEvent& event) override;

	//////////////////////////////// Object cloning //////////////////////////////////////

	/// \brief Creates a copy of this RefTarget object.
	/// \param deepCopy If \c true, then all objects referenced by this RefTarget should also be copied.
	///                 If \c false, then the new object clone should just take over the references of the original object
	///                 and no copying of sub-objects takes place.
	/// \param cloneHelper Copying of sub-objects should be done using the passed CloneHelper instance.
	///                    It makes sure that only one copy per object is made through the whole object graph.
	///
	/// The default implementation of this method takes care of instance creation. It creates
	/// a new instance of the original object class.
	///
	/// Sub-classes should override this method and must always call the base class' version of this method
	/// to create the new instance. The returned smart pointer can safely be cast to the class type of the original
	/// object.
	///
	/// Every sub-class that has reference fields or other internal data fields should override this
	/// method and copy or transfer the members to the new clone.
	///
	/// \sa CloneHelper::cloneObject()
	virtual OORef<RefTarget> clone(bool deepCopy, CloneHelper& cloneHelper) const;

public:

	/// \brief Returns true if this object is an instance of a RefTarget derived class.
	virtual bool isRefTarget() const override { return true; }

	//////////////////////////////// Notification events ////////////////////////////////////

	/// \brief Sends an event to all dependents of this RefTarget.
	/// \param eventType The event type passed to the ReferenceEvent constructor.
	inline void notifyDependents(ReferenceEvent::Type eventType) const {
		OVITO_ASSERT(eventType != ReferenceEvent::TargetChanged);
		OVITO_ASSERT(eventType != ReferenceEvent::ReferenceChanged);
		OVITO_ASSERT(eventType != ReferenceEvent::ReferenceAdded);
		OVITO_ASSERT(eventType != ReferenceEvent::ReferenceRemoved);
		const_cast<RefTarget*>(this)->notifyDependentsImpl(ReferenceEvent(eventType, const_cast<RefTarget*>(this)));
	}

	/// \brief Sends a ReferenceEvent::TargetChanged event to all dependents of this RefTarget.
	inline void notifyTargetChanged(const PropertyFieldDescriptor* field = nullptr) const {
		const_cast<RefTarget*>(this)->notifyDependentsImpl(PropertyFieldEvent(ReferenceEvent::TargetChanged, const_cast<RefTarget*>(this), field));
	}

	////////////////////////////////// Dependency graph ///////////////////////////////////////

	/// \brief Checks whether this RefTarget is directly or indirectly referenced by a RefMaker.
	/// \param obj The RefMaker that might hold a reference to \c this object.
	/// \return \c true if \a obj has a direct or indirect reference to this RefTarget;
	///         \c false if \a obj does not depend on this RefTarget.
	virtual bool isReferencedBy(const RefMaker* obj) const override;

	/// \brief Returns the list of RefMaker objects that hold at least one direct reference
	///        to this RefTarget.
	/// \return A list of reference makers that directly depend on this RefTarget and receive
	///         notification messages from it.
	const DependentsList& dependents() const { return _dependents; }

	/// \brief Recursively visits all dependents that directly or indirectly reference this target object
	///        and invokes the given function for every dependent encountered.
	///
	/// \note The visitor function may be called multiple times for a dependent if that dependent
	///       has multiple references that in turn reference this target.
	template<class Function>
	void visitDependents(Function fn) const {
		for(RefMaker* dependent : dependents()) {
			fn(dependent);
			if(dependent->isRefTarget())
				static_object_cast<RefTarget>(dependent)->visitDependents(fn);
		}
	}

	/// \brief Generates a list of dependents that directly or indirectly reference this target object.
	/// \return A list of all dependents that are instance of the object type specified by the template parameter.
	template<class ObjectType>
	QSet<ObjectType*> findDependents() const {
		QSet<ObjectType*> results;
		visitDependents([&results](RefMaker* dependent) {
			if(ObjectType* o = dynamic_object_cast<ObjectType>(dependent))
				results.insert(o);
		});
		return results;
	}

	/// \brief Asks this object to delete itself.
	///
	/// If undo recording is active, the object instance is kept alive such that
	/// the deletion can be undone.
	virtual void deleteReferenceObject();

	/// \brief Returns the title of this object.
	/// \return A string that is used as label or title for this object in the user interface.
	///
	/// The default implementation returns OvitoClass::objectTitle().
	/// Sub-classes can override this method to return a title that depends on the internal state of the object.
	virtual QString objectTitle() const;

	/// \brief Flags this object when it is opened in an editor.
	void setObjectEditingFlag();

	/// \brief Unflags this object when it is no longer opened in an editor.
	void unsetObjectEditingFlag();

	/// \brief Determines if this object's properties are currently being edited in an editor.
	bool isObjectBeingEdited() const;

	/// Returns an executor object to be used with Future<>::then(), which executes work
	/// in the context (and the thread) of this object.
	RefTargetExecutor executor() const { return RefTargetExecutor(this); }

private:

	/// The list of reference fields that hold a reference to this target.
	DependentsList _dependents;

	friend class RefMaker;
	friend class CloneHelper;
	friend class SingleReferenceFieldBase;
	friend class VectorReferenceFieldBase;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
