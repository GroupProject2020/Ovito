import QtQuick 2.12
import QtQuick.Controls 2.5
import org.ovito 1.0

CheckBox {
	id: control

	property alias propertyField: parameterUI.propertyName
	property bool parameterValue: false

	ParameterUI on checked {
		id: parameterUI
		editObject: propertyEditor.editObject
	}

	onToggled: { parameterUI.propertyValue = checked; }
}
