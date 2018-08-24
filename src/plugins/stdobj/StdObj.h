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

//
// Standard precompiled header file included by all source files in this module
//

#ifndef __OVITO_STDOBJ_
#define __OVITO_STDOBJ_

#include <core/Core.h>

namespace Ovito {
	namespace StdObj {

        class InputHelper;
        class OutputHelper;
        class PropertyObject;
        class PropertyStorage;
        class PropertyContainer;
        using PropertyPtr = std::shared_ptr<PropertyStorage>;
        using ConstPropertyPtr = std::shared_ptr<const PropertyStorage>;
        class PropertyClass;
        using PropertyClassPtr = const PropertyClass*;
        class PropertyReference;
        template<class PropertyObjectType> class TypedPropertyReference;
        class SimulationCell;
        class SimulationCellObject;
        class SimulationCellVis;
        class DataSeriesObject;
        class DataSeriesProperty;
    }
}

#endif
