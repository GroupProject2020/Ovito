////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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
#include <ovito/core/app/PluginManager.h>
#include <ovito/core/dataset/DataSet.h>
#include "DelegatingModifier.h"

namespace Ovito {

IMPLEMENT_OVITO_CLASS(ModifierDelegate);
DEFINE_PROPERTY_FIELD(ModifierDelegate, isEnabled);
DEFINE_PROPERTY_FIELD(ModifierDelegate, inputDataObject);
SET_PROPERTY_FIELD_LABEL(ModifierDelegate, isEnabled, "Enabled");
SET_PROPERTY_FIELD_LABEL(ModifierDelegate, inputDataObject, "Data object");

IMPLEMENT_OVITO_CLASS(DelegatingModifier);
DEFINE_REFERENCE_FIELD(DelegatingModifier, delegate);

IMPLEMENT_OVITO_CLASS(MultiDelegatingModifier);
DEFINE_REFERENCE_FIELD(MultiDelegatingModifier, delegates);

/******************************************************************************
* Returns the modifier to which this delegate belongs.
******************************************************************************/
Modifier* ModifierDelegate::modifier() const
{
	for(RefMaker* dependent : this->dependents()) {
		if(DelegatingModifier* modifier = dynamic_object_cast<DelegatingModifier>(dependent)) {
			if(modifier->delegate() == this) return modifier;
		}
		else if(MultiDelegatingModifier* modifier = dynamic_object_cast<MultiDelegatingModifier>(dependent)) {
			if(modifier->delegates().contains(const_cast<ModifierDelegate*>(this))) return modifier;
		}
	}
	return nullptr;
}

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
DelegatingModifier::DelegatingModifier(DataSet* dataset) : Modifier(dataset)
{
}

/******************************************************************************
* Determines the time interval over which a computed pipeline state will remain valid.
******************************************************************************/
TimeInterval DelegatingModifier::validityInterval(const PipelineEvaluationRequest& request, const ModifierApplication* modApp) const
{
	TimeInterval iv = Modifier::validityInterval(request, modApp);

	if(delegate() && delegate()->isEnabled())
		iv.intersect(delegate()->validityInterval(request, modApp));

	return iv;
}

/******************************************************************************
* Creates a default delegate for this modifier.
******************************************************************************/
void DelegatingModifier::createDefaultModifierDelegate(const OvitoClass& delegateType, const QString& defaultDelegateTypeName)
{
	OVITO_ASSERT(delegateType.isDerivedFrom(ModifierDelegate::OOClass()));

	// Find the delegate type that corresponds to the given name string.
	for(OvitoClassPtr clazz : PluginManager::instance().listClasses(delegateType)) {
		if(clazz->name() == defaultDelegateTypeName) {
			OORef<ModifierDelegate> delegate = static_object_cast<ModifierDelegate>(clazz->createInstance(dataset()));
			setDelegate(delegate);
			break;
		}
	}
	OVITO_ASSERT_MSG(delegate(), "DelegatingModifier::createDefaultModifierDelegate", qPrintable(QStringLiteral("There is no delegate class named '%1' inheriting from %2.").arg(defaultDelegateTypeName).arg(delegateType.name())));
}

/******************************************************************************
* Asks the metaclass whether the modifier can be applied to the given input data.
******************************************************************************/
bool DelegatingModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	if(!ModifierClass::isApplicableTo(input)) return false;

	// Check if there is any modifier delegate that could handle the input data.
	for(const ModifierDelegate::OOMetaClass* clazz : PluginManager::instance().metaclassMembers<ModifierDelegate>(delegateMetaclass())) {
		if(clazz->getApplicableObjects(input).empty() == false)
			return true;
	}
	return false;
}

/******************************************************************************
* Modifies the input data synchronously.
******************************************************************************/
void DelegatingModifier::evaluateSynchronous(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	// Apply the modifier delegate to the input data.
	applyDelegate(state, time, modApp);
}

/******************************************************************************
* Lets the modifier's delegate operate on a pipeline flow state.
******************************************************************************/
void DelegatingModifier::applyDelegate(PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
{
	OVITO_ASSERT(!dataset()->undoStack().isRecording());

	if(!delegate() || !delegate()->isEnabled())
		return;

	// Skip function if not applicable.
	if(delegate()->getOOMetaClass().getApplicableObjects(state).empty())
		throwException(tr("The modifier's pipeline input does not contain the expected kind of data."));

	// Call the delegate function.
	PipelineStatus delegateStatus = delegate()->apply(this, state, time, modApp, additionalInputs);

	// Append status text and code returned by the delegate function to the status returned to our caller.
	PipelineStatus status = state.status();
	if(status.type() == PipelineStatus::Success || delegateStatus.type() == PipelineStatus::Error)
		status.setType(delegateStatus.type());
	if(!delegateStatus.text().isEmpty()) {
		if(!status.text().isEmpty())
			status.setText(status.text() + QStringLiteral("\n") + delegateStatus.text());
		else
			status.setText(delegateStatus.text());
	}
	state.setStatus(std::move(status));
}

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
MultiDelegatingModifier::MultiDelegatingModifier(DataSet* dataset) : Modifier(dataset)
{
}

/******************************************************************************
* Determines the time interval over which a computed pipeline state will remain valid.
******************************************************************************/
TimeInterval MultiDelegatingModifier::validityInterval(const PipelineEvaluationRequest& request, const ModifierApplication* modApp) const
{
	TimeInterval iv = Modifier::validityInterval(request, modApp);

	for(const ModifierDelegate* delegate : delegates()) {
		if(delegate->isEnabled()) {
			iv.intersect(delegate->validityInterval(request, modApp));
		}
	}

	return iv;
}

/******************************************************************************
* Creates the list of delegate objects for this modifier.
******************************************************************************/
void MultiDelegatingModifier::createModifierDelegates(const OvitoClass& delegateType)
{
	OVITO_ASSERT(delegateType.isDerivedFrom(ModifierDelegate::OOClass()));

	// Generate the list of delegate objects.
	for(OvitoClassPtr clazz : PluginManager::instance().listClasses(delegateType)) {
		_delegates.push_back(this, PROPERTY_FIELD(delegates), static_object_cast<ModifierDelegate>(clazz->createInstance(dataset())));
	}
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool MultiDelegatingModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	if(!ModifierClass::isApplicableTo(input)) return false;

	// Check if there is any modifier delegate that could handle the input data.
	for(const ModifierDelegate::OOMetaClass* clazz : PluginManager::instance().metaclassMembers<ModifierDelegate>(delegateMetaclass())) {
		if(clazz->getApplicableObjects(input).empty() == false)
			return true;
	}
	return false;
}

/******************************************************************************
* Modifies the input data synchronously.
******************************************************************************/
void MultiDelegatingModifier::evaluateSynchronous(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	// Apply all enabled modifier delegates to the input data.
	applyDelegates(state, time, modApp);
}

/******************************************************************************
* Lets the registered modifier delegates operate on a pipeline flow state.
******************************************************************************/
void MultiDelegatingModifier::applyDelegates(PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
{
	OVITO_ASSERT(!dataset()->undoStack().isRecording());

	for(ModifierDelegate* delegate : delegates()) {

		// Skip function if not applicable.
		if(!state.data() || !delegate->isEnabled() || delegate->getOOMetaClass().getApplicableObjects(*state.data()).empty())
			continue;

		// Call the delegate function.
		PipelineStatus delegateStatus = delegate->apply(this, state, time, modApp, additionalInputs);

		// Append status text and code returned by the delegate function to the status returned to our caller.
		PipelineStatus status = state.status();
		if(status.type() == PipelineStatus::Success || delegateStatus.type() == PipelineStatus::Error)
			status.setType(delegateStatus.type());
		if(!delegateStatus.text().isEmpty()) {
			if(!status.text().isEmpty())
				status.setText(status.text() + QStringLiteral("\n") + delegateStatus.text());
			else
				status.setText(delegateStatus.text());
		}
		state.setStatus(std::move(status));
	}
}

}	// End of namespace
