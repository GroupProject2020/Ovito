import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12
import org.ovito 1.0

ScrollView {
	id: propertyEditor

	/// The RefTarget object that is currently shown in the properties editor.
	property RefTarget editObject
	clip: true

	Flickable {
		contentHeight: propertiesEditorLoader.height

		/// This Loader loads the QML component containing the UI for the current RefTarget.
		Loader { 
			id: propertiesEditorLoader 
			anchors.left: parent.left
			anchors.right: parent.right
			// Automatically generate the resource URL to the UI component for the current object's class. 
			source: propertyEditor.editObject ? ("qrc:/" + propertyEditor.editObject.pluginId + "/editors/" + propertyEditor.editObject.className + ".qml") : ""
		}
	}

	background: Rectangle {
		color: "#E0E0E0"

		Text {
			anchors.centerIn: parent
			visible: propertiesEditorLoader.status == Loader.Error
			text: "UI not implemented yet for this object"
		}
	}
}
