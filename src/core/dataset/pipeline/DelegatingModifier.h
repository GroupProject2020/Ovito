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

#pragma once


#include <core/Core.h>
#include <core/dataset/pipeline/Modifier.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)


/**
 * \brief Base class for modifier delegates used by the MultiDelegatingModifier class.
 */
class OVITO_CORE_EXPORT ModifierDelegate : public RefTarget
{
public:

	/// Give modifier delegates their own metaclass.
	class ModifierDelegateClass : public RefTarget::OOMetaClass 
	{
	public:

		/// Inherit constructor from base class.
		using RefTarget::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier delegate can operate on the given input data.
		virtual bool isApplicableTo(const PipelineFlowState& input) const { 
			OVITO_ASSERT_MSG(false, "ModifierDelegate::OOMetaClass::isApplicableTo()",
				qPrintable(QStringLiteral("Metaclass of modifier delegate class %1 does not override the isApplicableTo() method.").arg(name()))); 
			return false;
		}

		/// \brief The name by which Python scripts can refer to this modifier delegate.
		virtual QString pythonDataName() const {
			OVITO_ASSERT_MSG(false, "ModifierDelegate::OOMetaClass::pythonDataName()",
				qPrintable(QStringLiteral("Metaclass of modifier delegate class %1 does not override the pythonDataName() method.").arg(name()))); 
			return {};
		}
	};

	OVITO_CLASS_META(ModifierDelegate, ModifierDelegateClass)
	Q_OBJECT
	
protected:

	/// \brief Constructor.
	ModifierDelegate(DataSet* dataset) : RefTarget(dataset), _isEnabled(true) {}

public:

	/// \brief Applies the modifier operation to the data in a pipeline flow state.
	virtual PipelineStatus apply(Modifier* modifier, const PipelineFlowState& input, PipelineFlowState& output, TimePoint time, ModifierApplication* modApp) = 0;

	/// \brief Returns the modifier to which this delegate belongs.
	Modifier* modifier() const;

private:

	/// Indicates whether this delegate is active or not.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, isEnabled, setEnabled);
};

/**
 * \brief Base class for modifiers that delegate work to a ModifierDelegate object.
 */
class OVITO_CORE_EXPORT DelegatingModifier : public Modifier
{
public:

	/// The abstract base class of delegates used by this modifier type.
	using DelegateBaseType = ModifierDelegate;

	/// Give this modifier class its own metaclass.
	class DelegatingModifierClass : public ModifierClass 
	{
	public:

		/// Inherit constructor from base class.
		using ModifierClass::ModifierClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const PipelineFlowState& input) const override;

		/// Return the metaclass of delegates for this modifier type. 
		virtual const ModifierDelegate::OOMetaClass& delegateMetaclass() const { 
			OVITO_ASSERT_MSG(false, "DelegatingModifier::OOMetaClass::delegateMetaclass()",
				qPrintable(QStringLiteral("Delegating modifier class %1 does not define a corresponding delegate metaclass. "
				"You must override the delegateMetaclass() method in the modifier's metaclass.").arg(name()))); 
			return DelegateBaseType::OOClass();
		}
	};
		
	OVITO_CLASS_META(DelegatingModifier, DelegatingModifierClass)
	Q_OBJECT
	
public:

	/// Constructor.
	DelegatingModifier(DataSet* dataset);

	/// Modifies the input data in an immediate, preliminary way.
	virtual PipelineFlowState evaluatePreliminary(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

protected:

	/// Creates a default delegate for this modifier.
	/// This should be called from the modifier's constructor.
	void createDefaultModifierDelegate(const OvitoClass& delegateType, const QString& defaultDelegateTypeName);

	/// Lets the modifier's delegate operate on a pipeline flow state.
	void applyDelegate(const PipelineFlowState& input, PipelineFlowState& output, TimePoint time, ModifierApplication* modApp);

protected:

	/// The modifier delegate.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(ModifierDelegate, delegate, setDelegate, PROPERTY_FIELD_ALWAYS_CLONE);
};

/**
 * \brief Base class for modifiers that delegate work to a set of ModifierDelegate objects.
 */
class OVITO_CORE_EXPORT MultiDelegatingModifier : public Modifier
{
public:
	
	/// Give this modifier class its own metaclass.
	class MultiDelegatingModifierClass : public ModifierClass 
	{
	public:

		/// Inherit constructor from base class.
		using ModifierClass::ModifierClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const PipelineFlowState& input) const override;

		/// Return the metaclass of delegates for this modifier type. 
		virtual const ModifierDelegate::OOMetaClass& delegateMetaclass() const { 
			OVITO_ASSERT_MSG(false, "MultiDelegatingModifier::OOMetaClass::delegateMetaclass()",
				qPrintable(QStringLiteral("Multi-delegating modifier class %1 does not define a corresponding delegate metaclass. "
					"You must override the delegateMetaclass() method in the modifier's metaclass.").arg(name()))); 
			return ModifierDelegate::OOClass(); 
		}
	};
		
	OVITO_CLASS_META(MultiDelegatingModifier, MultiDelegatingModifierClass)
	Q_OBJECT
	
public:

	/// Constructor.
	MultiDelegatingModifier(DataSet* dataset);

	/// Modifies the input data in an immediate, preliminary way.
	virtual PipelineFlowState evaluatePreliminary(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

protected:

	/// Creates the list of delegate objects for this modifier.
	/// This should be called from the modifier's constructor.
	void createModifierDelegates(const OvitoClass& delegateType);

	/// Lets the registered modifier delegates operate on a pipeline flow state.
	void applyDelegates(const PipelineFlowState& input, PipelineFlowState& output, TimePoint time, ModifierApplication* modApp);

protected:

	/// List of modifier delegates.
	DECLARE_VECTOR_REFERENCE_FIELD_FLAGS(ModifierDelegate, delegates, PROPERTY_FIELD_ALWAYS_CLONE);
};


OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
