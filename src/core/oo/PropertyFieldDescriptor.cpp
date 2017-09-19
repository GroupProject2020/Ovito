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

#include <core/Core.h>
#include <core/utilities/units/UnitsManager.h>
#include <core/app/PluginManager.h>
#include <core/oo/RefMaker.h>
#include <core/dataset/DataSet.h>
#include "PropertyFieldDescriptor.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem)

/// Constructor	for a property field that stores a non-animatable property.
PropertyFieldDescriptor::PropertyFieldDescriptor(RefMakerClass* definingClass, const char* identifier, PropertyFieldFlags flags,
	void (*_propertyStorageCopyFunc)(RefMaker*, const RefMaker*),
	QVariant (*_propertyStorageReadFunc)(const RefMaker*),
	void (*_propertyStorageWriteFunc)(RefMaker*, const QVariant&),
	void (*_propertyStorageSaveFunc)(const RefMaker*, SaveStream&), 
	void (*_propertyStorageLoadFunc)(RefMaker*, LoadStream&))
	: _definingClassDescriptor(definingClass), _identifier(identifier), _flags(flags),
		propertyStorageCopyFunc(_propertyStorageCopyFunc), 
		propertyStorageReadFunc(_propertyStorageReadFunc), 
		propertyStorageWriteFunc(_propertyStorageWriteFunc),
		propertyStorageSaveFunc(_propertyStorageSaveFunc), 
		propertyStorageLoadFunc(_propertyStorageLoadFunc) 
{
	OVITO_ASSERT(_identifier != nullptr);
	OVITO_ASSERT(!_flags.testFlag(PROPERTY_FIELD_VECTOR));
	OVITO_ASSERT(definingClass != nullptr);
	// Make sure that there is no other reference field with the same identifier in the defining class.
	OVITO_ASSERT_MSG(definingClass->findPropertyField(identifier) == nullptr, "PropertyFieldDescriptor", 
		qPrintable(QString("Property field identifier is not unique for class %2: %1").arg(identifier).arg(definingClass->name())));
	// Insert into linked list of reference fields stored in the defining class' descriptor.
	this->_next = definingClass->_firstPropertyField;
	definingClass->_firstPropertyField = this;
}

/// Constructor	for a property field that stores a single reference to a RefTarget.
PropertyFieldDescriptor::PropertyFieldDescriptor(RefMakerClass* definingClass, OvitoClassPtr targetClass, const char* identifier, PropertyFieldFlags flags, SingleReferenceFieldBase& (*_storageAccessFunc)(RefMaker*))
	: _definingClassDescriptor(definingClass), _targetClassDescriptor(targetClass), _identifier(identifier), _flags(flags), singleStorageAccessFunc(_storageAccessFunc) 
{
	OVITO_ASSERT(_identifier != nullptr);
	OVITO_ASSERT(singleStorageAccessFunc != nullptr);
	OVITO_ASSERT(!_flags.testFlag(PROPERTY_FIELD_VECTOR));
	OVITO_ASSERT(definingClass != nullptr);
	OVITO_ASSERT(targetClass != nullptr);
	// Make sure that there is no other reference field with the same identifier in the defining class.
	OVITO_ASSERT_MSG(definingClass->findPropertyField(identifier) == nullptr, "PropertyFieldDescriptor", 
		qPrintable(QString("Property field identifier is not unique for class %2: %1").arg(identifier).arg(definingClass->name())));
	// Insert into linked list of reference fields stored in the defining class' descriptor.
	this->_next = definingClass->_firstPropertyField;
	definingClass->_firstPropertyField = this;
}

/// Constructor	for a property field that stores a vector of references to RefTarget objects.
PropertyFieldDescriptor::PropertyFieldDescriptor(RefMakerClass* definingClass, OvitoClassPtr targetClass, const char* identifier, PropertyFieldFlags flags, VectorReferenceFieldBase& (*_storageAccessFunc)(RefMaker*))
	: _definingClassDescriptor(definingClass), _targetClassDescriptor(targetClass), _identifier(identifier), _flags(flags), vectorStorageAccessFunc(_storageAccessFunc) 
{
	OVITO_ASSERT(_identifier != nullptr);
	OVITO_ASSERT(vectorStorageAccessFunc != nullptr);
	OVITO_ASSERT(_flags.testFlag(PROPERTY_FIELD_VECTOR));
	OVITO_ASSERT(definingClass != nullptr);
	OVITO_ASSERT(targetClass != nullptr);
	OVITO_ASSERT_MSG(definingClass->findPropertyField(identifier) == nullptr, "PropertyFieldDescriptor", 
		qPrintable(QString("Property field identifier is not unique for class %2: %1").arg(identifier).arg(definingClass->name())));
	// Insert into linked list of reference fields stored in the defining class' descriptor.
	this->_next = definingClass->_firstPropertyField;
	definingClass->_firstPropertyField = this;
}	

/******************************************************************************
* Return the human readable and localized name of the parameter field.
* This information is parsed from the plugin manifest file.
******************************************************************************/
QString PropertyFieldDescriptor::displayName() const
{
	if(_displayName.isEmpty())
		return identifier();
	else
		return _displayName;
}

/******************************************************************************
* Saves the current value of a property field in the application's settings store.
******************************************************************************/
void PropertyFieldDescriptor::memorizeDefaultValue(RefMaker* object) const
{
	OVITO_CHECK_OBJECT_POINTER(object);
	QSettings settings;
	settings.beginGroup(definingClass()->plugin()->pluginId());
	settings.beginGroup(definingClass()->name());
	QVariant v = object->getPropertyFieldValue(*this);
	// Workaround for bug in Qt 5.7.0: QVariants of type float do not get correctly stored
	// by QSettings (at least on macOS), because QVariant::Float is not an official type.
	if((QMetaType::Type)v.type() == QMetaType::Float)
		v = QVariant::fromValue((double)v.toFloat());
	settings.setValue(identifier(), v);
}

/******************************************************************************
* Loads the default value of a property field from the application's settings store.
******************************************************************************/
bool PropertyFieldDescriptor::loadDefaultValue(RefMaker* object) const
{
	OVITO_CHECK_OBJECT_POINTER(object);
	QSettings settings;
	settings.beginGroup(definingClass()->plugin()->pluginId());
	settings.beginGroup(definingClass()->name());
	QVariant v = settings.value(identifier());
	if(!v.isNull()) {
		//qDebug() << "Loading default value for parameter" << identifier() << "of class" << definingClass()->name() << ":" << v;
		object->setPropertyFieldValue(*this, v);
		return true;
	}
	return false;
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
