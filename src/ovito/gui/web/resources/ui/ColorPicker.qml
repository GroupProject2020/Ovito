import QtQuick 2.0
import QtQuick.Controls 2.5

AbstractButton {
    id: control
    property alias color: rect.color

	signal colorModified()   //< interactive change only, NOT emitted if \e color property is set directly)

    contentItem: Rectangle {
        id: rect
        implicitWidth: 64
        implicitHeight: 32

        border.width: 1
        border.color: control.down ? "#404040" : "#a0a0a0"
    }
/*
    onClicked: {
        colorPickerPopup.setColor(color);
        popup.open();
    }

    Popup {
        id: popup

        parent: Overlay.overlay

        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        ColorPickerPopup {
            id: colorPickerPopup
            onColorValueChanged: {
                if(popup.opened) {
                    control.color = colorValue;
                    colorModified();
                }
            }
        }
    }
*/
}