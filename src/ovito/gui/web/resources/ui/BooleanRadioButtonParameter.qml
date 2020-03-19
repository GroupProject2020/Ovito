import QtQuick 2.12
import QtQuick.Controls 2.5
import org.ovito 1.0

CustomRadioButton {
	id: control

	property bool inverted: false
	property alias propertyField: parameterUI.propertyName
	property bool parameterValue: false

	checked: parameterValue != inverted

	ParameterUI on parameterValue {
		id: parameterUI
		editObject: propertyEditor.editObject
	}

	onToggled: { parameterUI.propertyValue = (checked != inverted); }
}
