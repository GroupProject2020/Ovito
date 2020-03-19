import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12

ColumnLayout {
	spacing: 0
	// Modifiers combobox:
	CustomComboBox {
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

		// Request a viewport update whenever a new item in the pipeline editor is selected, 
		// because the currently selected modifier may be rendering gizmos in the viewports. 
		onSelectedObjectChanged: {
			if(viewportsPanel.viewportConfiguration)
				viewportsPanel.viewportConfiguration.updateViewports();
		}
	}
	// Properties editor:
	PropertiesEditor {
		Layout.fillWidth: true
		Layout.fillHeight: true
		Layout.preferredHeight: 160
		editObject: pipelineEditor.selectedObject
	}
}
