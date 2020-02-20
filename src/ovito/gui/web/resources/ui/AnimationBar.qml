import QtQuick 2.3
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

RowLayout {
	property var anim: mainWindow.datasetContainer.currentSet ? mainWindow.datasetContainer.currentSet.animationSettings : null
	visible: anim ? (anim.singleFrame == false) : false

	Slider {
		id: control
		Layout.fillWidth: true
		snapMode: Slider.SnapAlways
		stepSize: 1.0
		value: anim ? anim.currentFrame : 0
		from: anim ? anim.firstFrame : 0
		to: anim ? anim.lastFrame : 0
		onMoved: anim.currentFrame = value

		handle: Rectangle {
			x: control.leftPadding + (control.horizontal ? control.visualPosition * (control.availableWidth - width) : (control.availableWidth - width) / 2)
			y: control.topPadding + (control.horizontal ? (control.availableHeight - height) / 2 : control.visualPosition * (control.availableHeight - height))
			implicitWidth: timeLabel.implicitWidth + 20
			implicitHeight: 28
			radius: height / 6
			color: control.pressed ? control.palette.light : control.palette.window
			border.width: control.visualFocus ? 2 : 1
			border.color: control.visualFocus ? control.palette.highlight : control.enabled ? control.palette.mid : control.palette.midlight

			Text {
				id: timeLabel
				anchors.centerIn: parent
				text: anim ? (anim.currentFrame + " / " + anim.lastFrame) : ""
			}
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
			enabled: anim ? (anim.currentFrame != anim.firstFrame && !anim.playbackActive) : false
			onClicked: anim.jumpToAnimationStart()
		}
		ToolButton {
			id: jumpToPreviousFrameButton
			icon.source: "qrc:/gui/actions/animation/goto_previous_frame.bw.svg"
			ToolTip.text: qsTr("Previous animation frame")
			ToolTip.visible: hovered
			ToolTip.delay: 500
			display: AbstractButton.IconOnly
			enabled: anim ? (anim.currentFrame != anim.firstFrame && !anim.playbackActive) : false
			onClicked: anim.jumpToPreviousFrame()
		}
		ToolButton {
			id: playAnimationButton
			icon.source: checked ? "qrc:/gui/actions/animation/stop_animation.bw.svg" : "qrc:/gui/actions/animation/play_animation.bw.svg"
			ToolTip.text: checked ? qsTr("Stop animation") : qsTr("Play animation")
			ToolTip.visible: hovered
			ToolTip.delay: 500
			checkable: true
			display: AbstractButton.IconOnly
			enabled: anim ? (anim.singleFrame == false) : false
			checked: anim ? anim.playbackActive : false
			onToggled: anim.playbackActive = checked
		}
		ToolButton {
			id: jumpToNextFrameButton
			icon.source: "qrc:/gui/actions/animation/goto_next_frame.bw.svg"
			ToolTip.text: qsTr("Next animation frame")
			ToolTip.visible: hovered
			ToolTip.delay: 500
			display: AbstractButton.IconOnly
			enabled: anim ? (anim.currentFrame != anim.lastFrame && !anim.playbackActive) : false
			onClicked: anim.jumpToNextFrame()
		}
		ToolButton {
			id: jumpToLastFrameButton
			icon.source: "qrc:/gui/actions/animation/goto_animation_end.bw.svg"
			ToolTip.text: qsTr("Last animation frame")
			ToolTip.visible: hovered
			ToolTip.delay: 500
			display: AbstractButton.IconOnly
			enabled: anim ? (anim.currentFrame != anim.lastFrame && !anim.playbackActive) : false
			onClicked: anim.jumpToAnimationEnd()
		}
	}
}
