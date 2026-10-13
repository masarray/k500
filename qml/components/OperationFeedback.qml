import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// COMPACT_VERIFIED_OPERATION_TOAST_V2 — view only. Completion is still owned
// exclusively by the native K500 preset coordinator, never by a display timer.
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
    readonly property string action: kind === "Recall" ? "Memanggil"
                                     : kind === "Upload" ? "Mengunggah" : "Menyimpan"
    readonly property string headline: phase === "busy" ? action + " preset…"
                                       : phase === "success"
                                         ? (kind === "Recall" ? "Preset dipanggil"
                                            : kind === "Upload" ? "Preset diunggah" : "Preset disimpan")
                                         : "Operasi gagal"

    // Overlay.overlay means coordinates refer to the whole ApplicationWindow.
    parent: Overlay.overlay
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    width: Math.min(354, parent ? parent.width - 40 : 354)
    height: phase === "error" ? 86 : 64
    padding: 0
    visible: phase !== ""
    modal: false
    dim: false
    focus: false
    closePolicy: Popup.NoAutoClose

    enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 100 } }
    exit: Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 130 } }

    background: Rectangle {
        radius: 11
        color: "#18232A"
        border.width: 1
        border.color: "#40505A"
    }
    contentItem: RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        spacing: 11

        Rectangle {
            Layout.preferredWidth: 25
            Layout.preferredHeight: 25
            radius: 12.5
            color: "transparent"
            border.width: 1.5
            border.color: feedback.statusColor
            Text {
                anchors.centerIn: parent
                visible: !feedback.working
                text: feedback.phase === "success" ? "✓" : "!"
                color: feedback.statusColor
                font.family: Theme.fontFamily
                font.pixelSize: 16
                font.weight: Font.DemiBold
            }
            Rectangle {
                anchors.centerIn: parent
                visible: feedback.working
                width: 7
                height: 7
                radius: 3.5
                color: Theme.accent
                SequentialAnimation on opacity {
                    running: feedback.working
                    loops: Animation.Infinite
                    NumberAnimation { to: 0.35; duration: 480 }
                    NumberAnimation { to: 1; duration: 480 }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 3
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Text {
                    Layout.fillWidth: true
                    text: feedback.headline
                    color: Theme.text
                    font.family: Theme.fontFamily
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }
                Text {
                    visible: feedback.slot > 0
                    text: "Slot " + String(feedback.slot).padStart(2, "0")
                    color: Theme.textSoft
                    font.family: Theme.fontFamily
                    font.pixelSize: 11
                    font.weight: Font.Medium
                }
            }
            Text {
                visible: feedback.phase === "error"
                Layout.fillWidth: true
                text: feedback.message || "Periksa koneksi K500."
                color: "#FFC0C3"
                font.family: Theme.fontFamily
                font.pixelSize: 11
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }
        }
    }
}
