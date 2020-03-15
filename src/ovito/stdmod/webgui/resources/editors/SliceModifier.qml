import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12
import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Slice")
	
	ColumnLayout {
		anchors.fill: parent

		Ui.GroupBox {
			title: qsTr("Plane")
			Layout.fillWidth: true
			GridLayout {
				anchors.fill: parent
				columns: 2
			}
		}

		Ui.GroupBox {
			title: qsTr("Options")
			Layout.fillWidth: true
			ColumnLayout {
				anchors.fill: parent
				Ui.BooleanCheckBoxParameter {
					propertyField: "inverse"
					text: qsTr("Reverse orientation")
				}
			}
		}		
	}
}