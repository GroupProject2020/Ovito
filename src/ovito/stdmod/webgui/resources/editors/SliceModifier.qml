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

				Label { text: qsTr("Distance:") }
				Ui.FloatParameter {
					propertyField: "distanceController"
					Layout.fillWidth: true 
				}

				Label { text: qsTr("Normal (X):") }
				Ui.Vector3Parameter { 
					propertyField: "normalController"
					vectorComponent: 0
					Layout.fillWidth: true 
				}

				Label { text: qsTr("Normal (Y):") }
				Ui.Vector3Parameter { 
					propertyField: "normalController"
					vectorComponent: 1
					Layout.fillWidth: true 
				}

				Label { text: qsTr("Normal (Z):") }
				Ui.Vector3Parameter { 
					propertyField: "normalController"
					vectorComponent: 2
					Layout.fillWidth: true 
				}

				Label { text: qsTr("Slab width:") }
				Ui.FloatParameter {
					propertyField: "widthController"
					Layout.fillWidth: true 
				}
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

				Ui.BooleanCheckBoxParameter {
					propertyField: "createSelection"
					text: qsTr("Create selection")
				}

				Ui.BooleanCheckBoxParameter {
					propertyField: "applyToSelection"
					text: qsTr("Apply to selection only")
				}

				Ui.BooleanCheckBoxParameter {
					propertyField: "enablePlaneVisualization"
					text: qsTr("Visualize plane")
				}
			}
		}		
	}
}