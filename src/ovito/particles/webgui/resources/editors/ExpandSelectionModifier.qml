import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Expand selection")
	
	GridLayout {
		anchors.fill: parent
		columns: 2

		Label { 
			text: qsTr("Expand selection to include particles that are...") 
			wrapMode: Text.Wrap
			Layout.columnSpan: 2
			Layout.fillWidth: true 
		}

		Ui.IntegerRadioButtonParameter { 
			id: modeCutoffRange
			propertyField: "mode"
			value: 1
			text: qsTr("...within the range:")
			Layout.columnSpan: 2
		}

		GridLayout {
			columns: 2
			Layout.columnSpan: 2
			Layout.fillWidth: true
			Layout.leftMargin: 12
			Label { text: qsTr("Cutoff distance:") }
			Ui.FloatParameter { 
				propertyField: "cutoffRange"
				Layout.fillWidth: true 
				enabled: modeCutoffRange.checked
			}
		}

		Ui.IntegerRadioButtonParameter { 
			id: modeNearestNeighbors
			propertyField: "mode"
			value: 2
			text: qsTr("...among the N nearest neighbors:")
			Layout.columnSpan: 2
		}

		GridLayout {
			columns: 2
			Layout.columnSpan: 2
			Layout.fillWidth: true
			Layout.leftMargin: 12
			Label { text: qsTr("N:") }
			Ui.IntegerParameter { 
				propertyField: "numNearestNeighbors"
				Layout.fillWidth: true 
				enabled: modeNearestNeighbors.checked
			}
		}

		Ui.IntegerRadioButtonParameter { 
			propertyField: "mode"
			value: 0
			text: qsTr("...bonded to a selected particle.")
			Layout.columnSpan: 2
		}

		Label { text: qsTr("Number of iterations:") }
		Ui.IntegerParameter { 
			propertyField: "numberOfIterations"
			Layout.fillWidth: true 
		}
	}
}