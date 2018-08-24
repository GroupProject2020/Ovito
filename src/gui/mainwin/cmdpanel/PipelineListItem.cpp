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

#include <gui/GUI.h>
#include <core/dataset/data/DataObject.h>
#include <core/dataset/pipeline/PipelineObject.h>
#include <core/dataset/pipeline/Modifier.h>
#include "PipelineListItem.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

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
	if((event.type() == ReferenceEvent::ReferenceAdded || event.type() == ReferenceEvent::ReferenceRemoved || event.type() == ReferenceEvent::ReferenceChanged) && dynamic_object_cast<PipelineObject>(object()))
	{
		Q_EMIT subitemsChanged(this);
	}
	/// Update item if it has been enabled/disabled, its status has changed, or its title has changed.
	else if(event.type() == ReferenceEvent::TargetEnabledOrDisabled || event.type() == ReferenceEvent::ObjectStatusChanged || event.type() == ReferenceEvent::TitleChanged) {
		Q_EMIT itemChanged(this);
	}

	return RefMaker::referenceEvent(source, event);
}

/******************************************************************************
* Returns the status of the object represented by the list item.
******************************************************************************/
PipelineStatus PipelineListItem::status() const
{
	if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(object())) {
		return modApp->status();
	}
	else if(PipelineObject* pipelineObj = dynamic_object_cast<PipelineObject>(object())) {
		return pipelineObj->status();
	}
	else if(DataVis* displayObj = dynamic_object_cast<DataVis>(object())) {
		return displayObj->status();
	}
	else {
		return {};
	}
}

/******************************************************************************
* Returns the text for this list item.
******************************************************************************/
QString PipelineListItem::title() const
{ 
	switch(_itemType) {
	case Object: return object() ? object()->objectTitle() : QString();
	case SubObject:
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

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
