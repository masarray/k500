import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root
    required property var manager

    // SMART_UPDATE_CARD_V1 — one focused action path for non-technical users:
    // discover -> explain -> download -> verify -> elevate -> install -> restart.
    parent: Overlay.overlay
    modal: true
    focus: true
    width: 570
    height: manager && manager.state === "available" ? 446 : 342
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    padding: 0
    closePolicy: manager && manager.busy
                 ? Popup.NoAutoClose
                 : Popup.CloseOnEscape | Popup.CloseOnPressOutside

    Overlay.modal: Rectangle { color: "#B8060A0E" }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 140; easing.type: Easing.OutCubic }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 90; easing.type: Easing.InCubic }
    }

    background: Item {
        Rectangle {
            anchors.fill: parent
            anchors.margins: -10
            radius: 22
            color: "#62000000"
        }
        Rectangle {
            anchors.fill: parent
            radius: 16
            border.width: 1
            border.color: "#314048"
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#151D23" }
                GradientStop { position: 0.28; color: "#0E151A" }
                GradientStop { position: 1.0; color: "#070B0F" }
            }
        }
    }

    contentItem: ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 28
        anchors.rightMargin: 28
        anchors.topMargin: 26
        anchors.bottomMargin: 24
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            spacing: 15

            Rectangle {
                Layout.preferredWidth: 58
                Layout.preferredHeight: 58
                radius: 13
                color: "#0A181C"
                border.width: 1
                border.color: "#2C5A61"

                LucideIcon {
                    anchors.centerIn: parent
                    width: 25
                    height: 25
                    name: manager && manager.state === "error" ? "activity" : "download"
                    color: manager && manager.state === "error" ? Theme.amber : Theme.accent
                    strokeWidth: 1.9
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3

                Text {
                    Layout.fillWidth: true
                    text: manager && manager.state === "error"
                          ? "Update perlu perhatian"
                          : manager && manager.state === "upToDate"
                            ? "Aplikasi sudah terbaru"
                            : manager && manager.state === "available"
                              ? "Update SonKuPik K500 tersedia"
                              : "Memperbarui SonKuPik K500"
                    color: Theme.text
                    renderType: Text.NativeRendering
                    font.family: Theme.displayFamily
                    font.pixelSize: 19
                    font.weight: Font.Bold
                    font.hintingPreference: Font.PreferFullHinting
                }

                Text {
                    Layout.fillWidth: true
                    text: manager ? manager.statusText : ""
                    color: Theme.textSoft
                    renderType: Text.NativeRendering
                    font.family: Theme.fontFamily
                    font.pixelSize: 11
                    font.weight: Font.Medium
                    font.hintingPreference: Font.PreferFullHinting
                    wrapMode: Text.WordWrap
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            Layout.topMargin: 18
            Layout.bottomMargin: 17
            color: Theme.borderSoft
        }

        ColumnLayout {
            Layout.fillWidth: true
            visible: manager && manager.state === "available"
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Rectangle {
                    Layout.preferredHeight: 28
                    Layout.preferredWidth: versionText.implicitWidth + 18
                    radius: 7
                    color: "#0A171B"
                    border.width: 1
                    border.color: "#2B5960"
                    Text {
                        id: versionText
                        anchors.centerIn: parent
                        text: manager ? ("v" + manager.currentVersion + "  →  v" + manager.latestVersion) : ""
                        color: Theme.accent
                        renderType: Text.NativeRendering
                        font.family: Theme.monoFamily
                        font.pixelSize: 10
                        font.weight: Font.Bold
                        font.hintingPreference: Font.PreferFullHinting
                    }
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: "STABLE • VERIFIED SHA-256"
                    color: Theme.textDim
                    renderType: Text.NativeRendering
                    font.family: Theme.monoFamily
                    font.pixelSize: 9
                    font.weight: Font.Bold
                    font.hintingPreference: Font.PreferFullHinting
                    font.letterSpacing: .6
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 202
                radius: 10
                color: "#090D11"
                border.width: 1
                border.color: "#202A31"
                clip: true

                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 12
                    contentWidth: availableWidth

                    Text {
                        width: parent.width
                        text: manager && manager.releaseNotes.length > 0
                              ? manager.releaseNotes
                              : "Pembaruan stabil terbaru siap dipasang."
                        color: Theme.textSoft
                        renderType: Text.NativeRendering
                        font.family: Theme.fontFamily
                        font.pixelSize: 11
                        font.weight: Font.Medium
                        font.hintingPreference: Font.PreferFullHinting
                        wrapMode: Text.WordWrap
                        textFormat: Text.PlainText
                        lineHeight: 1.17
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                text: "Sekali konfirmasi: aplikasi akan download, verifikasi, meminta izin Windows, memasang update, lalu restart otomatis."
                color: Theme.textDim
                renderType: Text.NativeRendering
                font.family: Theme.fontFamily
                font.pixelSize: 10
                font.hintingPreference: Font.PreferFullHinting
                wrapMode: Text.WordWrap
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            visible: manager && (manager.state === "downloading"
                                 || manager.state === "verifying"
                                 || manager.state === "installing")
            spacing: 13

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 10
                radius: 5
                color: "#091015"
                border.width: 1
                border.color: "#233039"
                clip: true

                Rectangle {
                    height: parent.height
                    width: manager && manager.state === "downloading"
                           ? Math.max(0, parent.width * manager.progress)
                           : parent.width
                    radius: 5
                    color: Theme.accent
                    opacity: manager && manager.state === "installing" ? .65 : .88

                    Behavior on width { NumberAnimation { duration: 110; easing.type: Easing.OutCubic } }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: manager && manager.state === "downloading"
                          ? Math.round(manager.progress * 100) + "%"
                          : manager && manager.state === "verifying" ? "VERIFY" : "INSTALL"
                    color: Theme.accent
                    renderType: Text.NativeRendering
                    font.family: Theme.monoFamily
                    font.pixelSize: 11
                    font.weight: Font.Bold
                    font.hintingPreference: Font.PreferFullHinting
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: manager && manager.state === "installing"
                          ? "Jangan matikan komputer"
                          : "Paket resmi GitHub Release"
                    color: Theme.textDim
                    renderType: Text.NativeRendering
                    font.family: Theme.fontFamily
                    font.pixelSize: 10
                    font.hintingPreference: Font.PreferFullHinting
                }
            }

            Text {
                Layout.fillWidth: true
                text: manager && manager.state === "installing"
                      ? "Windows dapat menampilkan UAC untuk izin Administrator karena aplikasi dipasang di Program Files. Setelah itu proses dilanjutkan otomatis."
                      : "File hanya akan dijalankan setelah ukuran dan SHA-256 cocok dengan release manifest serta digest asset GitHub."
                color: Theme.textSoft
                renderType: Text.NativeRendering
                font.family: Theme.fontFamily
                font.pixelSize: 11
                font.hintingPreference: Font.PreferFullHinting
                wrapMode: Text.WordWrap
                lineHeight: 1.15
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            visible: manager && manager.state === "error"
            spacing: 10

            Text {
                Layout.fillWidth: true
                text: manager ? manager.errorMessage : ""
                color: Theme.amber
                renderType: Text.NativeRendering
                font.family: Theme.fontFamily
                font.pixelSize: 11
                font.weight: Font.Medium
                font.hintingPreference: Font.PreferFullHinting
                wrapMode: Text.WordWrap
                lineHeight: 1.15
            }

            Text {
                Layout.fillWidth: true
                text: "Versi yang sedang terpasang tetap aman dan tidak diubah."
                color: Theme.textDim
                renderType: Text.NativeRendering
                font.family: Theme.fontFamily
                font.pixelSize: 10
                font.hintingPreference: Font.PreferFullHinting
            }
        }

        Text {
            Layout.fillWidth: true
            visible: manager && manager.state === "upToDate"
            text: "Tidak ada update yang perlu dipasang."
            color: Theme.textSoft
            renderType: Text.NativeRendering
            font.family: Theme.fontFamily
            font.pixelSize: 11
            font.hintingPreference: Font.PreferFullHinting
        }

        Item { Layout.fillHeight: true; Layout.minimumHeight: 16 }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            visible: manager && !manager.busy

            Item { Layout.fillWidth: true }

            SoftButton {
                visible: manager && manager.state === "available"
                Layout.preferredWidth: 88
                text: "Nanti"
                compact: true
                onClicked: {
                    manager.dismiss()
                    root.close()
                }
            }

            SoftButton {
                visible: manager && manager.state === "available"
                Layout.preferredWidth: 154
                text: "Download & Update"
                compact: true
                mixerSelect: true
                primaryAction: true
                checked: true
                onClicked: manager.downloadAndInstall()
            }

            SoftButton {
                visible: manager && manager.state === "error"
                Layout.preferredWidth: 92
                text: "Tutup"
                compact: true
                onClicked: {
                    manager.dismiss()
                    root.close()
                }
            }

            SoftButton {
                visible: manager && manager.state === "error"
                Layout.preferredWidth: 110
                text: "Coba Lagi"
                compact: true
                mixerSelect: true
                primaryAction: true
                checked: true
                onClicked: manager.checkForUpdates(true)
            }

            SoftButton {
                visible: manager && manager.state === "upToDate"
                Layout.preferredWidth: 92
                text: "OK"
                compact: true
                onClicked: {
                    manager.dismiss()
                    root.close()
                }
            }
        }
    }

    Connections {
        target: manager
        function onUpdateAvailableFound() { root.open() }
        function onAttentionRequired() { root.open() }
    }
}
