#######################################################################################
#
#  Copyright 2019 Alexander Stukowski
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

# Locate the OpenVR library.
FIND_PACKAGE(OpenVR REQUIRED)

# Define the plugin module.
OVITO_STANDARD_PLUGIN(VRPlugin
	SOURCES
		StartVRAction.cpp
		VRWindow.cpp
		VRRenderingWidget.cpp
		VRSceneRenderer.cpp
		#VRCacheModifier.cpp
		VRSettingsObject.cpp
		VRSettingsObjectEditor.cpp
	PRIVATE_LIB_DEPENDENCIES
		OpenVR::OpenVR
	GUI_PLUGIN
)

IF(WIN32)
	# Determine location of OpenVR DLL.
	LIST(GET OpenVR_LIBRARIES 0 OpenVR_FIRST_LIBRARY)
	GET_FILENAME_COMPONENT(OPENVR_LIBRARY_PATH "${OpenVR_FIRST_LIBRARY}" PATH)

	# Deploy OpenVR DLL.
	OVITO_INSTALL_SHARED_LIB("${OPENVR_LIBRARY_PATH}/../../bin/win64/openvr_api.dll" ".")
ENDIF()

# Propagate list of plugins to parent scope.
SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)