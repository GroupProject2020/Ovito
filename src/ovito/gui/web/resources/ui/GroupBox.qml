import QtQuick 2.12
import QtQuick.Controls 2.5

GroupBox {
	id: control
	topPadding: label.height + 8.0
	leftPadding: 4
	rightPadding: 4
	bottomPadding: 4

	background: Rectangle {
		y: control.topPadding - control.bottomPadding
		width: parent.width
		height: parent.height - control.topPadding + control.bottomPadding
		color: "#E0E0E0"
	}

	label: Label {
		x: 2
		width: parent.width - 2
		text: control.title
		elide: Text.ElideRight
		font.pointSize: 0.8 * Qt.application.font.pointSize
	}
}
