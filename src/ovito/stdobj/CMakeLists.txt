###############################################################################
#
#  Copyright (2019) Alexander Stukowski
#
#  This file is part of OVITO (Open Visualization Tool).
#
#  OVITO is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  OVITO is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
###############################################################################

# Define the plugin module.
OVITO_STANDARD_PLUGIN(StdObj
	SOURCES
		StdObj.cpp
		camera/CameraObject.cpp
		camera/TargetObject.cpp
		simcell/SimulationCellObject.cpp
		simcell/SimulationCellVis.cpp
		simcell/PeriodicDomainDataObject.cpp
		properties/PropertyStorage.cpp
		properties/PropertyObject.cpp
		properties/PropertyReference.cpp
		properties/PropertyContainerClass.cpp
		properties/PropertyContainer.cpp
		properties/ElementType.cpp
		properties/GenericPropertyModifier.cpp
		properties/PropertyExpressionEvaluator.cpp
		series/DataSeriesObject.cpp
		io/DataSeriesExporter.cpp
		util/ElementSelectionSet.cpp
	LIB_DEPENDENCIES
		muParser
)

# Speed up compilation by using precompiled headers.
IF(OVITO_USE_PRECOMPILED_HEADERS)
	ADD_PRECOMPILED_HEADER(StdObj ovito/stdobj/StdObj.h)
ENDIF()

# Build corresponding GUI plugin.
IF(OVITO_BUILD_GUI)
	ADD_SUBDIRECTORY(gui)
ENDIF()

# Build corresponding Python binding plugin.
IF(OVITO_BUILD_PLUGIN_PYSCRIPT)
	ADD_SUBDIRECTORY(scripting)
ENDIF()

# Propagate list of plugins to parent scope.
SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)
