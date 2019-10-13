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


#include <ovito/core/Core.h>
#include <ovito/core/dataset/pipeline/AsynchronousModifier.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)


/**
 * \brief Base class for modifier delegates used by the AsynchronousDelegatingModifier class.
 */
class OVITO_CORE_EXPORT AsynchronousModifierDelegate : public RefTarget
{
public:

	/// Give asynchronous modifier delegates their own metaclass.
	class AsynchronousModifierDelegateClass : public RefTarget::OOMetaClass
	{
	public:

		/// Inherit constructor from base class.
		using RefTarget::OOMetaClass::OOMetaClass;

		/// Asks the metaclass which data objects in the given input data collection the modifier delegate can operate on.
		virtual QVector<DataObjectReference> getApplicableObjects(const DataCollection& input) const {
			OVITO_ASSERT_MSG(false, "AsynchronousModifierDelegate::OOMetaClass::getApplicableObjects()",
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
			OVITO_ASSERT_MSG(false, "AsynchronousModifierDelegate::OOMetaClass::getApplicableObjectClass()",
				qPrintable(QStringLiteral("Metaclass of modifier delegate class %1 does not override the getApplicableObjectClass() method.").arg(name())));
			return DataObject::OOClass();
		}

		/// \brief The name by which Python scripts can refer to this modifier delegate.
		virtual QString pythonDataName() const {
			OVITO_ASSERT_MSG(false, "AsynchronousModifierDelegate::OOMetaClass::pythonDataName()",
				qPrintable(QStringLiteral("Metaclass of modifier delegate class %1 does not override the pythonDataName() method.").arg(name())));
			return {};
		}
	};

	OVITO_CLASS_META(AsynchronousModifierDelegate, AsynchronousModifierDelegateClass)
	Q_OBJECT

protected:

	/// \brief Constructor.
	AsynchronousModifierDelegate(DataSet* dataset, const DataObjectReference& inputDataObj = DataObjectReference()) : RefTarget(dataset), _inputDataObject(inputDataObj) {}

public:

	/// \brief Returns the modifier to which this delegate belongs.
	AsynchronousDelegatingModifier* modifier() const;

private:

	/// Optionally specifies a particular input data object this delegate should operate on.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(DataObjectReference, inputDataObject, setInputDataObject);
};

/**
 * \brief Base class for modifiers that delegate work to a ModifierDelegate object.
 */
class OVITO_CORE_EXPORT AsynchronousDelegatingModifier : public AsynchronousModifier
{
public:

	/// The abstract base class of delegates used by this modifier type.
	using DelegateBaseType = AsynchronousModifierDelegate;

	/// Give this modifier class its own metaclass.
	class DelegatingModifierClass : public ModifierClass
	{
	public:

		/// Inherit constructor from base class.
		using ModifierClass::ModifierClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const DataCollection& input) const override;

		/// Return the metaclass of delegates for this modifier type.
		virtual const AsynchronousModifierDelegate::OOMetaClass& delegateMetaclass() const {
			OVITO_ASSERT_MSG(false, "AsynchronousDelegatingModifier::OOMetaClass::delegateMetaclass()",
				qPrintable(QStringLiteral("Delegating modifier class %1 does not define a corresponding delegate metaclass. "
				"You must override the delegateMetaclass() method in the modifier's metaclass.").arg(name())));
			return DelegateBaseType::OOClass();
		}
	};

	OVITO_CLASS_META(AsynchronousDelegatingModifier, DelegatingModifierClass)
	Q_OBJECT

public:

	/// Constructor.
	AsynchronousDelegatingModifier(DataSet* dataset);

protected:

	/// Creates a default delegate for this modifier.
	/// This should be called from the modifier's constructor.
	void createDefaultModifierDelegate(const OvitoClass& delegateType, const QString& defaultDelegateTypeName);

protected:

	/// The modifier's delegate.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(AsynchronousModifierDelegate, delegate, setDelegate, PROPERTY_FIELD_ALWAYS_CLONE);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
