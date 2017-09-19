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
#include <core/dataset/data/DataObject.h>
#include <core/dataset/pipeline/PipelineFlowState.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)
	
/**
 * \brief Helper class that allows easy access to properties.
 */
class OVITO_CORE_EXPORT InputHelper
{
public:

	/// Constructor.
	InputHelper(DataSet* dataset, const PipelineFlowState& input) : _dataset(dataset), _input(input) {}

	/// Returns a standard property from the input state.
	/// The returned property may be NULL if it does not exist.
	PropertyObject* inputStandardProperty(const PropertyClass& propertyClass, int typeId) const;

 	/// Returns a standard property from the input state.
	/// The returned property may be NULL if it does not exist.
	template<class PropertyObjectType>
	PropertyObjectType* inputStandardProperty(int typeId) const {
		return static_object_cast<PropertyObjectType>(inputStandardProperty(PropertyObjectType::OOClass(), typeId));
	}

	/// Returns the given standard property from the input object.
	/// The returned property may not be modified. If the input object does
	/// not contain the standard property then an exception is thrown.
	PropertyObject* expectStandardProperty(const PropertyClass& propertyClass, int typeId) const;

	/// Returns the given standard property from the input object.
	/// The returned property may not be modified. If the input object does
	/// not contain the standard property then an exception is thrown.
	template<class PropertyObjectType>
	PropertyObjectType* expectStandardProperty(int typeId) const {
		return static_object_cast<PropertyObjectType>(expectStandardProperty(PropertyObjectType::OOClass(), typeId));
	}
	
	/// Returns the property with the given name from the input state.
	/// The returned property may not be modified. If the input object does
	/// not contain a property with the given name and data type, then an exception is thrown.
	PropertyObject* expectCustomProperty(const PropertyClass& propertyClass, const QString& propertyName, int dataType, size_t componentCount = 1) const;

	/// Returns the property with the given name from the input state.
	/// The returned property may not be modified. If the input object does
	/// not contain a property with the given name and data type, then an exception is thrown.
	template<class PropertyObjectType>
	PropertyObjectType* expectCustomProperty(const QString& propertyName, int dataType, size_t componentCount = 1) const {
		return static_object_cast<PropertyObjectType>(expectCustomProperty(PropertyObjectType::OOClass(), propertyName, dataType, componentCount));
	}
	
	/// Returns the input simulation cell.
	/// The returned object may not be modified. If the input does
	/// not contain a simulation cell, an exception is thrown.
	SimulationCellObject* expectSimulationCell() const;

	/// Returns a reference to the input state.
	const PipelineFlowState& input() const { return _input; }

	/// Return the DataSet that provides a context for all performed operations.
	DataSet* dataset() const { return _dataset; }

private:

	/// The context data set.
	DataSet* _dataset;

	/// The input state.
	const PipelineFlowState& _input;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
