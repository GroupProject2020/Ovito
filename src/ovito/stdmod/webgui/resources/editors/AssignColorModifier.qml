import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Assign color")

	GridLayout {
		anchors.fill: parent
		columns: 2

		Label { text: qsTr("Operate on:") }
		Ui.ModifierDelegateParameter {
			delegateType: "AssignColorModifierDelegate"
			Layout.fillWidth: true
		}

		Label { text: qsTr("Color:") }
		Ui.ColorParameter {
			propertyField: "colorController"
			Layout.fillWidth: true
		}

		Ui.BooleanCheckBoxParameter { 
			propertyField: "keepSelection"
			Layout.columnSpan: 2
		}
	}
}
