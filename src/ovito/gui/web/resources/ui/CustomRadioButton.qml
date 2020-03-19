import QtQuick 2.12
import QtQuick.Controls 2.5

RadioButton {
	id: control
	
	indicator: Rectangle {
        implicitWidth: 18
        implicitHeight: 18
        x: control.leftPadding
        y: parent.height / 2 - height / 2
        radius: 9
		color: control.pressed ? "#F0F0F0" : "#FFFFFF"
        border.color: control.down ? "#404040" : "#505050"

        Rectangle {
            width: 12
            height: 12
            x: 3
            y: 3
            radius: 6
            color: control.down ? "#404040" : "#505050"
            visible: control.checked
        }
    }	
}
