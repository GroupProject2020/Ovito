////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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
#include <ovito/core/utilities/concurrent/TaskManager.h>
#include <ovito/core/utilities/concurrent/TaskWatcher.h>
#include "TaskDisplayWidget.h"

namespace Ovito {

/******************************************************************************
* Constructs the widget and associates it with the main window.
******************************************************************************/
TaskDisplayWidget::TaskDisplayWidget(MainWindow* mainWindow) : QWidget(nullptr), _mainWindow(mainWindow)
{
	setVisible(false);

	QHBoxLayout* progressWidgetLayout = new QHBoxLayout(this);
	progressWidgetLayout->setContentsMargins(0,0,0,0);
	progressWidgetLayout->setSpacing(0);
	_progressTextDisplay = new QLabel();
	_progressTextDisplay->setLineWidth(0);
	_progressTextDisplay->setAlignment(Qt::Alignment(Qt::AlignRight | Qt::AlignVCenter));
	_progressTextDisplay->setAutoFillBackground(true);
	_progressTextDisplay->setMargin(2);
	_progressTextDisplay->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);
	_progressBar = new QProgressBar(this);
#if 0
	_cancelTaskButton = new QToolButton(this);
	_cancelTaskButton->setText(tr("Cancel"));
	QIcon cancelIcon(":/gui/mainwin/process-stop-16.png");
	cancelIcon.addFile(":/gui/mainwin/process-stop-22.png");
	_cancelTaskButton->setIcon(cancelIcon);
#endif
	progressWidgetLayout->addWidget(_progressBar);
//	progressWidgetLayout->addWidget(_cancelTaskButton);
	progressWidgetLayout->addStrut(_progressTextDisplay->sizeHint().height());
	setMinimumHeight(_progressTextDisplay->minimumSizeHint().height());

//	connect(_cancelTaskButton, &QAbstractButton::clicked, &mainWindow->datasetContainer().taskManager(), &TaskManager::cancelAll);
	connect(&mainWindow->datasetContainer().taskManager(), &TaskManager::taskStarted, this, &TaskDisplayWidget::taskStarted);
	connect(&mainWindow->datasetContainer().taskManager(), &TaskManager::taskFinished, this, &TaskDisplayWidget::taskFinished);
	connect(this, &QObject::destroyed, _progressTextDisplay, &QObject::deleteLater);
}

/******************************************************************************
* Is called when a task has started to run.
******************************************************************************/
void TaskDisplayWidget::taskStarted(TaskWatcher* taskWatcher)
{
	// Show progress indicator only if the task doesn't finish within 200 milliseconds.
	if(isHidden())
		QTimer::singleShot(200, this, &TaskDisplayWidget::showIndicator);
	else
		updateIndicator();

	connect(taskWatcher, &TaskWatcher::progressRangeChanged, this, &TaskDisplayWidget::taskProgressChanged);
	connect(taskWatcher, &TaskWatcher::progressValueChanged, this, &TaskDisplayWidget::taskProgressChanged);
	connect(taskWatcher, &TaskWatcher::progressTextChanged, this, &TaskDisplayWidget::taskProgressChanged);
}

/******************************************************************************
* Is called when a task has finished.
******************************************************************************/
void TaskDisplayWidget::taskFinished(TaskWatcher* taskWatcher)
{
	updateIndicator();
}

/******************************************************************************
* Is called when the progress of a task has changed
******************************************************************************/
void TaskDisplayWidget::taskProgressChanged()
{
	const TaskManager& taskManager = _mainWindow->datasetContainer().taskManager();
	if(taskManager.runningTasks().empty() == false)
		updateIndicator();
}

/******************************************************************************
* Shows the progress indicator widget.
******************************************************************************/
void TaskDisplayWidget::showIndicator()
{
	const TaskManager& taskManager = _mainWindow->datasetContainer().taskManager();
	if(isHidden() && taskManager.runningTasks().empty() == false) {
		_mainWindow->statusBar()->addWidget(_progressTextDisplay, 1);
		show();
		_progressTextDisplay->show();
		updateIndicator();
	}
}

/******************************************************************************
* Shows or hides the progress indicator widgets and updates the displayed information.
******************************************************************************/
void TaskDisplayWidget::updateIndicator()
{
	if(isHidden())
		return;

	const TaskManager& taskManager = _mainWindow->datasetContainer().taskManager();
	if(taskManager.runningTasks().empty()) {
		hide();
		_mainWindow->statusBar()->removeWidget(_progressTextDisplay);
	}
	else {
		for(auto iter = taskManager.runningTasks().cbegin(); iter != taskManager.runningTasks().cend(); iter++) {
			TaskWatcher* watcher = *iter;
			qlonglong maximum = watcher->progressMaximum();
			if(watcher->progressMaximum() != 0 || watcher->progressText().isEmpty() == false) {
				if(maximum < (qlonglong)std::numeric_limits<int>::max()) {
					_progressBar->setRange(0, (int)maximum);
					_progressBar->setValue((int)watcher->progressValue());
				}
				else {
					_progressBar->setRange(0, 1000);
					_progressBar->setValue((int)(watcher->progressValue() * 1000ll / maximum));
				}
				_progressTextDisplay->setText(watcher->progressText());
				show();
				break;
			}
		}
	}
}

}	// End of namespace
