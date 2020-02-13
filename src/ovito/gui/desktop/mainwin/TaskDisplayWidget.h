////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2016 Alexander Stukowski
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

namespace Ovito {

/**
 * \brief Displays the running tasks in the status bar of the main window.
 */
class TaskDisplayWidget : public QWidget
{
	Q_OBJECT

public:

	/// Constructs the widget and associates it with the main window.
	TaskDisplayWidget(MainWindow* mainWindow);

public Q_SLOTS:

	/// \brief Shows the progress indicator widgets.
	void showIndicator();

	/// \brief Updates the displayed information in the indicator widget.
	void updateIndicator();

private Q_SLOTS:

	/// \brief Is called when a task has started to run.
	void taskStarted(TaskWatcher* taskWatcher);

	/// \brief Is called when a task has finished.
	void taskFinished(TaskWatcher* taskWatcher);

	/// \brief Is called when the progress or status of a task has changed.
	void taskProgressChanged();

private:

	/// The window this display widget is associated with.
	MainWindow* _mainWindow;

	/// The progress bar widget.
	QProgressBar* _progressBar;

#if 0
	/// The button that lets the user cancel running tasks.
	QAbstractButton* _cancelTaskButton;
#endif

	/// The label that displays the current progress text.
	QLabel* _progressTextDisplay;
};

}	// End of namespace


