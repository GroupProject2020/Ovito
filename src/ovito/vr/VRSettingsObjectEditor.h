////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify it either under the
//  terms of the GNU General Public License version 3 as published by the Free Software
//  Foundation (the "GPL") or, at your option, under the terms of the MIT License.
//  If you do not alter this notice, a recipient may use your version of this
//  file under either the GPL or the MIT License.
//
//  You should have received a copy of the GPL along with this program in a
//  file LICENSE.GPL.txt.  You should have received a copy of the MIT License along
//  with this program in a file LICENSE.MIT.txt
//
//  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
//  either express or implied. See the GPL or the MIT License for the specific language
//  governing rights and limitations.
//
////////////////////////////////////////////////////////////////////////////////////////

#pragma once


#include <ovito/gui/GUI.h>
#include <ovito/gui/properties/PropertiesEditor.h>
#include <ovito/core/viewport/ViewportConfiguration.h>

namespace VRPlugin {

using namespace Ovito;
/*
 * \brief The UI component for the VRSettingsObject class.
 */
class VRSettingsObjectEditor : public PropertiesEditor
{
	Q_OBJECT
	OVITO_CLASS(VRSettingsObjectEditor)

public:

	/// Default constructor.
	Q_INVOKABLE VRSettingsObjectEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

private Q_SLOTS:

	/// Disables rendering of the normal viewports.
	void disableViewportRendering(bool disable) {
		_viewportSuspender.reset(disable ? new ViewportSuspender(dataset()) : nullptr);
	}

private:

	/// Used to disable viewport rendering.
	std::unique_ptr<ViewportSuspender> _viewportSuspender;
};

}	// End of namespace
