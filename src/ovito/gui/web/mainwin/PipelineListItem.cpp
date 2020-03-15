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

#include <ovito/gui/web/GUIWeb.h>
#include <ovito/core/dataset/data/DataObject.h>
#include <ovito/core/dataset/pipeline/PipelineObject.h>
#include <ovito/core/dataset/pipeline/Modifier.h>
#include "PipelineListItem.h"

namespace Ovito {

IMPLEMENT_OVITO_CLASS(PipelineListItem);
DEFINE_REFERENCE_FIELD(PipelineListItem, object);

/******************************************************************************
* Constructor.
******************************************************************************/
PipelineListItem::PipelineListItem(RefTarget* object, PipelineItemType itemType, PipelineListItem* parent) :
	_parent(parent), _itemType(itemType)
{
	_object.set(this, PROPERTY_FIELD(object), object);
}

/******************************************************************************
* This method is called when the object presented by the modifier
* list item generates a message.
******************************************************************************/
bool PipelineListItem::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	// The list must be updated if a modifier has been added or removed
	// from a PipelineObject, or if a data object has been added/removed from the data source.
	if((event.type() == ReferenceEvent::ReferenceAdded || event.type() == ReferenceEvent::ReferenceRemoved || event.type() == ReferenceEvent::ReferenceChanged) && dynamic_object_cast<PipelineObject>(object())) {
		Q_EMIT subitemsChanged(this);
	}
	// Update item if it has been enabled/disabled, its status has changed, or its title has changed.
	else if(event.type() == ReferenceEvent::TargetEnabledOrDisabled || event.type() == ReferenceEvent::ObjectStatusChanged || event.type() == ReferenceEvent::TitleChanged) {
		Q_EMIT itemChanged(this);
	}

	return RefMaker::referenceEvent(source, event);
}

/******************************************************************************
* Returns the status of the object represented by the list item.
******************************************************************************/
const PipelineStatus& PipelineListItem::status() const
{
	if(ActiveObject* activeObject = dynamic_object_cast<ActiveObject>(object())) {
		return activeObject->status();
	}
	else {
		static const PipelineStatus defaultStatus;
		return defaultStatus;
	}
}

/******************************************************************************
* Returns whether an active computation is in progress for this object.
******************************************************************************/
bool PipelineListItem::isObjectActive() const
{
	if(ActiveObject* activeObject = dynamic_object_cast<ActiveObject>(object())) {
		return activeObject->isObjectActive();
	}
	return false;
}

/******************************************************************************
* Returns the text for this list item.
******************************************************************************/
QString PipelineListItem::title() const
{
	switch(_itemType) {
	case VisualElement: 
	case Modifier:
	case DataObject:
		return object() ? object()->objectTitle() : QString();
	case DataSubObject:
#ifdef Q_OS_LINUX
		return object() ? (QStringLiteral("  ⇾ ") + object()->objectTitle()) : QString();
#else
		return object() ? (QStringLiteral("    ") + object()->objectTitle()) : QString();
#endif
	case VisualElementsHeader: return tr("Visual elements");
	case ModificationsHeader: return tr("Modifications");
	case DataSourceHeader: return tr("Data source");
	case PipelineBranch: return tr("Pipeline branch");
	default: return {};
	}
}

}	// End of namespace
