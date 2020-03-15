import QtQuick 2.12
import QtQuick.Controls 2.5

GroupBox {
	id: control
	leftPadding: 6
	rightPadding: 6
	topPadding: 6 + implicitLabelHeight
	bottomPadding: 6
	
	label: Label {
		x: 2.0
		width: control.width - 4.0
		text: control.title
		horizontalAlignment: Text.AlignHCenter
		color: "white"
		elide: Text.ElideRight
		topPadding: 2.0
		bottomPadding: 2.0
		background: Rectangle {
			color: "gray"
			border.width: 1
			border.color: "#060606"
		}
	}

	background: Rectangle {
		anchors.top: label.bottom 
		anchors.left: label.left
		anchors.right: label.right
		height: parent.height - control.topPadding + control.bottomPadding
		color: "transparent"
		border.color: "#909090"
	}
}
