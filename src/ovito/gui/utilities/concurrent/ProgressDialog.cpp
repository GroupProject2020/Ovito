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

#include <ovito/gui/GUI.h>
#include <ovito/gui/mainwin/MainWindow.h>
#include <ovito/gui/widgets/general/ElidedTextLabel.h>
#include "ProgressDialog.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

/******************************************************************************
* Initializes the dialog window.
******************************************************************************/
ProgressDialog::ProgressDialog(MainWindow* mainWindow, const QString& dialogTitle) : ProgressDialog(mainWindow, mainWindow->datasetContainer().taskManager(), dialogTitle)
{
}

/******************************************************************************
* Initializes the dialog window.
******************************************************************************/
ProgressDialog::ProgressDialog(QWidget* parent, TaskManager& taskManager, const QString& dialogTitle) : QDialog(parent), _taskManager(taskManager)
{
	setWindowModality(Qt::WindowModal);
	setWindowTitle(dialogTitle);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addStretch(1);

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
	layout->addWidget(buttonBox);

	// Cancel all currently running tasks when user presses the cancel button.
	connect(buttonBox, &QDialogButtonBox::rejected, &taskManager, &TaskManager::cancelAll);

	// Helper function that sets up the UI widgets in the dialog for a newly started task.
	auto createUIForTask = [this, layout](TaskWatcher* taskWatcher) {
		QLabel* statusLabel = new QLabel(taskWatcher->progressText());
		statusLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
		QProgressBar* progressBar = new QProgressBar();
		progressBar->setMaximum(taskWatcher->progressMaximum());
		progressBar->setValue(taskWatcher->progressValue());
		if(statusLabel->text().isEmpty()) {
			statusLabel->hide();
			progressBar->hide();
		}
		layout->insertWidget(layout->count() - 2, statusLabel);
		layout->insertWidget(layout->count() - 2, progressBar);
		connect(taskWatcher, &TaskWatcher::progressRangeChanged, progressBar, &QProgressBar::setMaximum);
		connect(taskWatcher, &TaskWatcher::progressValueChanged, progressBar, &QProgressBar::setValue);
		connect(taskWatcher, &TaskWatcher::progressTextChanged, statusLabel, &QLabel::setText);
		connect(taskWatcher, &TaskWatcher::progressTextChanged, statusLabel, [statusLabel, progressBar](const QString& text) {
			statusLabel->setVisible(!text.isEmpty());
			progressBar->setVisible(!text.isEmpty());
		});

		// Remove progress display when this task finished.
		connect(taskWatcher, &TaskWatcher::finished, progressBar, &QObject::deleteLater);
		connect(taskWatcher, &TaskWatcher::finished, statusLabel, &QObject::deleteLater);
	};

	// Create UI for every running task.
	for(TaskWatcher* watcher : taskManager.runningTasks()) {
		createUIForTask(watcher);
	}

	// Set to preferred size.
	resize(450, height());

	// Create a separate progress bar for every new active task.
	connect(&taskManager, &TaskManager::taskStarted, this, createUIForTask);

	// Show the dialog with a short delay.
	// This prevents the dialog from showing up for short tasks that terminate very quickly.
	QTimer::singleShot(100, this, &QDialog::show);

	// Activate local event handling to keep the dialog responsive.
	taskManager.startLocalEventHandling();
}

/******************************************************************************
* Destructor
******************************************************************************/
ProgressDialog::~ProgressDialog()
{
	_taskManager.stopLocalEventHandling();
}

/******************************************************************************
* Is called whenever one of the tasks was canceled.
******************************************************************************/
void ProgressDialog::onTaskCanceled()
{
	// Cancel all tasks when one of the active tasks has been canceled.
	taskManager().cancelAll();
}

/******************************************************************************
* Is called when the user tries to close the dialog.
******************************************************************************/
void ProgressDialog::closeEvent(QCloseEvent* event)
{
	taskManager().cancelAll();
	if(event->spontaneous())
		event->ignore();
	QDialog::closeEvent(event);
}

/******************************************************************************
* Is called when the user tries to close the dialog.
******************************************************************************/
void ProgressDialog::reject()
{
	taskManager().cancelAll();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
