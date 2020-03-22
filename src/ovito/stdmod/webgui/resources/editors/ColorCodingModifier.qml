import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12

import "qrc:/gui/ui" as Ui

Ui.RolloutPanel {
	title: qsTr("Color coding")

	ColumnLayout {
		anchors.fill: parent

		Label { text: qsTr("Operate on:") }
		Ui.ModifierDelegateParameter {
			delegateType: "ColorCodingModifierDelegate"
			Layout.fillWidth: true 
		}

		Ui.BooleanCheckBoxParameter { 
			id: colorOnlySelectedUI
			propertyField: "colorOnlySelected"
			Layout.fillWidth: true
		}

		Ui.BooleanCheckBoxParameter { 
			propertyField: "keepSelection"
			enabled: colorOnlySelectedUI.checked
			Layout.fillWidth: true
		}
	}
}
