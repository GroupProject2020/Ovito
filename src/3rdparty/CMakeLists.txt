#######################################################################################
#
#  Copyright 2019 Alexander Stukowski and Technische Universität Darmstadt
#
#  This file is part of OVITO (Open Visualization Tool).
#
#  OVITO is free software; you can redistribute it and/or modify it either under the
#  terms of the GNU General Public License version 3 as published by the Free Software
#  Foundation (the "GPL") or, at your option, under the terms of the MIT License.
#  If you do not alter this notice, a recipient may use your version of this
#  file under either the GPL or the MIT License.
#
#  You should have received a copy of the GPL along with this program in a
#  file LICENSE.GPL.txt.  You should have received a copy of the MIT License along
#  with this program in a file LICENSE.MIT.txt
#
#  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
#  either express or implied. See the GPL or the MIT License for the specific language
#  governing rights and limitations.
#
#######################################################################################

IF(OVITO_BUILD_GUI)
    ADD_SUBDIRECTORY(qwt)
ENDIF()
IF(OVITO_BUILD_PLUGIN_STDOBJ OR OVITO_BUILD_PLUGIN_PARTICLES)
    ADD_SUBDIRECTORY(muparser)
ENDIF()
IF(OVITO_BUILD_PLUGIN_PARTICLES)
    ADD_SUBDIRECTORY(voro++)
    ADD_SUBDIRECTORY(ptm)
    ADD_SUBDIRECTORY(copr)
ENDIF()
IF(OVITO_BUILD_PLUGIN_TACHYON)
	ADD_SUBDIRECTORY(tachyon)
ENDIF()
IF(OVITO_BUILD_PLUGIN_PARTICLES)
	ADD_SUBDIRECTORY(geogram)
ENDIF()
IF(OVITO_BUILD_PLUGIN_NETCDFPLUGIN OR OVITO_BUILD_PLUGIN_CRYSTALANALYSIS)
    ADD_SUBDIRECTORY(netcdf_integration)
ENDIF()
IF(OVITO_BUILD_PLUGIN_CORRELATION)
	ADD_SUBDIRECTORY(kissfft)
ENDIF()
