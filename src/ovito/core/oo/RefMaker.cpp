////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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
#include <ovito/core/app/Application.h>
#include <ovito/core/oo/RefTarget.h>
#include <ovito/core/dataset/UndoStack.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/app/PluginManager.h>
#include <ovito/core/utilities/io/ObjectSaveStream.h>
#include <ovito/core/utilities/io/ObjectLoadStream.h>
#include <ovito/core/dataset/animation/controller/Controller.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include "RefMaker.h"

namespace Ovito {

IMPLEMENT_OVITO_CLASS(RefMaker);

/******************************************************************************
* Constructor.
******************************************************************************/
RefMaker::RefMaker(DataSet* dataset) : _dataset(dataset)
{
}

/******************************************************************************
* This method is called when the reference counter of this OvitoObject
* has reached zero.
******************************************************************************/
void RefMaker::aboutToBeDeleted()
{
	OVITO_CHECK_OBJECT_POINTER(this);

	// Make sure undo recording is not active while deleting a RefTarget.
	OVITO_ASSERT_MSG(!isRefTarget() || !dataset() || dataset()->undoStack().isRecording() == false, "RefMaker::aboutToBeDeleted()", "Cannot delete object from memory while undo recording is active.");

	// Clear all references this object has to other objects.
	clearAllReferences();

	OvitoObject::aboutToBeDeleted();
}

/******************************************************************************
* This helper method throws an Exception with the given message text.
******************************************************************************/
void RefMaker::throwException(const QString& msg) const
{
	throw Exception(msg, dataset());
}

/******************************************************************************
* Returns the value stored in a non-animatable property field of this RefMaker object.
******************************************************************************/
QVariant RefMaker::getPropertyFieldValue(const PropertyFieldDescriptor& field) const
{
	OVITO_ASSERT_MSG(!field.isReferenceField(), "RefMaker::getPropertyFieldValue", "This function may be used only to access property fields and not reference fields.");
	OVITO_ASSERT_MSG(getOOClass().isDerivedFrom(*field.definingClass()), "RefMaker::getPropertyFieldValue", "The property field has not been defined in this class or its base classes.");
	OVITO_ASSERT_MSG(field.propertyStorageReadFunc != nullptr, "RefMaker::getPropertyFieldValue", "The property field is a runtime property field, which doesn't allow conversion to a QVariant value.");
	return field.propertyStorageReadFunc(const_cast<RefMaker*>(this));
}

/******************************************************************************
* Sets the value stored in a non-animatable property field of this RefMaker object.
******************************************************************************/
void RefMaker::setPropertyFieldValue(const PropertyFieldDescriptor& field, const QVariant& newValue)
{
	OVITO_ASSERT_MSG(!field.isReferenceField(), "RefMaker::setPropertyFieldValue", "This function may be used only to access property fields and not reference fields.");
	OVITO_ASSERT_MSG(getOOClass().isDerivedFrom(*field.definingClass()), "RefMaker::setPropertyFieldValue", "The property field has not been defined in this class or its base classes.");
	OVITO_ASSERT_MSG(field.propertyStorageWriteFunc != nullptr, "RefMaker::getPropertyFieldValue", "The property field is a runtime property field, which doesn't allow assignment of a QVariant value.");
	field.propertyStorageWriteFunc(this, newValue);
}

/******************************************************************************
* Copies the value stored in a non-animatable property field of from another
* RefMaker instance to this RefMaker object.
******************************************************************************/
void RefMaker::copyPropertyFieldValue(const PropertyFieldDescriptor& field, const RefMaker& other)
{
	OVITO_ASSERT_MSG(!field.isReferenceField(), "RefMaker::copyPropertyFieldValue", "This function may be used only to access property fields and not reference fields.");
	OVITO_ASSERT_MSG(getOOClass().isDerivedFrom(*field.definingClass()), "RefMaker::copyPropertyFieldValue", "The property field has not been defined in this class or its base classes.");
	OVITO_ASSERT_MSG(other.getOOClass().isDerivedFrom(*field.definingClass()), "RefMaker::copyPropertyFieldValue", "The property field has not been defined in the source's class or its base classes.");
	OVITO_ASSERT(field.propertyStorageCopyFunc != nullptr);
	field.propertyStorageCopyFunc(this, &other);
}

/******************************************************************************
* Looks up the reference field.
******************************************************************************/
const SingleReferenceFieldBase& RefMaker::getReferenceField(const PropertyFieldDescriptor& field) const
{
	OVITO_ASSERT_MSG(field.isReferenceField(), "RefMaker::getReferenceField", "This function may not be used to retrieve property fields.");
	OVITO_ASSERT_MSG(field.isVector() == false, "RefMaker::getReferenceField", "This function may not be used to retrieve vector reference fields.");
	OVITO_ASSERT_MSG(getOOClass().isDerivedFrom(*field.definingClass()), "RefMaker::getReferenceField", "The reference field has not been defined in this class or its base classes.");
	OVITO_ASSERT(field.singleStorageAccessFunc != nullptr);
	return field.singleStorageAccessFunc(const_cast<RefMaker*>(this));
}

/******************************************************************************
* Looks up the vector reference field.
******************************************************************************/
const VectorReferenceFieldBase& RefMaker::getVectorReferenceField(const PropertyFieldDescriptor& field) const
{
	OVITO_ASSERT_MSG(field.isReferenceField(), "RefMaker::getVectorReferenceField", "This function may not be used to retrieve property fields.");
	OVITO_ASSERT_MSG(field.isVector() == true, "RefMaker::getVectorReferenceField", "This function may not be used to retrieve single reference fields.");
	OVITO_ASSERT_MSG(getOOClass().isDerivedFrom(*field.definingClass()), "RefMaker::getVectorReferenceField", "The reference field has not been defined in this class or its base classes.");
	OVITO_ASSERT(field.vectorStorageAccessFunc != nullptr);
	return field.vectorStorageAccessFunc(const_cast<RefMaker*>(this));
}

/******************************************************************************
* Handles a notification event from a RefTarget referenced by this object.
******************************************************************************/
bool RefMaker::handleReferenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	OVITO_CHECK_OBJECT_POINTER(this);

	// Handle delete messages.
	if(event.type() ==  ReferenceEvent::TargetDeleted) {
		OVITO_ASSERT(source == event.sender());
		referenceEvent(source, event);
		OVITO_CHECK_OBJECT_POINTER(this);
		clearReferencesTo(event.sender());
		return false;
	}

	// Let the RefMaker-derived class process the message.
	return referenceEvent(source, event);
}

/******************************************************************************
* Is called when a RefTarget referenced by this object has generated an event.
******************************************************************************/
bool RefMaker::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(event.shouldPropagate()) {
		// Check if message is comming from a reference field for which message propagation is explicitly disabled.
		for(const PropertyFieldDescriptor* field : getOOMetaClass().propertyFields()) {
			if(!field->isReferenceField()) continue;
			if(!field->flags().testFlag(PROPERTY_FIELD_DONT_PROPAGATE_MESSAGES)) continue;
			if(!field->isVector()) {
				if(getReferenceField(*field) == source)
					return false;
			}
			else {
				if(getVectorReferenceField(*field).contains(source))
					return false;
			}
		}
		return true;
	}
	return false;
}

/******************************************************************************
* Checks if this RefMaker has any reference to the given RefTarget.
******************************************************************************/
bool RefMaker::hasReferenceTo(const RefTarget* target) const
{
	if(!target) return false;
	OVITO_CHECK_OBJECT_POINTER(target);

	for(const PropertyFieldDescriptor* field : getOOMetaClass().propertyFields()) {
		if(!field->isReferenceField()) continue;
		if(!field->isVector()) {
			if(getReferenceField(*field) == target)
				return true;
		}
		else {
			if(getVectorReferenceField(*field).contains(target))
				return true;
		}
	}
	return false;
}

/******************************************************************************
* Replaces all references of this RefMaker to the old RefTarget with
* the new RefTarget.
******************************************************************************/
void RefMaker::replaceReferencesTo(const RefTarget* oldTarget, const RefTarget* newTarget)
{
	if(!oldTarget) return;
	OVITO_CHECK_OBJECT_POINTER(oldTarget);

	// Check for cyclic references first.
	if(newTarget && isReferencedBy(newTarget))
		throw CyclicReferenceError();

	// Iterate over all reference fields in the class hierarchy.
	bool hasBeenReplaced = false;
	const OvitoClass& oldTargetClass = oldTarget->getOOClass();
	for(const PropertyFieldDescriptor* field : getOOMetaClass().propertyFields()) {
		if(!field->isReferenceField()) continue;
		if(!oldTargetClass.isDerivedFrom(*field->targetClass())) continue;
		if(!field->isVector()) {
			SingleReferenceFieldBase& singleField = field->singleStorageAccessFunc(this);
			if(singleField == oldTarget) {
				singleField.setInternal(this, *field, newTarget);
				hasBeenReplaced = true;
			}
		}
		else {
			VectorReferenceFieldBase& vectorField = field->vectorStorageAccessFunc(this);
			for(int i = vectorField.size(); i--;) {
				if(vectorField[i] == oldTarget) {
					vectorField.remove(this, *field, i);
					vectorField.insertInternal(this, *field, newTarget, i);
					hasBeenReplaced = true;
				}
			}
		}
	}
	OVITO_ASSERT_MSG(hasBeenReplaced, "RefMaker::replaceReferencesTo", "The target to be replaced was not referenced by this RefMaker.");
}

/******************************************************************************
* Stops observing a RefTarget object.
* All single reference fields containing the RefTarget will be reset to NULL.
* If the target is referenced in a vector reference field then the item is
* removed from the vector.
******************************************************************************/
void RefMaker::clearReferencesTo(const RefTarget* target)
{
	if(!target) return;
	OVITO_CHECK_OBJECT_POINTER(target);

	// Iterate over all reference fields in the class hierarchy.
	for(const PropertyFieldDescriptor* field : getOOMetaClass().propertyFields()) {
		if(!field->isReferenceField()) continue;
		if(!field->isVector()) {
			SingleReferenceFieldBase& singleField = field->singleStorageAccessFunc(this);
			if(singleField == target)
				singleField.setInternal(this, *field, nullptr);
		}
		else {
			VectorReferenceFieldBase& vectorField = field->vectorStorageAccessFunc(this);
			for(int i=vectorField.size(); i--;) {
				if(vectorField[i] == target)
					vectorField.remove(this, *field, i);
			}
		}
	}
}

/******************************************************************************
* Clears all references held by this RefMarker.
******************************************************************************/
void RefMaker::clearAllReferences()
{
	OVITO_CHECK_OBJECT_POINTER(this);
	OVITO_ASSERT_MSG(getOOClass() != RefMaker::OOClass(), "RefMaker::clearAllReferences", "clearAllReferences() must not be called from the RefMaker destructor.");

	// Iterate over all reference fields in the class hierarchy.
	for(const PropertyFieldDescriptor* field : getOOMetaClass().propertyFields()) {
		if(field->isReferenceField())
			clearReferenceField(*field);
	}
}

/******************************************************************************
* Clears the given reference field.
* If this is a single reference field then it is set to NULL.
* If it is a list reference field the all references are removed.
******************************************************************************/
void RefMaker::clearReferenceField(const PropertyFieldDescriptor& field)
{
	OVITO_ASSERT_MSG(field.isReferenceField(), "RefMaker::clearReferenceField", "This function may not be used for property fields.");
	OVITO_ASSERT_MSG(getOOClass().isDerivedFrom(*field.definingClass()), "RefMaker::clearReferenceField()", "The reference field has not been defined in this class or its base classes.");

	if(!field.isVector())
		field.singleStorageAccessFunc(this).setInternal(this, field, nullptr);
	else
		field.vectorStorageAccessFunc(this).clear(this, field);
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void RefMaker::saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData)
{
	OvitoObject::saveToStream(stream, excludeRecomputableData);

#if 0
	qDebug() << "Saving object" << this;
#endif

	// Iterate over all property fields in the class hierarchy.
	for(const PropertyFieldDescriptor* field : getOOMetaClass().propertyFields()) {

		if(field->isReferenceField()) {
			// Write the object pointed to by the reference field to the stream.

			// Write reference target object to stream.
			stream.beginChunk(0x02);
			try {
				if(!field->isVector()) {
					stream.saveObject(getReferenceField(*field), excludeRecomputableData || field->dontSaveRecomputableData());
				}
				else {
					const QVector<RefTarget*>& list = getVectorReferenceField(*field);
					stream << (qint32)list.size();
					for(RefTarget* target : list)
						stream.saveObject(target, excludeRecomputableData || field->dontSaveRecomputableData());
				}
			}
			catch(Exception& ex) {
				ex.prependGeneralMessage(tr("Failed to serialize contents of reference field %1 of class %2.").arg(field->identifier()).arg(field->definingClass()->name()));
				throw ex;
			}
			stream.endChunk();
		}
		else {
			// Write the primitive value stored in the property field to the stream.
			if(field->propertyStorageSaveFunc != nullptr) {
				stream.beginChunk(0x04);
				field->propertyStorageSaveFunc(this, stream);
#if 0
				qDebug() << "  Property field" << field->identifier() << " contains" << field->propertyStorageReadFunc(this);
#endif
			}
			else {
				// Indicate that this property field is not serizaliable.
				stream.beginChunk(0x05);
			}
			stream.endChunk();
		}
	}
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void RefMaker::loadFromStream(ObjectLoadStream& stream)
{
	OvitoObject::loadFromStream(stream);
	OVITO_ASSERT(!dataset()->undoStack().isRecording());

#if 0
	qDebug() << "Loading object" << this;
#endif

	// Look up the serialized metadata for this RefMaker-derived class,
	// which was loaded from the input stream.
	const RefMakerClass::SerializedClassInfo* classInfo = static_cast<const RefMakerClass::SerializedClassInfo*>(stream.getSerializedClassInfo());

	// Read property field values from the stream.
	for(const RefMakerClass::SerializedClassInfo::PropertyFieldInfo& fieldEntry : classInfo->propertyFields) {
		if(fieldEntry.isReferenceField) {
			OVITO_ASSERT(fieldEntry.targetClass != nullptr);

			// Parse target object(s).
			int chunkId = stream.openChunk();
			if(chunkId == 0x02) {

				// Parse target object chunk.
				if(fieldEntry.field != nullptr) {
					OVITO_CHECK_POINTER(fieldEntry.field);
					OVITO_ASSERT(fieldEntry.field->isVector() == ((fieldEntry.field->flags() & PROPERTY_FIELD_VECTOR) != 0));
					OVITO_ASSERT(fieldEntry.targetClass->isDerivedFrom(*fieldEntry.field->targetClass()));
					if(!fieldEntry.field->isVector()) {
						OORef<RefTarget> target = stream.loadObject<RefTarget>();
						if(target && !target->getOOClass().isDerivedFrom(*fieldEntry.targetClass)) {
							throwException(tr("Incompatible object stored in reference field %1 of class %2. Expected class %3 but found class %4 in file.")
								.arg(QString(fieldEntry.identifier)).arg(fieldEntry.definingClass->name()).arg(fieldEntry.targetClass->name()).arg(target->getOOClass().name()));
						}
#if 0
						qDebug() << "  Reference field" << fieldEntry.identifier << " contains" << target;
#endif
						fieldEntry.field->singleStorageAccessFunc(this).setInternal(this, *fieldEntry.field, target);
					}
					else {
						// Get storage address of member variable.
						VectorReferenceFieldBase& refField = fieldEntry.field->vectorStorageAccessFunc(this);
						refField.clear(this, *fieldEntry.field);

						// Load each target object and store it in the list reference field.
						qint32 numEntries;
						stream >> numEntries;
						OVITO_ASSERT(numEntries >= 0);
						for(qint32 i = 0; i < numEntries; i++) {
							OORef<RefTarget> target = stream.loadObject<RefTarget>();
							if(target && !target->getOOClass().isDerivedFrom(*fieldEntry.targetClass)) {
								throwException(tr("Incompatible object stored in reference field %1 of class %2. Expected class %3 but found class %4 in file.")
									.arg(QString(fieldEntry.identifier)).arg(fieldEntry.definingClass->name(), fieldEntry.targetClass->name(), target->getOOClass().name()));
							}
#if 0
							qDebug() << "  Vector reference field" << fieldEntry.identifier << " contains" << target;
#endif
							refField.insertInternal(this, *fieldEntry.field, target);
						}
					}
				}
				else {
#if 0
					qDebug() << "  Reference field" << fieldEntry.identifier << " no longer exists.";
#endif
					// The serialized reference field no longer exists in the current program version.
					// Load object from stream and release it immediately.
					if(fieldEntry.flags & PROPERTY_FIELD_VECTOR) {
						qint32 numEntries;
						stream >> numEntries;
						for(qint32 i = 0; i < numEntries; i++)
							stream.loadObject<RefTarget>();
					}
					else {
						stream.loadObject<RefTarget>();
					}
				}
			}
			else {
				throwException(tr("Expected reference field '%1' in object %2").arg(QString(fieldEntry.identifier)).arg(fieldEntry.definingClass->name()));
			}
			stream.closeChunk();
		}
		else {
			// Read the primitive value of the property field from the stream.
			OVITO_ASSERT(fieldEntry.targetClass == nullptr);
			int chunkId = stream.openChunk();
			if(chunkId == 0x04) {
				if(!loadPropertyFieldFromStream(stream, fieldEntry)) {
					if(fieldEntry.field && fieldEntry.field->propertyStorageLoadFunc != nullptr) {
						fieldEntry.field->propertyStorageLoadFunc(this, stream);
					}
					else {
						// The property field no longer exists.
						// Ignore chunk contents.
					}
				}
			}
			else if(chunkId != 0x05) {
				throwException(tr("Expected non-serializable property field '%1' in object %2").arg(QString(fieldEntry.identifier)).arg(fieldEntry.definingClass->name()));
			}
			stream.closeChunk();
		}
	}

#if 0
	qDebug() << "Done loading automatic fields of " << this;
#endif
}

/******************************************************************************
* Returns a list of all targets this RefMaker depends on (both
* directly and indirectly).
******************************************************************************/
QSet<RefTarget*> RefMaker::getAllDependencies() const
{
	QSet<RefTarget*> nodes;
	walkNode(nodes, this);
	return nodes;
}

/******************************************************************************
* Recursive gathering function.
******************************************************************************/
void RefMaker::walkNode(QSet<RefTarget*>& nodes, const RefMaker* node)
{
	OVITO_CHECK_OBJECT_POINTER(node);

	// Iterate over all reference fields in the class hierarchy.
	for(const PropertyFieldDescriptor* field : node->getOOMetaClass().propertyFields()) {
		if(!field->isReferenceField()) continue;
		if(!field->isVector()) {
			RefTarget* target = node->getReferenceField(*field);
			if(target != nullptr && !nodes.contains(target)) {
				nodes.insert(target);
				walkNode(nodes, target);
			}
		}
		else {
			const QVector<RefTarget*>& list = node->getVectorReferenceField(*field);
			for(RefTarget* target : list) {
				if(target != nullptr && !nodes.contains(target)) {
					nodes.insert(target);
					walkNode(nodes, target);
				}
			}
		}
	}
}

/******************************************************************************
* Loads the user-defined default values of this object's parameter fields from the
* application's settings store.
*
* This function should be called immediately after creation of the object instance.
* It loads the default value for every property field for which the user has set
* a default value. This is usually the case for property fields that have the
* PROPERTY_FIELD_MEMORIZE flag set.
*
* This function is recursive, i.e., it also loads default parameter values for
* referenced objects (when the PROPERTY_FIELD_MEMORIZE flag is set for this RefMaker's reference field).
******************************************************************************/
void RefMaker::loadUserDefaults()
{
#ifdef OVITO_DEBUG
	if(Application::instance()->executionContext() == Application::ExecutionContext::Scripting)
		qWarning() << "Warning: loadUserDefaults() called in a scripting context for" << this;
#endif

	// Iterate over all property fields in the class hierarchy.
	for(const PropertyFieldDescriptor* field : getOOMetaClass().propertyFields()) {
		if(field->flags().testFlag(PROPERTY_FIELD_MEMORIZE)) {
			if(field->isReferenceField()) {
				// If it's a reference field, recursively call loadUserDefaults() on the reference object(s).
				if(!field->isVector()) {
					if(RefTarget* target = getReferenceField(*field)) {
						target->loadUserDefaults();

						// If it's a controller type, load default controller value.
						if(Controller* ctrl = dynamic_object_cast<Controller>(target)) {
							QSettings settings;
							settings.beginGroup(getOOClass().plugin()->pluginId());
							settings.beginGroup(getOOClass().name());
							QVariant v = settings.value(field->identifier());
							if(!v.isNull()) {
								if(ctrl->controllerType() == Controller::ControllerTypeFloat) {
									ctrl->setFloatValue(0, v.value<FloatType>());
								}
								else if(ctrl->controllerType() == Controller::ControllerTypeInt) {
									ctrl->setIntValue(0, v.value<int>());
								}
								else if(ctrl->controllerType() == Controller::ControllerTypeVector3) {
									ctrl->setVector3Value(0, v.value<Vector3>());
								}
							}
						}
					}
				}
				else {
					const QVector<RefTarget*>& list = getVectorReferenceField(*field);
					for(RefTarget* target : list) {
						if(target) target->loadUserDefaults();
					}
				}
			}
			else {
				// If it's a property field, load the user-defined default value.
				field->loadDefaultValue(this);
			}
		}
	}
}

}	// End of namespace
