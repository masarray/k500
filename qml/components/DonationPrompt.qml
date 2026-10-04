import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root
    required property var promptController

    parent: Overlay.overlay
    modal: true
    focus: true
    width: 540
    height: 670
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    padding: 0
    closePolicy: Popup.NoAutoClose

    property int countdownSeconds: 3
    readonly property bool canAcknowledge: countdownSeconds <= 0
    readonly property bool qrisReady: promptController && promptController.qrisAvailable

    Overlay.modal: Rectangle {
        color: "#C0060A0E"
    }

    enter: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0
            to: 1
            duration: 130
            easing.type: Easing.OutCubic
        }
    }
    exit: Transition {
        NumberAnimation {
            property: "opacity"
            from: 1
            to: 0
            duration: 90
            easing.type: Easing.InCubic
        }
    }

    onOpened: {
        countdownSeconds = 3
        countdownTimer.restart()
    }

    background: Item {
        Rectangle {
            anchors.fill: parent
            anchors.margins: -10
            radius: 22
            color: "#66000000"
        }

        Rectangle {
            anchors.fill: parent
            radius: 16
            border.width: 1
            border.color: "#314048"
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#151D23" }
                GradientStop { position: 0.22; color: "#0E151A" }
                GradientStop { position: 1.0; color: "#080D11" }
            }
        }
    }

    Timer {
        id: countdownTimer
        interval: 1000
        repeat: true
        onTriggered: {
            if (root.countdownSeconds > 1) {
                root.countdownSeconds -= 1
            } else {
                root.countdownSeconds = 0
                stop()
            }
        }
    }

    contentItem: ColumnLayout {
        anchors.fill: parent
        anchors.margins: 22
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Rectangle {
                Layout.preferredWidth: 36
                Layout.preferredHeight: 36
                radius: 9
                color: "#0A2225"
                border.width: 1
                border.color: "#26656B"

                Text {
                    anchors.centerIn: parent
                    text: "♡"
                    color: Theme.accent
                    renderType: Text.NativeRendering
                    font.family: Theme.fontFamily
                    font.pixelSize: 21
                    font.weight: Font.DemiBold
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0

                Text {
                    text: "DUKUNG PENGEMBANGAN SONKUPIK"
                    color: Theme.accent
                    renderType: Text.NativeRendering
                    font.family: Theme.monoFamily
                    font.pixelSize: 9
                    font.weight: Font.Bold
                    font.letterSpacing: .8
                }

                Text {
                    text: "Terima kasih sudah menggunakan K500"
                    color: Theme.text
                    renderType: Text.NativeRendering
                    font.family: Theme.fontFamily
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                }
            }
        }

        Text {
            Layout.fillWidth: true
            text: "Jika aplikasi dan preset SonKuPik membantu, dukungan sukarela Anda membantu pengujian, penyempurnaan software, dan pengembangan preset-preset baru."
            color: Theme.textSoft
            renderType: Text.NativeRendering
            font.family: Theme.fontFamily
            font.pixelSize: 10
            font.weight: Font.Medium
            wrapMode: Text.Wrap
            lineHeight: 1.14
        }

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 320
            Layout.preferredHeight: 320
            radius: 14
            color: "#F7F9FA"
            border.width: 1
            border.color: root.qrisReady ? "#7BC8CC" : "#C8D0D5"

            Image {
                anchors.fill: parent
                anchors.margins: 12
                source: root.qrisReady ? SupportLinks.qrisSource : ""
                sourceClipRect: SupportLinks.qrisCrop
                fillMode: Image.PreserveAspectFit
                // QR modules must stay pixel-crisp after scaling.
                smooth: false
                asynchronous: false
                visible: root.qrisReady
            }

            Column {
                anchors.centerIn: parent
                width: parent.width - 44
                spacing: 8
                visible: !root.qrisReady

                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: "QRIS RESMI"
                    color: "#1B252B"
                    font.family: Theme.fontFamily
                    font.pixelSize: 16
                    font.weight: Font.Bold
                }

                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: "Belum dibundel pada build ini.\nTidak ada QR pengganti yang dibuat otomatis."
                    color: "#53616A"
                    font.family: Theme.fontFamily
                    font.pixelSize: 10
                    font.weight: Font.Medium
                    wrapMode: Text.Wrap
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 1

            Text {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: SupportLinks.merchantName
                color: Theme.text
                renderType: Text.NativeRendering
                font.family: Theme.fontFamily
                font.pixelSize: 10
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: "NMID " + SupportLinks.merchantNmid
                color: Theme.textDim
                renderType: Text.NativeRendering
                font.family: Theme.monoFamily
                font.pixelSize: 9
                font.weight: Font.Medium
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 42
                radius: 8
                color: youtubeMouse.containsMouse ? "#151E24" : "#0C1217"
                border.width: 1
                border.color: youtubeMouse.containsMouse ? "#485861" : "#26323A"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 11
                    anchors.rightMargin: 11
                    spacing: 8

                    Text {
                        text: "▶"
                        color: "#FF626B"
                        font.family: Theme.fontFamily
                        font.pixelSize: 10
                        font.weight: Font.Bold
                    }
                    Text {
                        Layout.fillWidth: true
                        text: "Tutorial YouTube SonKuPik"
                        color: Theme.textSoft
                        font.family: Theme.fontFamily
                        font.pixelSize: 10
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }
                    Text {
                        text: "↗"
                        color: Theme.textFaint
                        font.pixelSize: 13
                    }
                }

                MouseArea {
                    id: youtubeMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: Qt.openUrlExternally(SupportLinks.youtubeUrl)
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 42
                radius: 8
                color: tokopediaMouse.containsMouse ? "#151E24" : "#0C1217"
                border.width: 1
                border.color: tokopediaMouse.containsMouse ? "#485861" : "#26323A"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 11
                    anchors.rightMargin: 11
                    spacing: 8

                    Text {
                        text: "●"
                        color: "#42B549"
                        font.family: Theme.fontFamily
                        font.pixelSize: 11
                        font.weight: Font.Bold
                    }
                    Text {
                        Layout.fillWidth: true
                        text: "Order K500 di Tokopedia"
                        color: Theme.textSoft
                        font.family: Theme.fontFamily
                        font.pixelSize: 10
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }
                    Text {
                        text: "↗"
                        color: Theme.textFaint
                        font.pixelSize: 13
                    }
                }

                MouseArea {
                    id: tokopediaMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: Qt.openUrlExternally(SupportLinks.tokopediaUrl)
                }
            }
        }

        Text {
            Layout.fillWidth: true
            text: "Donasi sepenuhnya sukarela. Semua fitur K500 tetap tersedia tanpa donasi."
            color: Theme.textDim
            renderType: Text.NativeRendering
            font.family: Theme.fontFamily
            font.pixelSize: 9
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }

        Item {
            Layout.fillHeight: true
        }

        SoftButton {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 150
            Layout.preferredHeight: 34
            text: root.canAcknowledge ? "OK" : "OK (" + root.countdownSeconds + ")"
            compact: false
            mixerSelect: true
            primaryAction: true
            checked: root.canAcknowledge
            enabled: root.canAcknowledge
            onClicked: {
                if (!root.canAcknowledge)
                    return

                root.promptController.acknowledge()
                root.close()
            }
        }
    }
}
