import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12

ColumnLayout {
	ComboBox {
		Layout.fillWidth: true
		model: ["Add modifier...", "Second", "Third"]
	}
	RowLayout {
		ListView {
			Layout.fillWidth: true
			Layout.fillHeight: true
			model: ListModel {
				ListElement {
					name: "Bill Smith"
					number: "555 3264"
				}
				ListElement {
					name: "John Brown"
					number: "555 8426"
				}
				ListElement {
					name: "Sam Wise"
					number: "555 0473"
				}
			}
			delegate: Text {
				text: name + ": " + number
			}
		}
		ToolBar {
			Layout.alignment: Qt.AlignTop
			hoverEnabled: true
			Flow {
				flow: Flow.TopToBottom
				Layout.fillHeight: true

				Column {
					ToolButton {
						icon.source: "qrc:/gui/actions/file/file_import.bw.svg"
						ToolTip.text: qsTr("Import local file")
						ToolTip.visible: hovered
						ToolTip.delay: 500
					}
				}
			}
		}
	}
}
