import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes

Popup {
    id: root
    required property var updateManager
    required property var deviceManager
    readonly property var presetManager: root.deviceManager ? root.deviceManager.presetManager : null
    readonly property bool deviceTransactionBusy: !!root.presetManager && !!root.presetManager.busy

    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    focus: true
    padding: 0
    closePolicy: root.updateManager && root.updateManager.busy
                 ? Popup.NoAutoClose
                 : Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: 584
    height: 444

    Overlay.modal: Rectangle { color: "#B804070A" }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 130; easing.type: Easing.OutCubic }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 90; easing.type: Easing.InCubic }
    }

    background: Item {
        Rectangle {
            anchors.fill: parent
            anchors.margins: -9
            radius: 21
            color: "#56000000"
        }
        Rectangle {
            anchors.fill: parent
            radius: 15
            border.width: 1
            border.color: "#2A353D"
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#151D23" }
                GradientStop { position: 0.46; color: "#0C1217" }
                GradientStop { position: 1.0; color: "#070B0F" }
            }
        }
    }

    contentItem: Item {
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 13

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
                        width: 24
                        height: 24
                        name: "download"
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
                        renderType: Text.NativeRendering
                        font.family: Theme.fontFamily
                        font.pixelSize: 20
                        font.weight: Font.Bold
                        font.hintingPreference: Font.PreferFullHinting
                    }
                    Text {
                        Layout.fillWidth: true
                        text: "Pembaruan stabil · diverifikasi sebelum instalasi"
                        color: Theme.textSoft
                        renderType: Text.NativeRendering
                        font.family: Theme.fontFamily
                        font.pixelSize: 11
                        font.weight: Font.Medium
                        font.hintingPreference: Font.PreferFullHinting
                    }
                }

                SoftButton {
                    Layout.preferredWidth: 34
                    Layout.preferredHeight: 30
                    compact: true
                    toolbar: true
                    text: ""
                    enabled: !root.updateManager || !root.updateManager.busy
                    onClicked: root.close()

                    Shape {
                        anchors.centerIn: parent
                        width: 24
                        height: 24
                        scale: 15 / 24
                        preferredRendererType: Shape.CurveRenderer
                        ShapePath {
                            strokeColor: Theme.textSoft
                            strokeWidth: 1.9
                            fillColor: "transparent"
                            capStyle: ShapePath.RoundCap
                            joinStyle: ShapePath.RoundJoin
                            PathSvg { path: "M18 6 6 18 M6 6l12 12" }
                        }
                    }
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
                    Layout.preferredWidth: 110
                    Layout.preferredHeight: 27
                    radius: 7
                    color: "#11181E"
                    border.width: 1
                    border.color: Theme.borderSoft
                    Text {
                        anchors.centerIn: parent
                        text: "CURRENT  " + (root.updateManager ? root.updateManager.currentVersion : "-")
                        color: Theme.textDim
                        renderType: Text.NativeRendering
                        font.family: Theme.monoFamily
                        font.pixelSize: 9
                        font.weight: Font.Bold
                        font.hintingPreference: Font.PreferFullHinting
                    }
                }
                LucideIcon {
                    Layout.preferredWidth: 14
                    Layout.preferredHeight: 14
                    name: "chevron-right"
                    strokeWidth: 1.8
                    color: Theme.textDim
                }
                Rectangle {
                    Layout.preferredWidth: 110
                    Layout.preferredHeight: 27
                    radius: 7
                    color: "#123036"
                    border.width: 1
                    border.color: "#3F8C92"
                    Text {
                        anchors.centerIn: parent
                        text: "LATEST  " + (root.updateManager ? root.updateManager.latestVersion : "-")
                        color: Theme.accent
                        renderType: Text.NativeRendering
                        font.family: Theme.monoFamily
                        font.pixelSize: 9
                        font.weight: Font.Bold
                        font.hintingPreference: Font.PreferFullHinting
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
                        renderType: Text.NativeRendering
                        font.family: Theme.fontFamily
                        font.pixelSize: 11
                        font.hintingPreference: Font.PreferFullHinting
                        background: null
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6
                visible: root.deviceTransactionBusy
                         || (root.updateManager
                             && (root.updateManager.busy || root.updateManager.state === "error"))

                ProgressBar {
                    Layout.fillWidth: true
                    visible: root.updateManager && root.updateManager.busy
                    from: 0
                    to: 1
                    value: root.updateManager ? root.updateManager.progress : 0
                }

                Text {
                    Layout.fillWidth: true
                    text: root.deviceTransactionBusy
                          ? "Tunggu Save / Upload / Mass Upload selesai sebelum memperbarui aplikasi. K500 tidak akan diputus di tengah transaksi permanen."
                          : root.updateManager
                            ? (root.updateManager.errorText.length > 0
                               ? root.updateManager.errorText
                               : root.updateManager.statusText)
                            : ""
                    color: root.deviceTransactionBusy
                           || (root.updateManager && root.updateManager.state === "error")
                           ? Theme.amber : Theme.textDim
                    renderType: Text.NativeRendering
                    font.family: Theme.fontFamily
                    font.pixelSize: 10
                    font.hintingPreference: Font.PreferFullHinting
                    wrapMode: Text.Wrap
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                SoftButton {
                    Layout.preferredWidth: 108
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
                    text: root.updateManager && root.updateManager.busy
                          ? "Memproses…"
                          : root.deviceTransactionBusy
                            ? "Tunggu transaksi"
                            : (root.updateManager && root.updateManager.state === "error"
                               ? "Coba lagi"
                               : "Update sekarang")
                    compact: false
                    mixerSelect: true
                    primaryAction: true
                    checked: true
                    enabled: root.updateManager && !root.updateManager.busy && !root.deviceTransactionBusy
                    onClicked: root.updateManager.downloadAndInstall()
                }
            }

            Text {
                Layout.fillWidth: true
                text: root.updateManager && root.updateManager.installationScope === "user"
                      ? "SonKuPik akan mengunduh Setup resmi, memverifikasi manifest + SHA-256, memasang pembaruan pada akun pengguna tanpa Administrator, lalu membuka aplikasi kembali."
                      : "SonKuPik akan mengunduh Setup resmi, memverifikasi manifest + SHA-256, meminta izin Administrator untuk instalasi Program Files, lalu membuka aplikasi kembali."
                color: Theme.textDim
                renderType: Text.NativeRendering
                font.family: Theme.fontFamily
                font.pixelSize: 9
                font.hintingPreference: Font.PreferFullHinting
                wrapMode: Text.Wrap
            }
        }
    }
}
