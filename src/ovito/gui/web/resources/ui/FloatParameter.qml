import QtQuick 2.12
import QtQuick.Controls 2.5
import org.ovito 1.0

Spinner {
	id: control

	from: Math.max(parameterUI.minParameterValue, -(Math.pow(2, 31) - 1))
	to: Math.min(parameterUI.maxParameterValue, Math.pow(2, 31) - 1)

	property alias propertyField: parameterUI.propertyName

	ParameterUI on value {
		id: parameterUI
		editObject: propertyEditor.editObject
	}

	onValueModified: { 
		parameterUI.propertyValue = value; 
	}
}
