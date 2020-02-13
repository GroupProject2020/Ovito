import QtQuick 2.12
import QtQuick.Controls 2.12

// Items in this module are defined in C++ code.
import org.ovito 1.0

ViewportWindow {
	id: viewportWindow

	onViewportError: {
		viewportErrorDisplay.text = message
		viewportErrorDisplay.visible = true
	}

	Text {
		id: viewportErrorDisplay
		anchors.right: parent.right
		anchors.bottom: parent.bottom
		color: "red"
		horizontalAlignment: Text.AlignRight
		padding: 2.0
		visible: false
	}

	Button {
		id: control
		anchors.top: parent.top
		anchors.left: parent.left
		text: parent.viewport ? parent.viewport.title : "inactive"
		flat: true
		font.weight: Font.Bold
		hoverEnabled: true

		contentItem: Text {
			text: control.text
			font: control.font
			color: control.hovered ? "#ccccff" : "white"
			horizontalAlignment: Text.AlignHCenter
			verticalAlignment: Text.AlignVCenter
			elide: Text.ElideRight
			padding: 0.0
		}

		background: Rectangle {
			visible: false
		}

		onClicked: viewportMenu.popup()
	}

	Menu {
		id: viewportMenu
		MenuItem { 
			text: qsTr("Preview Mode")
			checkable: true
			checked: viewportWindow.viewport && viewportWindow.viewport.previewMode 
			onToggled: viewportWindow.viewport.previewMode = checked
		}
		MenuItem { 
			text: qsTr("Show Grid")
			checkable: true
			checked: viewportWindow.viewport && viewportWindow.viewport.gridVisible 
			onToggled: viewportWindow.viewport.gridVisible = checked
		}
		MenuItem {
			text: qsTr("Constrain Rotation")
			checkable: true
			checked: ViewportSettings.constrainCameraRotation
			onToggled: ViewportSettings.constrainCameraRotation = checked
		}
		MenuSeparator {}
		Menu {
			title: qsTr("View Type")
			MenuItem { 
				text: "Top" 
				checkable: true
				checked: viewportWindow.viewport && viewportWindow.viewport.viewType == Viewport.VIEW_TOP 
				onToggled: {
					viewportWindow.viewport.viewType = Viewport.VIEW_TOP
					viewportWindow.viewport.zoomToSceneExtents()
				}
			}
			MenuItem { 
				text: "Bottom" 
				checkable: true
				checked: viewportWindow.viewport && viewportWindow.viewport.viewType == Viewport.VIEW_BOTTOM 
				onToggled: {
					viewportWindow.viewport.viewType = Viewport.VIEW_BOTTOM
					viewportWindow.viewport.zoomToSceneExtents()
				}
			}
			MenuItem { 
				text: "Front" 
				checkable: true
				checked: viewportWindow.viewport && viewportWindow.viewport.viewType == Viewport.VIEW_FRONT 
				onToggled: {
					viewportWindow.viewport.viewType = Viewport.VIEW_FRONT
					viewportWindow.viewport.zoomToSceneExtents()
				}
			}
			MenuItem { 
				text: "Back" 
				checkable: true
				checked: viewportWindow.viewport && viewportWindow.viewport.viewType == Viewport.VIEW_BACK 
				onToggled: {
					viewportWindow.viewport.viewType = Viewport.VIEW_BACK
					viewportWindow.viewport.zoomToSceneExtents()
				}
			}
			MenuItem { 
				text: "Left" 
				checkable: true
				checked: viewportWindow.viewport && viewportWindow.viewport.viewType == Viewport.VIEW_LEFT 
				onToggled: {
					viewportWindow.viewport.viewType = Viewport.VIEW_LEFT
					viewportWindow.viewport.zoomToSceneExtents()
				}
			}
			MenuItem { 
				text: "Right" 
				checkable: true
				checked: viewportWindow.viewport && viewportWindow.viewport.viewType == Viewport.VIEW_RIGHT 
				onToggled: {
					viewportWindow.viewport.viewType = Viewport.VIEW_RIGHT
					viewportWindow.viewport.zoomToSceneExtents()
				}
			}
			MenuItem { 
				text: "Ortho" 
				checkable: true
				checked: viewportWindow.viewport && viewportWindow.viewport.viewType == Viewport.VIEW_ORTHO 
				onToggled: {
					viewportWindow.viewport.viewType = Viewport.VIEW_ORTHO
					viewportWindow.viewport.zoomToSceneExtents()
				}
			}
			MenuItem {
				text: "Perspective" 
				checkable: true
				checked: viewportWindow.viewport && viewportWindow.viewport.viewType == Viewport.VIEW_PERSPECTIVE 
				onToggled: {
					viewportWindow.viewport.viewType = Viewport.VIEW_PERSPECTIVE
					viewportWindow.viewport.zoomToSceneExtents()
				}
			}
		}
	}
}
