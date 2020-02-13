import QtQuick 2.12
import QtQuick.Layouts 1.11
import QtQuick.Controls 2.12

// Items in this module are defined in C++ code.
import org.ovito 1.0

import "ui" as Ui

ApplicationWindow {
    visible: true
	width: 800
	height: 600
	color: "#444"

	header: Ui.ToolBar {
		id: toolBar
		objectName: "toolBar"
	}

	Ui.ErrorDialog {
		id: errorDialog
	}

	Ui.AboutDialog {
		id: aboutDialog
	}

	MainWindow {
		id: mainWindow
		anchors.fill: parent

		ViewportsPanel {
			id: viewportsPanel
			anchors.fill: parent
		}

		onError: {
            errorDialog.text = message
            errorDialog.open()
        }
	}

	footer: Ui.AnimationBar {
		id: animationBar
		objectName: "animationBar"
	}
}