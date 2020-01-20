import QtQuick 2.3
import QtQuick.Window 2.2

// The main window.
Window {
    visible: true

    MouseArea {
        anchors.fill: parent
        onClicked: {
            	Qt.quit();
        }
    }

	Rectangle {
		anchors.fill: parent
		Text {
			anchors.centerIn: parent
			text: "Hello World"
		}
	}
}