import QtQuick 2.12
import QtQuick.Controls 2.5

GroupBox {
	id: control
	topPadding: label.height + 14.0

	background: Rectangle {
		y: control.topPadding - control.bottomPadding
		width: parent.width
		height: parent.height - control.topPadding + control.bottomPadding
		color: "#D0D0D0"
	}

	label: Label {
		x: 2
		width: parent.width - 2
		text: control.title
		elide: Text.ElideRight
		font.pointSize: 11
	}	
}
