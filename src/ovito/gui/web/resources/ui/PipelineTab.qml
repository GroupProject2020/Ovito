import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12

ColumnLayout {
	spacing: 0
	// Modifiers combobox:
	ComboBox {
		Layout.fillWidth: true
		model: mainWindow.modifierListModel
		textRole: "display"
		onActivated: {
			mainWindow.modifierListModel.insertModifier(index, pipelineEditor.model)
			currentIndex = 0;
		}
	}
	// Pipeline editor:
	PipelineEditor {
		id: pipelineEditor
		Layout.fillWidth: true
		Layout.fillHeight: true
		Layout.preferredHeight: 100
		model: mainWindow.pipelineListModel
	}
	// Properties editor:
	PropertiesEditor {
		Layout.fillWidth: true
		Layout.fillHeight: true
		Layout.preferredHeight: 100
		editObject: pipelineEditor.selectedObject
	}
}
