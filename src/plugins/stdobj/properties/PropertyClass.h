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
#include <core/dataset/data/DataObject.h>
#include <core/dataset/pipeline/PipelineFlowState.h>

namespace Ovito { namespace StdObj {

/**
 * \brief A meta-class for properties (i.e. classes derived from PropertyObject).
 */
 class OVITO_STDOBJ_EXPORT PropertyClass : public DataObject::OOMetaClass 
{
public:

	/// Inherit standard constructor from base meta class.
	using DataObject::OOMetaClass::OOMetaClass;

	/// This helper method returns a standard property (if present) from the given pipeline state.
	PropertyObject* findInState(const PipelineFlowState& state, int typeId, const QString& bundleName = QString()) const;
	
	/// This helper method returns a specific user-defined property (if present) from the given pipeline state.
	PropertyObject* findInState(const PipelineFlowState& state, const QString& name, const QString& bundleName = QString()) const;

	/// Returns a human-readable name used for the property class in the user interface, e.g. 'Particles' or 'Bonds'.
	const QString& propertyClassDisplayName() const { return _propertyClassDisplayName; }

	/// Returns a human-readable name describing the data elements of this property class in the user interface, e.g. 'particles' or 'bonds'.
	const QString& elementDescriptionName() const { return _elementDescriptionName; }
	
	/// Returns the name by which this property class is referred to from Python scripts.
	const QString& pythonName() const { return _pythonName; }

	/// Returns the number of elements in a property for the given data state.
	virtual size_t elementCount(const PipelineFlowState& state) const { OVITO_ASSERT(false); return 0; }

	/// Determines if the data elements which this property class applies to are present for the given data state.
	virtual bool isDataPresent(const PipelineFlowState& state) const { OVITO_ASSERT(false); return false; }
	
	/// Creates a new property storage for one of the registered standard properties.
	virtual PropertyPtr createStandardStorage(size_t elementCount, int typeId, bool initializeMemory) const { return {}; }

	/// Returns the index of the data element that was picked in a viewport.
	virtual std::pair<size_t, PipelineFlowState> elementFromPickResult(const ViewportPickResult& pickResult) const { 
		return std::pair<size_t, PipelineFlowState>(std::numeric_limits<size_t>::max(), PipelineFlowState{});
	}

	/// Tries to remap an index from one data collection to another, considering the possibility that
	/// elements may have been added or removed. 
	virtual size_t remapElementIndex(const PipelineFlowState& sourceState, size_t elementIndex, const PipelineFlowState& destState) const {
		return std::numeric_limits<size_t>::max();
	}

	/// Determines which elements are located within the given viewport fence region (=2D polygon).
	virtual boost::dynamic_bitset<> viewportFenceSelection(const QVector<Point2>& fence, const PipelineFlowState& state, PipelineSceneNode* node, const Matrix4& projectionTM) const {
		return boost::dynamic_bitset<>{}; // Return empty set to indicate missing fence selection support.
	}
	
	/// Creates a new instace of the property object type.
	OORef<PropertyObject> createFromStorage(DataSet* dataset, const PropertyPtr& storage) const;

	/// Determines whether a standard property ID is defined for this property class.
	bool isValidStandardPropertyId(int id) const {
		return _standardPropertyNames.find(id) != _standardPropertyNames.end();
	}

	/// Returns the standard property type ID from a property name.
	int standardPropertyTypeId(const QString& name) const {
		auto iter = _standardPropertyIds.find(name);
		if(iter == _standardPropertyIds.end()) return 0;
		else return iter.value();
	}
	
	/// Returns the name of a standard property type.
	const QString& standardPropertyName(int typeId) const {
		OVITO_ASSERT(_standardPropertyNames.contains(typeId));
		return _standardPropertyNames.find(typeId).value();
	}
	
	/// Returns the display title used for a standard property type.
	const QString& standardPropertyTitle(int typeId) const {
		OVITO_ASSERT(_standardPropertyTitles.contains(typeId));
		return _standardPropertyTitles.find(typeId).value();
	}

	/// Returns the data type used by the given standard property type.
	int standardPropertyDataType(int typeId) const {
		OVITO_ASSERT(_standardPropertyDataTypes.contains(typeId));
		return _standardPropertyDataTypes.find(typeId).value();
	}
		
	/// Returns the number of vector components per element used by the given standard property type.
	size_t standardPropertyComponentCount(int typeId) const {
		OVITO_ASSERT(_standardPropertyComponents.contains(typeId));
		return std::max(_standardPropertyComponents.find(typeId).value().size(), 1);
	}

	/// Returns the list of component names for the given standard property type.
	const QStringList& standardPropertyComponentNames(int typeId) const {
		OVITO_ASSERT(_standardPropertyComponents.contains(typeId));
		return _standardPropertyComponents.find(typeId).value();
	}

	/// Returns the list of standard property type IDs.
	const QList<int>& standardProperties() const {
		return _standardPropertyList;
	}
	
	/// Returns the mapping from standard property names to standard property type IDs.
	const QMap<QString, int>& standardPropertyIds() const {
		return _standardPropertyIds;
	}

	/// The data object type that bundles a set of properties.
	OvitoClassPtr bundleObjectClass() const { return _bundleObjectClass; }

protected:
	
	/// Registers a new standard property with this property meta class.
	void registerStandardProperty(int typeId, QString name, int dataType, QStringList componentNames, QString title = QString());		
	
	/// Sets the human-readable name used for the property class in the user interface.
	void setPropertyClassDisplayName(const QString& name) { _propertyClassDisplayName = name; }

	/// Set the human-readable name describing the data elements of this property class in the user interface, e.g. 'particles' or 'bonds'.
	void setElementDescriptionName(const QString& name) { _elementDescriptionName = name; }
	
	/// Sets the name by which this property class is referred to from Python scripts.
	void setPythonName(const QString& name) { _pythonName = name; }
	
	/// Sets the data object type that bundles a set of properties of this class.
	void setBundleObjectClass(const DataObject::OOMetaClass& dataObjectClass) { 
		OVITO_ASSERT(dataObjectClass.isDerivedFrom(DataObject::OOClass()));
		_bundleObjectClass = &dataObjectClass; 
	}

	/// Gives the property class the opportunity to set up a newly created property object.
	virtual void prepareNewProperty(PropertyObject* property) const {}
	
private:

	/// The human-readable display name of this property class used in the user interface,
	/// e.g. 'Particles' or 'Bonds'.
	QString _propertyClassDisplayName;

	/// The name of the elements described by the properties of this class, e.g. 'particles' or 'bonds'.
	QString _elementDescriptionName;

	/// The name by which this property class is referred to from Python scripts.
	QString _pythonName;
	
	/// The list of standard property type IDs.
	QList<int> _standardPropertyList;

	/// Mapping from standard property names to standard property type IDs.
	QMap<QString, int> _standardPropertyIds;

	/// Mapping from standard property type ID to standard property names.
	QMap<int, QString> _standardPropertyNames;

	/// Mapping from standard property type ID to standard property title strings.
	QMap<int, QString> _standardPropertyTitles;

	/// Mapping from standard property type ID to property component names.
	QMap<int, QStringList> _standardPropertyComponents;

	/// Mapping from standard property type ID to property data type.
	QMap<int, int> _standardPropertyDataTypes;

	/// The data object type that bundles a set of properties.
	OvitoClassPtr _bundleObjectClass = nullptr;
};

}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::StdObj::PropertyClassPtr);
Q_DECLARE_TYPEINFO(Ovito::StdObj::PropertyClassPtr, Q_PRIMITIVE_TYPE);
