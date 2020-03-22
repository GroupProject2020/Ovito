import QtQuick 2.12
import QtQuick.Controls 2.5

CheckBox {
	id: control

	indicator: Rectangle {
		implicitWidth: 18
		implicitHeight: 18
		x: control.leftPadding
		y: parent.height / 2 - height / 2
		border.color: control.down ? "#000000" : "#444444"
		color: control.pressed ? "#F0F0F0" : "#FFFFFF"
		radius: 2

		Rectangle {
			width: 12
			height: 12
			x: 3
			y: 3
			radius: 2
			color: control.down ? "#000000" : "#444444"
			visible: control.checked
		}
	}
}
