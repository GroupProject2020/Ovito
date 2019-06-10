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
#include <plugins/stdobj/properties/PropertyContainer.h>
#include <plugins/stdobj/properties/PropertyReference.h>

namespace Ovito { namespace StdObj {

/**
 * \brief Data object that holds a series of data values for 2d plots.
 */
class OVITO_STDOBJ_EXPORT DataSeriesObject : public PropertyContainer
{
	/// Define a new property metaclass for data series property containers.
	class DataSeriesObjectClass : public PropertyContainerClass
	{
	public:

		/// Inherit constructor from base class.
		using PropertyContainerClass::PropertyContainerClass;

		/// Creates a storage object for standard data series properties.
		virtual PropertyPtr createStandardStorage(size_t elementCount, int type, bool initializeMemory, const ConstDataObjectPath& containerPath = {}) const override;

	protected:

		/// Is called by the system after construction of the meta-class instance.
		virtual void initialize() override;
	};

	Q_OBJECT
	OVITO_CLASS_META(DataSeriesObject, DataSeriesObjectClass);
	Q_CLASSINFO("DisplayName", "Data series");

public:

	/// \brief The list of standard data series properties.
	enum Type {
		UserProperty = PropertyStorage::GenericUserProperty,	//< This is reserved for user-defined properties.
		XProperty = PropertyStorage::FirstSpecificProperty,
		YProperty
	};

	enum PlotMode {
		None,
		Line,
		Histogram,
		BarChart,
		Scatter
	};
	Q_ENUMS(PlotMode);

	/// Constructor.
	Q_INVOKABLE DataSeriesObject(DataSet* dataset, PlotMode plotMode = Line, const QString& title = QString(), const PropertyPtr& y = nullptr, const PropertyPtr& x = nullptr);

	/// Returns the property object containing the y-coordinates of the data points.
	const PropertyObject* getY() const { return getProperty(Type::YProperty); }

	/// Returns the property object containing the x-coordinates of the data points (may be NULL).
	const PropertyObject* getX() const { return getProperty(Type::XProperty); }

	/// Returns the data array containing the y-coordinates of the data points.
	ConstPropertyPtr getYStorage() const { return getPropertyStorage(Type::YProperty); }

	/// Returns the data array containing the x-coordinates of the data points.
	/// If no explicit x-coordinate data is available, the array is dynamically generated
	/// from the x-axis interval set for this data series.
	ConstPropertyPtr getXStorage() const;

	//////////////////////////////// from RefTarget //////////////////////////////

	/// Returns the display title of this object in the user interface.
	virtual QString objectTitle() const override;

private:

	/// The title of the data series, which is shown in the user interface.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, title, setTitle);

	/// The lower bound of the x-interval of the histogram if data points have no explicit x-coordinates.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, intervalStart, setIntervalStart);

	/// The upper bound of the x-interval of the histogram if data points have no explicit x-coordinates.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, intervalEnd, setIntervalEnd);

	/// The label of the x-axis (optional).
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, axisLabelX, setAxisLabelX);

	/// The label of the y-axis (optional).
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, axisLabelY, setAxisLabelY);

	/// The data plotting mode for this data series.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(PlotMode, plotMode, setPlotMode);
};

/**
 * Encapsulates a reference to a data series property.
 */
using DataSeriesPropertyReference = TypedPropertyReference<DataSeriesObject>;

}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::StdObj::DataSeriesPropertyReference);
Q_DECLARE_METATYPE(Ovito::StdObj::DataSeriesObject::PlotMode);
Q_DECLARE_TYPEINFO(Ovito::StdObj::DataSeriesObject::PlotMode, Q_PRIMITIVE_TYPE);
