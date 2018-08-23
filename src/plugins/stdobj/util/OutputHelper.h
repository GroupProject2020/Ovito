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


#include <plugins/stdobj/StdObj.h>
#include <core/dataset/pipeline/PipelineOutputHelper.h>

namespace Ovito { namespace StdObj {

/**
 * \brief Helper class that allows easy manipulation of properties.
 */
class OVITO_STDOBJ_EXPORT OutputHelper : public PipelineOutputHelper
{
public:

	/// Inherit constructor.
	using PipelineOutputHelper::PipelineOutputHelper;

	/// Creates a standard property in the modifier's output.
	/// If the property already exists in the input, its contents are copied to the
	/// output property by this method.
	PropertyObject* outputStandardProperty(const PropertyClass& propertyClass, int typeId, bool initializeMemory = false);

	/// Creates a standard property in the modifier's output.
	/// If the property already exists in the input, its contents are copied to the
	/// output property by this method.
	template<class PropertyObjectType>
	PropertyObjectType* outputStandardProperty(int typeId, bool initializeMemory = false) {
		return static_object_cast<PropertyObjectType>(outputStandardProperty(PropertyObjectType::OOClass(), typeId, initializeMemory));
	}

	/// Creates a custom property in the modifier's output.
	/// If the property already exists in the input, its contents are copied to the
	/// output property by this method.
	PropertyObject* outputCustomProperty(const PropertyClass& propertyClass, const QString& name, int dataType, size_t componentCount, size_t stride, bool initializeMemory = false);

	/// Creates a custom property in the modifier's output.
	/// If the property already exists in the input, its contents are copied to the
	/// output property by this method.
	template<class PropertyObjectType>
	PropertyObjectType* outputCustomProperty(const QString& name, int dataType, size_t componentCount, size_t stride, bool initializeMemory = false) {
		return static_object_cast<PropertyObjectType>(outputCustomProperty(PropertyObjectType::OOClass(), name, dataType, componentCount, stride, initializeMemory));
	}
	
	/// Creates a property in the modifier's output and sets its content.
	PropertyObject* outputProperty(const PropertyClass& propertyClass, const PropertyPtr& storage);

	/// Creates a property in the modifier's output and sets its content.
	template<class PropertyObjectType>
	PropertyObjectType* outputProperty(const PropertyPtr& storage) {
		return static_object_cast<PropertyObjectType>(outputProperty(PropertyObjectType::OOClass(), storage));
	}

	/// Outputs a new data series to the pipeline.
	DataSeriesObject* outputDataSeries(const QString& id, const QString& title, const PropertyPtr& y, const PropertyPtr& x = nullptr);
};

}	// End of namespace
}	// End of namespace
