import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Voronoi analysis")
	
	GridLayout {
		anchors.fill: parent
		columns: 2

		Label { text: qsTr("Absolute face area threshold:") }
		Ui.FloatParameter { 
			propertyField: "faceThreshold"
			Layout.fillWidth: true 
		}

		Label { text: qsTr("Relative face area threshold:") }
		Ui.FloatParameter { 
			propertyField: "relativeFaceThreshold"
			Layout.fillWidth: true 
		}

		Ui.BooleanCheckBoxParameter { 
			propertyField: "computeIndices"
			Layout.columnSpan: 2
		}

		Label { text: qsTr("Edge length threshold:") }
		Ui.FloatParameter { 
			propertyField: "edgeThreshold"
			Layout.fillWidth: true 
		}

		Ui.BooleanCheckBoxParameter { 
			propertyField: "computeBonds"
			Layout.columnSpan: 2
		}

		Ui.BooleanCheckBoxParameter { 
			propertyField: "computePolyhedra"
			Layout.columnSpan: 2
		}

		Ui.BooleanCheckBoxParameter { 
			propertyField: "useRadii"
			Layout.columnSpan: 2
		}

		Ui.BooleanCheckBoxParameter { 
			propertyField: "onlySelected"
			Layout.columnSpan: 2
		}
	}
}