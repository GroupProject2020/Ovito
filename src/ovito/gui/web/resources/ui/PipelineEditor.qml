import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12
import org.ovito 1.0

ScrollView {
	property alias model: listView.model
	property RefTarget selectedObject: listView.model ? listView.model.selectedObject : null

	clip: true

	ListView {	
		id: listView
		focus: true
		highlightMoveDuration: 0
		highlightMoveVelocity: -1
		onCurrentIndexChanged: { model.selectedIndex = currentIndex }
		Component {
			id: itemDelegate
			MouseArea {
				id: mouseArea
				anchors { left: parent.left; right: parent.right }
				height: itemInfo.height
				hoverEnabled: true
				onClicked: { 
					if(type <= 3) 
						ListView.view.currentIndex = index; 
				}
				states: State {
					name: "HOVERING"
					when: (containsMouse && type == 1)
					PropertyChanges { target: deleteButton; opacity: 0.7 }
				}
				transitions: Transition {
					to: "*"
					NumberAnimation { target: deleteButton; property: "opacity"; duration: 100; }
				}			
				Rectangle { 
					id: itemInfo
					height: (type >= 4) ? textItem.implicitHeight : Math.max(textItem.implicitHeight, checkboxItem.implicitHeight)
					width: parent.width
					color: (type >= 4) ? "lightgray" : "#00000000"; 
					CheckBox {
						id: checkboxItem
						visible: (type <= 1)
						checked: ischecked
						anchors.verticalCenter: parent.verticalCenter
						anchors.left: parent.left
						onToggled: {
							model.ischecked = checked;
						}
						indicator: Rectangle {
							implicitWidth: 18
							implicitHeight: 18
							x: checkboxItem.leftPadding
							y: parent.height / 2 - height / 2
							border.color: checkboxItem.down ? "#000000" : "#444444"
							radius: 2

							Rectangle {
								width: 12
								height: 12
								x: 3
								y: 3
								radius: 2
								color: checkboxItem.down ? "#000000" : "#444444"
								visible: checkboxItem.checked
							}
						}
					}
					Text {
						id: textItem
						text: title
						horizontalAlignment: (type < 4) ? Text.AlignLeft : Text.AlignHCenter
						anchors.verticalCenter: parent.verticalCenter
						anchors.left: (type <= 1) ? checkboxItem.right : parent.left
						anchors.right: parent.right
						anchors.leftMargin: (type <= 1) ? 0 : 6
					}
					Image {
						id: deleteButton
						anchors.verticalCenter: parent.verticalCenter
						anchors.right: parent.right
						anchors.rightMargin: 4.0
						opacity: 0
						source: "qrc:/gui/actions/edit/delete_modifier.svg"
						MouseArea {
							anchors.fill: parent
							onClicked: { 
								listView.model.deleteModifier(index)
							}
						}
					}
				}
			}
		}
		delegate: itemDelegate
		highlight: Rectangle { color: "lightsteelblue"; }
	}
}
