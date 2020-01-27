import QtQuick 2.12
import QtQuick.Controls 2.12

// Items in this module are defined in C++ code.
import org.ovito 1.0

ViewportWindow {
	id: viewportWindow

	Button {
		id: control
		anchors.top: parent.top
		anchors.left: parent.left
		text: parent.viewport ? parent.viewport.title : "inactive"
		flat: true
		font.weight: Font.Bold

		contentItem: Text {
			text: control.text
			font: control.font
			color: control.hovered ? "#ccccff" : "white"
			horizontalAlignment: Text.AlignHCenter
			verticalAlignment: Text.AlignVCenter
			elide: Text.ElideRight
			padding: 0.0
		}

		background: Rectangle {
			visible: false
		}

		onClicked: viewportMenu.popup()
	}

	Menu {
		id: viewportMenu
		MenuItem { 
			text: qsTr("Preview Mode")
			checkable: true
			checked: viewportWindow.viewport && viewportWindow.viewport.previewMode 
			onToggled: viewportWindow.viewport.previewMode = checked
		}
		MenuItem { 
			text: qsTr("Show Grid")
			checkable: true
			checked: viewportWindow.viewport && viewportWindow.viewport.gridVisible 
			onToggled: viewportWindow.viewport.gridVisible = checked
		}
		MenuItem {
			text: qsTr("Constrain Rotation")
			checkable: true
			enabled: false
		}
		MenuSeparator {}
		Menu {
			title: qsTr("View Type")
			enabled: false
			MenuItem { text: "Top" }
			MenuItem { text: "Bottom" }
			MenuItem { text: "Front" }
			MenuItem { text: "Back" }
			MenuItem { text: "Left" }
			MenuItem { text: "Right" }
			MenuItem { text: "Ortho" }
			MenuItem { text: "Perspective" }
		}
	}
}
