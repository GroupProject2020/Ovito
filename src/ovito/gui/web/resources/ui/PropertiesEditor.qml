import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12
import org.ovito 1.0

ScrollView {
	id: propertyEditor

	property RefTarget editObject
	readonly property ParameterUI parameterUI: ParameterUI { editObject: propertyEditor.editObject }
	clip: true

	Flickable {
		contentHeight: column.childrenRect.height + column.topPadding + column.bottomPadding

		Column {
			id: column
			anchors.fill: parent
			topPadding: 2
			bottomPadding: 2
			spacing: 2

			/// Displays the editors for the current object.
			Repeater {
				id: editorList 
				model: parameterUI.editorComponentList

				/// This Loader loads the QML component containing the UI for the current RefTarget.
				Loader { 
					source: modelData
					anchors.left: parent.left
					anchors.right: parent.right
				}
			}

			/// Displays the sub-object editors for RefMaker reference fields that have the PROPERTY_FIELD_OPEN_SUBEDITOR flag set.
			Repeater {
				id: subobjectEditorList
				model: parameterUI.subobjectFieldList

				SubobjectEditor {
					propertyField: modelData
					parentEditObject: propertyEditor.editObject
					anchors.left: parent.left
					anchors.right: parent.right
					visible: hasEditorComponent
				}
			}
		}
	}

	background: Rectangle {
		color: "#F0F0F0"

		Text {
			anchors.centerIn: parent
			visible: editObject != null && column.visibleChildren.length <= 2 
			text: "UI not implemented yet for this object"
		}
	}
}
