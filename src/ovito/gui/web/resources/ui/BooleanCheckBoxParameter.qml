import QtQuick 2.12
import QtQuick.Controls 2.5
import org.ovito 1.0

CheckBox {
	id: control

	property alias propertyField: parameterUI.propertyName
	property bool parameterValue: false
	text: parameterUI.propertyDisplayName

	ParameterUI on checked {
		id: parameterUI
		editObject: propertyEditor.editObject
	}

	onToggled: { parameterUI.propertyValue = checked; }

	indicator: Rectangle {
		implicitWidth: 18
		implicitHeight: 18
		x: control.leftPadding
		y: parent.height / 2 - height / 2
		border.color: control.down ? "#000000" : "#444444"
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
