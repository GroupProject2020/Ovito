import QtQuick 2.12
import QtQuick.Controls 2.5
import org.ovito 1.0

Column {
	spacing: 2

	id: propertyEditor
	property RefTarget parentEditObject
	property RefTarget editObject
	property bool hasEditorComponent: subEditorList.model.length != 0
	property alias propertyField: parameterUI.propertyName
	readonly property ParameterUI subParameterUI: ParameterUI { editObject: propertyEditor.editObject }

	ParameterUI on editObject {
		id: parameterUI
		editObject: parentEditObject
	}

	/// Displays the editor components for the current edit object.
	Repeater {
		id: subEditorList 
		model: subParameterUI.editorComponentList

		Loader {
			source: modelData
			anchors.left: parent.left
			anchors.right: parent.right
		}
	}
}
