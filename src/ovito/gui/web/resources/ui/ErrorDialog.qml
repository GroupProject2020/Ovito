import QtQuick 2.3
import QtQuick.Controls 2.12

Dialog {
	id: dialog
	modal: true
	anchors.centerIn: parent
	standardButtons: Dialog.Ok
	contentWidth: 0.5 * parent.width
//	title: "Error"
	property string text;

	Text {
		text: "<h3>Error</h3><p></p>" + dialog.text
		textFormat: Text.StyledText
		wrapMode: Text.WordWrap
		anchors.fill: parent
	}
}
