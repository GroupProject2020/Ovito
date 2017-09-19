///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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
#include <core/app/PluginManager.h>
#include <core/dataset/DataSet.h>
#include "DelegatingModifier.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

IMPLEMENT_OVITO_CLASS(ModifierDelegate);
DEFINE_PROPERTY_FIELD(ModifierDelegate, isEnabled);	
SET_PROPERTY_FIELD_LABEL(ModifierDelegate, isEnabled, "Enabled");

IMPLEMENT_OVITO_CLASS(DelegatingModifier);
DEFINE_REFERENCE_FIELD(DelegatingModifier, delegate);	

IMPLEMENT_OVITO_CLASS(MultiDelegatingModifier);
DEFINE_REFERENCE_FIELD(MultiDelegatingModifier, delegates);	

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
DelegatingModifier::DelegatingModifier(DataSet* dataset) : Modifier(dataset)
{
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
bool DelegatingModifier::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	if(!ModifierClass::isApplicableTo(input)) return false;

	// Check if there is any modifier delegate that could handle the input data.
	for(const ModifierDelegate::OOMetaClass* clazz : PluginManager::instance().metaclassMembers<ModifierDelegate>(delegateMetaclass())) {
		if(clazz->isApplicableTo(input))
			return true;
	}
	return false;
}

/******************************************************************************
* Modifies the input data in an immediate, preliminary way.
******************************************************************************/
PipelineFlowState DelegatingModifier::evaluatePreliminary(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	PipelineFlowState output = input;

	// Apply the modifier delegate to the input data.
	applyDelegate(input, output, time, modApp);
	
	return output;
}

/******************************************************************************
* Lets the modifier's delegate operate on a pipeline flow state.
******************************************************************************/
void DelegatingModifier::applyDelegate(const PipelineFlowState& input, PipelineFlowState& output, TimePoint time, ModifierApplication* modApp)
{
	if(!delegate() || !delegate()->isEnabled())
		return;

	// Skip function if not applicable.
	if(!delegate()->getOOMetaClass().isApplicableTo(input))
		throwException(tr("The modifier input does not contain the expected kind of data."));

	// Call the delegate function.
	PipelineStatus delegateStatus = delegate()->apply(this, input, output, time, modApp);

	// Append status text and code returned by the delegate function to the status returned to our caller.
	PipelineStatus status = output.status();
	if(status.type() == PipelineStatus::Success || delegateStatus.type() == PipelineStatus::Error)
		status.setType(delegateStatus.type());
	if(delegateStatus.text().isEmpty() == false) {
		if(status.text().isEmpty() == false)
			status.setText(status.text() + QStringLiteral("\n") + delegateStatus.text());
		else
			status.setText(delegateStatus.text());
	}
	output.setStatus(std::move(status));
}

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
MultiDelegatingModifier::MultiDelegatingModifier(DataSet* dataset) : Modifier(dataset)
{
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
bool MultiDelegatingModifier::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	if(!ModifierClass::isApplicableTo(input)) return false;

	// Check if there is any modifier delegate that could handle the input data.
	for(const ModifierDelegate::OOMetaClass* clazz : PluginManager::instance().metaclassMembers<ModifierDelegate>(delegateMetaclass())) {
		if(clazz->isApplicableTo(input))
			return true;
	}
	return false;
}

/******************************************************************************
* Modifies the input data in an immediate, preliminary way.
******************************************************************************/
PipelineFlowState MultiDelegatingModifier::evaluatePreliminary(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	PipelineFlowState output = input;

	// Apply all enabled modifier delegates to the input data.
	applyDelegates(input, output, time, modApp);
	
	return output;
}

/******************************************************************************
* Lets the registered modifier delegates operate on a pipeline flow state.
******************************************************************************/
void MultiDelegatingModifier::applyDelegates(const PipelineFlowState& input, PipelineFlowState& output, TimePoint time, ModifierApplication* modApp)
{
	for(ModifierDelegate* delegate : delegates()) {

		// Skip function if not applicable.
		if(!delegate->isEnabled() || !delegate->getOOMetaClass().isApplicableTo(input))
			continue;

		// Call the delegate function.
		PipelineStatus delegateStatus = delegate->apply(this, input, output, time, modApp);

		// Append status text and code returned by the delegate function to the status returned to our caller.
		PipelineStatus status = output.status();
		if(status.type() == PipelineStatus::Success || delegateStatus.type() == PipelineStatus::Error)
			status.setType(delegateStatus.type());
		if(delegateStatus.text().isEmpty() == false) {
			if(status.text().isEmpty() == false)
				status.setText(status.text() + QStringLiteral("\n") + delegateStatus.text());
			else
				status.setText(delegateStatus.text());
		}
		output.setStatus(std::move(status));
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
