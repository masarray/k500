import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// WINDOW_CENTERED_OPERATION_FEEDBACK_V1 — presentation only. Native operation
// signals own completion; this Popup never issues a Store or Recall command.
Popup {
    id: feedback
    property string kind: ""
    property string phase: ""
    property int slot: 0
    property string stage: ""
    property string message: ""

    readonly property bool working: phase === "busy"
    readonly property color statusColor: phase === "error" ? "#F0848B"
                                         : phase === "success" ? "#59D8B4" : Theme.accent
    readonly property string actionTitle: kind === "Recall" ? "RECALLING PRESET"
                                           : kind === "Upload" ? "UPLOADING PRESET" : "SAVING PRESET"

    // Popups declared inside a lazy System workspace are reparented to the
    // ApplicationWindow overlay, so x/y are global WINDOW coordinates.
    parent: Overlay.overlay
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    width: Math.min(474, parent ? parent.width - 40 : 474)
    height: 142
    padding: 0
    visible: phase !== ""
    modal: false
    dim: false
    focus: false
    closePolicy: Popup.NoAutoClose

    enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 130 } }
    exit: Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 130 } }

    background: Rectangle {
        radius: 12
        color: "#18232A"
        border.width: 1
        border.color: feedback.statusColor
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 12
            height: 1
            color: feedback.statusColor
            opacity: 0.38
        }
    }
    contentItem: ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 9
        RowLayout {
            Layout.fillWidth: true
            spacing: 11
            Item {
                Layout.preferredWidth: 29
                Layout.preferredHeight: 29
                Item {
                    anchors.centerIn: parent
                    width: 26
                    height: 26
                    visible: feedback.working
                    Rectangle {
                        anchors.centerIn: parent
                        width: 23; height: 23; radius: 12
                        color: "transparent"
                        border.width: 2
                        border.color: "#31525B"
                    }
                    Rectangle {
                        width: 6; height: 6; radius: 3
                        anchors.top: parent.top
                        anchors.horizontalCenter: parent.horizontalCenter
                        color: Theme.accent
                    }
                    RotationAnimator on rotation {
                        running: feedback.working
                        from: 0; to: 360; duration: 850
                        loops: Animation.Infinite
                    }
                }
                Text {
                    anchors.centerIn: parent
                    visible: !feedback.working
                    text: feedback.phase === "success" ? "✓" : "!"
                    color: feedback.statusColor
                    font.family: Theme.fontFamily
                    font.pixelSize: 22
                    font.weight: Font.DemiBold
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3
                Text {
                    Layout.fillWidth: true
                    text: feedback.actionTitle
                    color: Theme.text
                    font.family: Theme.fontFamily
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0.4
                }
                Text {
                    text: "DEVICE SLOT " + String(feedback.slot).padStart(2, "0")
                    color: Theme.textSoft
                    font.family: Theme.fontFamily
                    font.pixelSize: 10
                    font.weight: Font.Medium
                }
            }
            Text {
                text: feedback.working ? "IN PROGRESS"
                      : feedback.phase === "success" ? "VERIFIED" : "CHECK DEVICE"
                color: feedback.statusColor
                font.family: Theme.fontFamily
                font.pixelSize: 10
                font.weight: Font.DemiBold
            }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#31404A" }
        Text {
            Layout.fillWidth: true
            Layout.fillHeight: true
            text: feedback.working
                  ? (feedback.stage.length ? feedback.stage : "Preparing K500 transaction…")
                  : feedback.message
            color: feedback.phase === "error" ? "#FFC0C3" : Theme.textSoft
            font.family: Theme.fontFamily
            font.pixelSize: 12
            wrapMode: Text.Wrap
            maximumLineCount: 2
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Rectangle {
                id: progressTrack
                Layout.fillWidth: true
                Layout.preferredHeight: 4
                radius: 2
                color: "#081014"
                clip: true
                Rectangle {
                    width: 76
                    height: parent.height
                    radius: 2
                    visible: feedback.working
                    color: Theme.accent
                    SequentialAnimation on x {
                        running: feedback.working
                        loops: Animation.Infinite
                        NumberAnimation { from: -76; to: progressTrack.width; duration: 1150
                                          easing.type: Easing.InOutSine }
                    }
                }
                Rectangle {
                    anchors.fill: parent
                    visible: !feedback.working
                    radius: 2
                    color: feedback.statusColor
                }
            }
            Text {
                text: feedback.phase === "error" ? "Periksa koneksi & slot"
                      : feedback.working ? "PLEASE WAIT" : "K500 CONFIRMED"
                color: feedback.phase === "error" ? "#FFC0C3" : Theme.textDim
                font.family: Theme.fontFamily
                font.pixelSize: 9
            }
        }
    }
}
