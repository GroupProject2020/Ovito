import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Centrosymmetry parameter")
	
	GridLayout {
		anchors.fill: parent
		columns: 2

		Label { text: qsTr("Number of neighbors:") }
		Ui.IntegerParameter { 
			propertyField: "numNeighbors"
			Layout.fillWidth: true 
		}
	}
}