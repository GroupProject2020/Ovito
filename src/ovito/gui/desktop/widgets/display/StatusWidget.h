////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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


#include <ovito/gui/desktop/GUI.h>
#include <ovito/core/dataset/pipeline/PipelineStatus.h>

namespace Ovito {

/**
 * \brief A widget that displays information from the PipelineStatus class.
 */
class OVITO_GUI_EXPORT StatusWidget : public QScrollArea
{
	Q_OBJECT

public:

	/// \brief Constructs the widget.
	/// \param parent The parent widget for the new widget.
	StatusWidget(QWidget* parent = nullptr);

	/// Returns the current status displayed by the widget.
	const PipelineStatus& status() const { return _status; }

	/// Sets the status to be displayed by the widget.
	void setStatus(const PipelineStatus& status);

	/// Resets the widget to not display any status.
	void clearStatus() {
		setStatus(PipelineStatus());
	}

	/// Returns the minimum size of the widget.
	virtual QSize minimumSizeHint() const override;

	/// Returns the preferred size of the widget.
	virtual QSize sizeHint() const override;

private:

	/// The current status displayed by the widget.
	PipelineStatus _status;

	/// The internal text label.
	QLabel* _textLabel;

	/// The internal icon label.
	QLabel* _iconLabel;
};

}	// End of namespace
