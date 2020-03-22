import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Simulation cell")
	
	ColumnLayout {
		anchors.fill: parent

		Ui.GroupBox {
			title: qsTr("Dimensionality")
			Layout.fillWidth: true
			RowLayout {
				anchors.fill: parent
				Ui.BooleanRadioButtonParameter {
					propertyField: "is2D"
					inverted: false
					text: qsTr("2D")
				}
				Ui.BooleanRadioButtonParameter {
					propertyField: "is2D"
					inverted: true
					text: qsTr("3D")
				}
			}
		}

		Ui.GroupBox {
			title: qsTr("Periodic boundary conditions")
			Layout.fillWidth: true
			RowLayout {
				anchors.fill: parent
				Ui.BooleanCheckBoxParameter {
					propertyField: "pbcX"
					text: qsTr("X")
				}
				Ui.BooleanCheckBoxParameter {
					propertyField: "pbcY"
					text: qsTr("Y")
				}
				Ui.BooleanCheckBoxParameter {
					propertyField: "pbcZ"
					text: qsTr("Z")
				}
			}
		}
	}
}