import QtQuick 2.12
import QtQuick.Controls 2.12

// Items in this module are defined in C++ code.
import org.ovito 1.0

ViewportWindow {
	Button {
		id: control
		anchors.top: parent.top
		anchors.left: parent.left
		text: parent.title
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
		MenuItem { text: "Preview Mode" }
		MenuItem { text: "Show Grid" }
		MenuItem { text: "Constrain Rotation" }
		MenuSeparator {}
		Menu {
			title: qsTr("View Type")
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
