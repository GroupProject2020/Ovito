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

#pragma once


#include <plugins/stdobj/StdObj.h>
#include <plugins/stdobj/properties/PropertyObject.h>
#include <plugins/stdobj/properties/PropertyClass.h>
#include <plugins/stdobj/properties/PropertyReference.h>

namespace Ovito { namespace StdObj {

/**
 * \brief Property type for data series.
 */
class OVITO_STDOBJ_EXPORT DataSeriesProperty : public PropertyObject
{
public:

	/// Define a new property metaclass for data series properties.
	class OVITO_STDOBJ_EXPORT DataSeriesPropertyClass : public PropertyClass 
	{
	public:

		/// Inherit constructor from base class.
		using PropertyClass::PropertyClass;

		/// Creates a storage object for standard data series properties.
		virtual PropertyPtr createStandardStorage(size_t elementCount, int type, bool initializeMemory) const override;

		/// Returns the number of elements in a property for the given data state.
		virtual size_t elementCount(const PipelineFlowState& state) const override;

		/// Determines if the data elements which this property class applies to are present for the given data state.
		virtual bool isDataPresent(const PipelineFlowState& state) const override;
	
	protected:

		/// Is called by the system after construction of the meta-class instance.
		virtual void initialize() override;
	};

	Q_OBJECT
	OVITO_CLASS_META(DataSeriesProperty, DataSeriesPropertyClass)
	
public:

	/// \brief The list of standard data series properties.
	enum Type {
		UserProperty = PropertyStorage::GenericUserProperty,	//< This is reserved for user-defined properties.
//		SelectionProperty = PropertyStorage::GenericSelectionProperty,
//		ColorProperty = PropertyStorage::GenericColorProperty,
		XProperty = PropertyStorage::FirstSpecificProperty,
		YProperty
	};

	/// \brief Creates a data series property object.
	Q_INVOKABLE DataSeriesProperty(DataSet* dataset) : PropertyObject(dataset) {}

	/// \brief Returns the type of this property.
	Type type() const { return static_cast<Type>(PropertyObject::type()); }

	/// This helper method returns a standard data series property (if present) from the given pipeline state.
	static DataSeriesProperty* findInState(const PipelineFlowState& state, DataSeriesProperty::Type type, const QString& bundleName) {
		return static_object_cast<DataSeriesProperty>(OOClass().findInState(state, type, bundleName));
	}

	/// Creates a new instace of the property object type.
	static OORef<DataSeriesProperty> createFromStorage(DataSet* dataset, const PropertyPtr& storage) {
		return static_object_cast<DataSeriesProperty>(OOClass().createFromStorage(dataset, storage));
	}
};

/**
 * Encapsulates a reference to a data series property. 
 */
using DataSeriesPropertyReference = TypedPropertyReference<DataSeriesProperty>;

}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::StdObj::DataSeriesPropertyReference);
