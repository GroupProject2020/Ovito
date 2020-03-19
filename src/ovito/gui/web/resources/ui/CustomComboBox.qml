import QtQuick 2.12
import QtQuick.Controls 2.5

ComboBox {
	id: control
	implicitHeight: 32
	rightPadding: 4

	delegate: ItemDelegate {
		width: parent.width
		height: control.implicitHeight
        text: control.textRole ? (Array.isArray(control.model) ? modelData[control.textRole] : model[control.textRole]) : modelData
		font.weight: control.currentIndex === index ? Font.DemiBold : Font.Normal
        highlighted: control.highlightedIndex === index
        hoverEnabled: control.hoverEnabled
	}
	
	indicator: Image {
		id: image
		x: control.width - width - control.rightPadding
		y: control.topPadding + (control.availableHeight - height) / 2
		width: 20
		height: 20
		source: "qrc:/gui/ui/arrow_down.svg"
	}

/*
	indicator: Canvas {
		id: canvas
		x: control.width - width - control.rightPadding
		y: control.topPadding + (control.availableHeight - height) / 2
		width: 12
		height: 8
		contextType: "2d"

		Connections {
			target: control
			onPressedChanged: canvas.requestPaint()
		}

		onPaint: {
			context.reset();
			context.moveTo(0, 0);
			context.lineTo(width, 0);
			context.lineTo(width / 2, height);
			context.closePath();
			context.fillStyle = control.pressed ? "#000000" : "#303030";
			context.fill();
		}
	}
*/
}
