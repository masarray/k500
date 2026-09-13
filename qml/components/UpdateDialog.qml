import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root
    required property var updateManager
    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    focus: true
    padding: 0
    closePolicy: root.updateManager && root.updateManager.busy
                 ? Popup.NoAutoClose
                 : Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: 570
    height: 430

    Overlay.modal: Rectangle { color: "#B804070A" }

    background: Rectangle {
        radius: 15
        color: "#0B1015"
        border.width: 1
        border.color: "#263039"

        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: 14
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#151D23" }
                GradientStop { position: 0.46; color: "#0C1217" }
                GradientStop { position: 1.0; color: "#070B0F" }
            }
            opacity: .96
        }
    }

    contentItem: Item {
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 14

            RowLayout {
                Layout.fillWidth: true
                spacing: 13

                Rectangle {
                    Layout.preferredWidth: 50
                    Layout.preferredHeight: 50
                    radius: 12
                    color: "#10252A"
                    border.width: 1
                    border.color: "#2D656A"

                    LucideIcon {
                        anchors.centerIn: parent
                        name: "download"
                        size: 24
                        strokeWidth: 1.9
                        color: Theme.accent
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        Layout.fillWidth: true
                        text: root.updateManager && root.updateManager.updateAvailable
                              ? ("SonKuPik K500 " + root.updateManager.latestVersion + " tersedia")
                              : "Software Update"
                        color: Theme.text
                        font.family: Theme.fontFamily
                        font.pixelSize: 20
                        font.weight: Font.Bold
                    }
                    Text {
                        Layout.fillWidth: true
                        text: "Update resmi · diverifikasi sebelum instalasi"
                        color: Theme.textSoft
                        font.family: Theme.fontFamily
                        font.pixelSize: 11
                        font.weight: Font.Medium
                    }
                }

                SoftButton {
                    Layout.preferredWidth: 34
                    Layout.preferredHeight: 30
                    compact: true
                    text: "×"
                    enabled: !root.updateManager || !root.updateManager.busy
                    onClicked: root.close()
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: Theme.borderSoft
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Rectangle {
                    Layout.preferredWidth: 104
                    Layout.preferredHeight: 27
                    radius: 7
                    color: "#11181E"
                    border.width: 1
                    border.color: Theme.borderSoft
                    Text {
                        anchors.centerIn: parent
                        text: "CURRENT  " + (root.updateManager ? root.updateManager.currentVersion : "-")
                        color: Theme.textDim
                        font.family: Theme.monoFamily
                        font.pixelSize: 9
                        font.weight: Font.Bold
                    }
                }
                LucideIcon { name: "chevron-right"; size: 14; strokeWidth: 1.8; color: Theme.textDim }
                Rectangle {
                    Layout.preferredWidth: 104
                    Layout.preferredHeight: 27
                    radius: 7
                    color: "#123036"
                    border.width: 1
                    border.color: "#3F8C92"
                    Text {
                        anchors.centerIn: parent
                        text: "LATEST  " + (root.updateManager ? root.updateManager.latestVersion : "-")
                        color: Theme.accent
                        font.family: Theme.monoFamily
                        font.pixelSize: 9
                        font.weight: Font.Bold
                    }
                }
                Item { Layout.fillWidth: true }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 10
                color: "#080D11"
                border.width: 1
                border.color: "#1C252C"
                clip: true

                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 12
                    TextArea {
                        readOnly: true
                        wrapMode: Text.Wrap
                        selectByMouse: true
                        text: root.updateManager && root.updateManager.releaseNotes.length > 0
                              ? root.updateManager.releaseNotes
                              : "Pembaruan stabil SonKuPik K500 siap dipasang."
                        color: Theme.textSoft
                        font.family: Theme.fontFamily
                        font.pixelSize: 11
                        background: null
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6
                visible: root.updateManager && (root.updateManager.busy || root.updateManager.state === "error")

                ProgressBar {
                    Layout.fillWidth: true
                    visible: root.updateManager && root.updateManager.busy
                    from: 0
                    to: 1
                    value: root.updateManager ? root.updateManager.progress : 0
                }

                Text {
                    Layout.fillWidth: true
                    text: root.updateManager
                          ? (root.updateManager.errorText.length > 0
                             ? root.updateManager.errorText
                             : root.updateManager.statusText)
                          : ""
                    color: root.updateManager && root.updateManager.state === "error" ? Theme.amber : Theme.textDim
                    font.family: Theme.fontFamily
                    font.pixelSize: 10
                    wrapMode: Text.Wrap
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                SoftButton {
                    Layout.preferredWidth: 118
                    text: "Nanti"
                    compact: true
                    enabled: root.updateManager && !root.updateManager.busy
                    onClicked: {
                        root.updateManager.remindLater()
                        root.close()
                    }
                }

                SoftButton {
                    Layout.preferredWidth: 128
                    text: "Lewati versi"
                    compact: true
                    enabled: root.updateManager && !root.updateManager.busy
                    onClicked: {
                        root.updateManager.skipThisVersion()
                        root.close()
                    }
                }

                Item { Layout.fillWidth: true }

                SoftButton {
                    Layout.preferredWidth: 174
                    Layout.preferredHeight: 34
                    text: root.updateManager && root.updateManager.busy ? "Memproses…" : "Update sekarang"
                    compact: false
                    mixerSelect: true
                    primaryAction: true
                    checked: true
                    enabled: root.updateManager && !root.updateManager.busy
                    onClicked: root.updateManager.downloadAndInstall()
                }
            }

            Text {
                Layout.fillWidth: true
                text: "Aplikasi akan mengunduh installer resmi, memverifikasi SHA-256, meminta izin Administrator Windows, memasang update, lalu membuka SonKuPik K500 kembali."
                color: Theme.textDim
                font.family: Theme.fontFamily
                font.pixelSize: 9
                wrapMode: Text.Wrap
            }
        }
    }
}
