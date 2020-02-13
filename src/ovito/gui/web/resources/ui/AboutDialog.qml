import QtQuick 2.3
import QtQuick.Controls 2.12

Dialog {
	modal: true
	anchors.centerIn: parent
	standardButtons: Dialog.Close
	contentWidth: 0.6 * parent.width

	Text {
		textFormat: Text.StyledText
		wrapMode: Text.WordWrap
		anchors.fill: parent

		text: 
			  qsTr("<p><img src=\"qrc:/guibase/mainwin/window_icon_128.png\" width=\"48\" height=\"48\" /></p><p></p><h3>Ovito (Open Visualization Tool)</h3><p></p><p>Version %1</p>").arg(Qt.application.version)
			+ qsTr("<p>A scientific data visualization and analysis software for particle simulations.</p><p></p><p>Copyright (C) 2020, Alexander Stukowski</p><p>This is free, open-source software, and you are welcome to redistribute it according to the terms of the GNU General Public License (v3).</p>")
			+ qsTr("<p></p><p>Source code is available at <a href=\"https://gitlab.com/stuko/ovito\">gitlab.com/stuko/ovito</a>.</p>")

		onLinkActivated: Qt.openUrlExternally(link)
	}
}
