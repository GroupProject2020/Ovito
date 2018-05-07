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
#include <plugins/stdobj/properties/PropertyStorage.h>
#include <core/dataset/data/DataObject.h>

namespace Ovito { namespace StdObj {
	
/**
 * \brief Holds a data plot series.
 */
class OVITO_STDOBJ_EXPORT PlotObject : public DataObject
{
	Q_OBJECT
	OVITO_CLASS(PlotObject)
	
public:

	/// \brief Creates a plot object.
	PlotObject(DataSet* dataset);

	//////////////////////////////// from RefTarget //////////////////////////////

	/// Returns the display title of this property object in the user interface.
	virtual QString objectTitle() override;

protected:

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

private:

	/// The title of the plot.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, title, setTitle);

	/// The x coordinates of the data points.
	DECLARE_RUNTIME_PROPERTY_FIELD(PropertyPtr, x, setx);

	/// The y coordinates of the data points.
	DECLARE_RUNTIME_PROPERTY_FIELD(PropertyPtr, y, sety);
};

}	// End of namespace
}	// End of namespace
