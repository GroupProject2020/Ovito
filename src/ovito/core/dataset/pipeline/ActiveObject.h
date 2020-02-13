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
#include <ovito/core/oo/RefTarget.h>
#include <ovito/core/dataset/pipeline/PipelineStatus.h>

namespace Ovito {

/**
 * \brief Abstract base class for objects perform long-running computations and which 
 *        can be enabled or disabled.
 */
class OVITO_CORE_EXPORT ActiveObject : public RefTarget
{
	Q_OBJECT
	OVITO_CLASS(ActiveObject)

protected:

	/// \brief Constructor.
	ActiveObject(DataSet* dataset);

public:

	/// \brief Returns the title of this object.
	virtual QString objectTitle() const override {
		if(title().isEmpty()) 
			return RefTarget::objectTitle();
		else
			return title();
	}

	/// \brief Changes the title of this object.
	/// \undoable
	void setObjectTitle(const QString& title) { 
		setTitle(title); 
	}

	/// \brief Returns true if at least one computation task associated with this object is currently active. 
	bool isObjectActive() const { return _numberOfActiveTasks > 0; }

protected:

	/// Is called when the value of a non-animatable property field of this RefMaker has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Increments the internal task counter and notifies the UI that this object is currently active.
	void incrementNumberOfActiveTasks();

	/// Decrements the internal task counter and, if the counter has reached zero, notifies the 
	/// UI that this object is no longer active.
	void decrementNumberOfActiveTasks();

	/// Registers the given future as an active task associated with this object.
	void registerActiveFuture(const FutureBase& future);

private:

	/// Controls whether the object is currently enabled.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, isEnabled, setEnabled);

	/// The user-defined title of this object.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, title, setTitle);

	/// The current status of this object.
	DECLARE_RUNTIME_PROPERTY_FIELD_FLAGS(PipelineStatus, status, setStatus, PROPERTY_FIELD_NO_UNDO | PROPERTY_FIELD_NO_CHANGE_MESSAGE);

	/// Indicates how many running tasks are currently associated with this object.
	int _numberOfActiveTasks = 0;
};

}	// End of namespace
