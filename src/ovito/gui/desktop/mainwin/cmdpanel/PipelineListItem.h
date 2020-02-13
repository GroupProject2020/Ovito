////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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


#include <ovito/gui/desktop/GUI.h>
#include <ovito/core/oo/RefMaker.h>
#include <ovito/core/oo/RefTarget.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>

namespace Ovito {

/**
 * An item managed by the PipelineListModel representing a data source, data object, modifier application or vis element.
 */
class PipelineListItem : public RefMaker
{
	Q_OBJECT
	OVITO_CLASS(PipelineListItem)

public:

	enum PipelineItemType {
		Object,
		SubObject,
		VisualElementsHeader,
		ModificationsHeader,
		DataSourceHeader,
		PipelineBranch
	};

public:

	/// Constructor.
	PipelineListItem(RefTarget* object, PipelineItemType itemType, PipelineListItem* parent = nullptr);

	/// Returns true if this is a sub-object entry.
	bool isSubObject() const { return _parent != nullptr; }

	/// Returns the parent entry if this item represents a sub-object.
	PipelineListItem* parent() const { return _parent; }

	/// Returns the status of the object represented by the list item.
	const PipelineStatus& status() const;

	/// Returns whether an active computation is in progress for this object.
	bool isObjectActive() const;

	/// Returns the title text for this list item.
	QString title() const;

	/// Returns the type of this list item.
	PipelineItemType itemType() const { return _itemType; }

Q_SIGNALS:

	/// This signal is emitted when this item has changed.
	void itemChanged(PipelineListItem* item);

	/// This signal is emitted when the list of sub-items of this item has changed.
	void subitemsChanged(PipelineListItem* parent);

protected:

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

private:

	/// The object represented by this item in the list box.
	DECLARE_REFERENCE_FIELD_FLAGS(RefTarget, object, PROPERTY_FIELD_NO_UNDO | PROPERTY_FIELD_WEAK_REF | PROPERTY_FIELD_NO_CHANGE_MESSAGE);

	/// The type of this list item.
	PipelineItemType _itemType;

	/// If this is a sub-object entry then this points to the parent.
	PipelineListItem* _parent;
};

}	// End of namespace
