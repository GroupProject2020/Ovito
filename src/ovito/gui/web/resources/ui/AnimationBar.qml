import QtQuick 2.3
import QtQuick.Controls 2.12

ToolBar {
	leftPadding: 8
	hoverEnabled: true
	visible: false

	Row {
		id: timeSliderRow
		ToolSeparator {
			contentItem.visible: timeSliderRow.y === animationRow.y
		}
	}

	Row {
		id: animationRow
		ToolButton {
			id: jumpToFirstFrameButton
			icon.source: "qrc:/gui/actions/animation/goto_animation_start.bw.svg"
			ToolTip.text: qsTr("First animation frame")
			ToolTip.visible: hovered
			ToolTip.delay: 500
			display: AbstractButton.IconOnly
//				onClicked: aboutDialog.open()
		}
		ToolButton {
			id: jumpToPreviousFrameButton
			icon.source: "qrc:/gui/actions/animation/goto_previous_frame.bw.svg"
			ToolTip.text: qsTr("Previous animation frame")
			ToolTip.visible: hovered
			ToolTip.delay: 500
			display: AbstractButton.IconOnly
//				onClicked: aboutDialog.open()
		}
		ToolButton {
			id: playAnimationButton
			icon.source: "qrc:/gui/actions/animation/play_animation.bw.svg"
			ToolTip.text: qsTr("Play animation")
			ToolTip.visible: hovered
			ToolTip.delay: 500
			display: AbstractButton.IconOnly
//				onClicked: aboutDialog.open()
		}
		ToolButton {
			id: jumpToNextFrameButton
			icon.source: "qrc:/gui/actions/animation/goto_next_frame.bw.svg"
			ToolTip.text: qsTr("Next animation frame")
			ToolTip.visible: hovered
			ToolTip.delay: 500
			display: AbstractButton.IconOnly
//				onClicked: aboutDialog.open()
		}
		ToolButton {
			id: jumpToLastFrameButton
			icon.source: "qrc:/gui/actions/animation/goto_animation_end.bw.svg"
			ToolTip.text: qsTr("Last animation frame")
			ToolTip.visible: hovered
			ToolTip.delay: 500
			display: AbstractButton.IconOnly
//				onClicked: aboutDialog.open()
		}
	}
}
