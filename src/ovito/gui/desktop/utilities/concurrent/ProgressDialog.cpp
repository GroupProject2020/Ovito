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

#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include <ovito/gui/desktop/widgets/general/ElidedTextLabel.h>
#include "ProgressDialog.h"

namespace Ovito {

/******************************************************************************
* Initializes the dialog window.
******************************************************************************/
ProgressDialog::ProgressDialog(QWidget* parent, const TaskPtr& task, const QString& dialogTitle) : QDialog(parent)
{
	setWindowModality(Qt::WindowModal);
	setWindowTitle(dialogTitle);

	// Get the task manager.
	_taskManager = task->taskManager();
	OVITO_ASSERT(_taskManager != nullptr);

	// Create the task watcher to monitor the running task.
	_watcher = new TaskWatcher(this);
	_watcher->watch(task);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addStretch(1);

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
	layout->addWidget(buttonBox);

	// Cancel the running task when user presses the cancel button.
	connect(buttonBox, &QDialogButtonBox::rejected, _watcher, &TaskWatcher::cancel);

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
	for(TaskWatcher* watcher : _taskManager->runningTasks()) {
		createUIForTask(watcher);
	}

	// Set to preferred size.
	resize(450, height());

	// Create a separate progress bar for every new active task.
	connect(_taskManager, &TaskManager::taskStarted, this, std::move(createUIForTask));

	// Show the dialog with a short delay.
	// This prevents the dialog from showing up for short tasks that terminate very quickly.
	QTimer::singleShot(100, this, &QDialog::show);

	// Activate local event handling to keep the dialog responsive.
	_taskManager->startLocalEventHandling();
}

/******************************************************************************
* Destructor
******************************************************************************/
ProgressDialog::~ProgressDialog()
{
	_taskManager->stopLocalEventHandling();
}

/******************************************************************************
* Is called when the user tries to close the dialog.
******************************************************************************/
void ProgressDialog::closeEvent(QCloseEvent* event)
{
	_watcher->cancel();
	if(event->spontaneous())
		event->ignore();
	QDialog::closeEvent(event);
}

/******************************************************************************
* Is called when the user tries to close the dialog.
******************************************************************************/
void ProgressDialog::reject()
{
	_watcher->cancel();
}

}	// End of namespace
