import QtQuick 2.12
import QtQuick.Controls 2.5
import org.ovito 1.0

Spinner {
	id: control

	from: parameterUI.minParameterValue
	to: parameterUI.maxParameterValue

	property alias propertyField: parameterUI.propertyName
	property vector3d vectorValue
	property int vectorComponent: 0

	value: (vectorComponent == 0) ? vectorValue.x : ((vectorComponent == 1) ? vectorValue.y : vectorValue.z)
	onVectorValueChanged: value = (vectorComponent == 0) ? vectorValue.x : ((vectorComponent == 1) ? vectorValue.y : vectorValue.z)

	ParameterUI on vectorValue {
		id: parameterUI
		editObject: propertyEditor.editObject
	}

	onValueModified: { 
		var newVec = vectorValue;
		if(vectorComponent == 0) newVec.x = value;
		if(vectorComponent == 1) newVec.y = value;
		if(vectorComponent == 2) newVec.z = value;
		parameterUI.propertyValue = newVec;
	}
}
