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
#include <core/scene/objects/SceneObject.h>
#include <core/scene/pipeline/PipelineObject.h>
#include <core/scene/pipeline/Modifier.h>
#include "ModificationListItem.h"

namespace Ovito {

IMPLEMENT_OVITO_OBJECT(Core, ModificationListItem, RefTarget)
DEFINE_FLAGS_REFERENCE_FIELD(ModificationListItem, _object, "Object", RefTarget, PROPERTY_FIELD_NO_UNDO)
DEFINE_FLAGS_VECTOR_REFERENCE_FIELD(ModificationListItem, _modApps, "ModifierApplications", ModifierApplication, PROPERTY_FIELD_NO_UNDO)

/******************************************************************************
* Constructor.
******************************************************************************/
ModificationListItem::ModificationListItem(RefTarget* object, bool isSubObject, const QString& title) :
	_isSubObject(isSubObject), _title(title)
{
	INIT_PROPERTY_FIELD(ModificationListItem::_object);
	INIT_PROPERTY_FIELD(ModificationListItem::_modApps);

	this->_object = object;
}

/******************************************************************************
* This method is called when the object presented by the modifier
* list item generates a message.
******************************************************************************/
bool ModificationListItem::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	// The modifier stack list must be updated if a modifier has been added or removed
	// from a PipelineObject.
	if((event->type() == ReferenceEvent::ReferenceAdded || event->type() == ReferenceEvent::ReferenceRemoved || event->type() == ReferenceEvent::ReferenceChanged)
		&& source == object() && dynamic_object_cast<PipelineObject>(object()))
	{
		Q_EMIT subitemsChanged(this);
	}
	/// Update item if an object has been enabled or disabled.
	else if(event->type() == ReferenceEvent::TargetEnabledOrDisabled && source == object() && event->sender() == object()) {
		Q_EMIT itemChanged(this);
	}
	/// Update an entry if the evaluation status of the modifier has changed.
	else if(event->type() == ReferenceEvent::StatusChanged || event->type() == ReferenceEvent::TitleChanged) {
		Q_EMIT itemChanged(this);
	}
	/// If the list of sub-objects changes for one of the entries, we need
	/// to update everything.
	else if(event->type() == ReferenceEvent::SubobjectListChanged && source == object() && event->sender() == object()) {
		Q_EMIT subitemsChanged(this);
	}

	return RefTarget::referenceEvent(source, event);
}

/******************************************************************************
* Returns the status of the object represented by the list item.
******************************************************************************/
ModificationListItem::Status ModificationListItem::status() const
{
	ObjectStatus status;
	Modifier* modifier = dynamic_object_cast<Modifier>(object());
	if(modifier)
		status = modifier->status();
	else {
		SceneObject* sceneObject = dynamic_object_cast<SceneObject>(object());
		if(sceneObject)
			status = sceneObject->status();
	}
	if(status.type() == ObjectStatus::Success) {
		if(!status.shortText().isEmpty())
			return Info;
	}
	else if(status.type() == ObjectStatus::Warning)
		return Warning;
	else if(status.type() == ObjectStatus::Error)
		return Error;
	else if(status.type() == ObjectStatus::Pending)
		return Pending;

	return None;
}

/******************************************************************************
* Returns the tooltip text for the list item.
******************************************************************************/
QVariant ModificationListItem::toolTip() const
{
	Modifier* modifier = dynamic_object_cast<Modifier>(object());
	if(modifier && modifier->isEnabled()) {
		ObjectStatus status = modifier->status();
		if(status.shortText().isEmpty() == false)
			return qVariantFromValue(status.shortText());
	}
	return QVariant();
}

};