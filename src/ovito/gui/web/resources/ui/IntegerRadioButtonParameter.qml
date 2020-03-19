import QtQuick 2.12
import QtQuick.Controls 2.5
import org.ovito 1.0

CustomRadioButton {
	id: control

	property int value: 0
	property alias propertyField: parameterUI.propertyName
	property int parameterValue: 0

	checked: parameterValue == value

	ParameterUI on parameterValue {
		id: parameterUI
		editObject: propertyEditor.editObject
	}

	onToggled: { 
		if(checked)
			parameterUI.propertyValue = value; 
	}
}
