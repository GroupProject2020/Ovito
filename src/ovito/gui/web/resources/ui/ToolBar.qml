import QtQuick 2.3
import QtQuick.Controls 2.12

ToolBar {
	leftPadding: 8
	hoverEnabled: true

	Flow {
		id: flow
		width: parent.width

		Row {
			id: fileRow
			ToolButton {
				id: importLocalFileButton
				icon.source: "qrc:/gui/actions/file/file_import.bw.svg"
				ToolTip.text: qsTr("Import local file")
				ToolTip.visible: hovered
				ToolTip.delay: 500
				onClicked: mainWindow.importDataFile()
			}
			/*
			ToolButton {
				id: importRemoteFileButton
				icon.source: "qrc:/gui/actions/file/file_import_remote.bw.svg"
				ToolTip.text: qsTr("Import remote file")
				ToolTip.visible: hovered
				ToolTip.delay: 500
				display: AbstractButton.IconOnly
			}
			*/
			ToolSeparator {
				contentItem.visible: fileRow.y === viewportRow.y
			}
		}

/*
		Row {
			id: editRow
			ToolButton {
				id: undoButton
				icon.source: "qrc:/gui/actions/edit/edit_undo.bw.svg"
				ToolTip.text: qsTr("Undo action")
				ToolTip.visible: hovered
				ToolTip.delay: 500
				enabled: false
				display: AbstractButton.IconOnly
			}
			ToolButton {
				id: redoButton
				icon.source: "qrc:/gui/actions/edit/edit_redo.bw.svg"
				ToolTip.text: qsTr("Redo action")
				ToolTip.visible: hovered
				ToolTip.delay: 500
				enabled: false
				display: AbstractButton.IconOnly
			}
			ToolSeparator {
				contentItem.visible: editRow.y === viewportRow.y
			}
		}
*/

		Row {
			id: viewportRow
			ToolButton {
				id: zoomSceneExtentsButton
				icon.source: "qrc:/gui/actions/viewport/zoom_scene_extents.bw.svg"
				ToolTip.text: qsTr("Zoom to scene extents")
				ToolTip.visible: hovered
				ToolTip.delay: 500
				display: AbstractButton.IconOnly
				enabled: viewportsPanel.viewportConfiguration && viewportsPanel.viewportConfiguration.activeViewport
				onClicked: viewportsPanel.viewportConfiguration.activeViewport.zoomToSceneExtents()
			}
			ToolButton {
				id: multiViewportButton
				checkable: true
				checked: viewportsPanel.viewportConfiguration && viewportsPanel.viewportConfiguration.maximizedViewport == null
				enabled: viewportsPanel.viewportConfiguration
				icon.source: "qrc:/gui/actions/viewport/multi_viewports.svg"
				ToolTip.text: qsTr("Show multiple viewports")
				ToolTip.visible: hovered
				ToolTip.delay: 500
				display: AbstractButton.IconOnly
				onToggled: viewportsPanel.viewportConfiguration.maximizedViewport = checked ? null : viewportsPanel.viewportConfiguration.activeViewport
			}
			ToolSeparator {
				contentItem.visible: viewportRow.y === aboutRow.y
			}
		}

		Row {
			id: aboutRow
			ToolButton {
				id: aboutButton
				icon.source: "qrc:/gui/actions/file/about.bw.svg"
				ToolTip.text: qsTr("About OVITO")
				ToolTip.visible: hovered
				ToolTip.delay: 500
				display: AbstractButton.IconOnly
				onClicked: aboutDialog.open()
			}
		}
	}
}
