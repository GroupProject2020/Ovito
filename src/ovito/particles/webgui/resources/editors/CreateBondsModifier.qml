import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12

import "qrc:/gui/ui" as Ui

Column {
	spacing: 2

	Ui.RolloutPanel {
		title: qsTr("Create bonds")
		anchors.left: parent.left
		anchors.right: parent.right
		
		GridLayout {
			anchors.fill: parent
			columns: 2

			Label { text: qsTr("Cutoff radius:"); }
			Ui.FloatParameter { 
				propertyField: "uniformCutoff"
				Layout.fillWidth: true 
			}

			Label { text: qsTr("Lower cutoff:"); }
			Ui.FloatParameter { 
				propertyField: "minimumCutoff"
				Layout.fillWidth: true 
			}

			Ui.BooleanCheckBoxParameter { 
				propertyField: "onlyIntraMoleculeBonds"
				Layout.columnSpan: 2
			}
		}
	}

	Ui.SubobjectEditor {
		propertyField: "bondType"
		parentEditObject: propertyEditor.editObject
		anchors.left: parent.left
		anchors.right: parent.right
	}

	Ui.SubobjectEditor {
		propertyField: "bondsVis"
		parentEditObject: propertyEditor.editObject
		anchors.left: parent.left
		anchors.right: parent.right
	}
}