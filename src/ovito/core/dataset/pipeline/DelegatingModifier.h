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

#pragma once


#include <ovito/core/Core.h>
#include <ovito/core/dataset/pipeline/Modifier.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)


/**
 * \brief Base class for modifier delegates used by the MultiDelegatingModifier class.
 */
class OVITO_CORE_EXPORT ModifierDelegate : public RefTarget
{
public:

	/// Give modifier delegates their own metaclass.
	class OVITO_CORE_EXPORT ModifierDelegateClass : public RefTarget::OOMetaClass
	{
	public:

		/// Inherit constructor from base class.
		using RefTarget::OOMetaClass::OOMetaClass;

		/// Indicates which data objects in the given input data collection the modifier delegate is able to operate on.
		virtual QVector<DataObjectReference> getApplicableObjects(const DataCollection& input) const {
			OVITO_ASSERT_MSG(false, "ModifierDelegate::OOMetaClass::getApplicableObjects()",
				qPrintable(QStringLiteral("Metaclass of modifier delegate class %1 does not override the getApplicableObjects() method.").arg(name())));
			return {};
		}

		/// Asks the metaclass which data objects in the given input pipeline state the modifier delegate can operate on.
		QVector<DataObjectReference> getApplicableObjects(const PipelineFlowState& input) const {
			if(input.isEmpty()) return {};
			return getApplicableObjects(*input.data());
		}

		/// Indicates which class of data objects the modifier delegate is able to operate on.
		virtual const DataObject::OOMetaClass& getApplicableObjectClass() const {
			OVITO_ASSERT_MSG(false, "ModifierDelegate::OOMetaClass::getApplicableObjectClass()",
				qPrintable(QStringLiteral("Metaclass of modifier delegate class %1 does not override the getApplicableObjectClass() method.").arg(name())));
			return DataObject::OOClass();
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
	ModifierDelegate(DataSet* dataset, const DataObjectReference& inputDataObj = DataObjectReference()) : RefTarget(dataset), _isEnabled(true), _inputDataObject(inputDataObj) {}

public:

	/// \brief Applies the modifier operation to the data in a pipeline flow state.
	virtual PipelineStatus apply(Modifier* modifier, PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs) = 0;

	/// \brief Returns the modifier owning this delegate.
	Modifier* modifier() const;

private:

	/// Indicates whether this delegate is active or not.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, isEnabled, setEnabled);

	/// Optionally specifies a particular input data object this delegate should operate on.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(DataObjectReference, inputDataObject, setInputDataObject);
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
	class OVITO_CORE_EXPORT DelegatingModifierClass : public ModifierClass
	{
	public:

		/// Inherit constructor from base class.
		using ModifierClass::ModifierClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const DataCollection& input) const override;

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
	virtual void evaluatePreliminary(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

protected:

	/// Creates a default delegate for this modifier.
	/// This should be called from the modifier's constructor.
	void createDefaultModifierDelegate(const OvitoClass& delegateType, const QString& defaultDelegateTypeName);

	/// Lets the modifier's delegate operate on a pipeline flow state.
	void applyDelegate(PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs = {});

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
	class OVITO_CORE_EXPORT MultiDelegatingModifierClass : public ModifierClass
	{
	public:

		/// Inherit constructor from base class.
		using ModifierClass::ModifierClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const DataCollection& input) const override;

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
	virtual void evaluatePreliminary(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

protected:

	/// Creates the list of delegate objects for this modifier.
	/// This should be called from the modifier's constructor.
	void createModifierDelegates(const OvitoClass& delegateType);

	/// Lets the registered modifier delegates operate on a pipeline flow state.
	void applyDelegates(PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs = {});

protected:

	/// List of modifier delegates.
	DECLARE_VECTOR_REFERENCE_FIELD_FLAGS(ModifierDelegate, delegates, PROPERTY_FIELD_ALWAYS_CLONE);
};


OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
