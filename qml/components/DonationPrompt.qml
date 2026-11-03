import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root
    required property var promptController

    parent: Overlay.overlay
    modal: true
    focus: true
    width: Math.min(640, parent ? Math.max(1, parent.width - 16) : 640)
    height: Math.min(760, parent ? Math.max(1, parent.height - 16) : 760)
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    padding: 0
    // Native Qt Escape handling is reliable even when popup focus moves.
    closePolicy: Popup.CloseOnEscape
    onAboutToHide: {
        if (root.promptController && root.promptController.shouldShow)
            root.promptController.acknowledge()
    }

    // DONATION_DISMISS_ALWAYS_AVAILABLE_V1: no voluntary popup may trap users.
    function dismiss() {
        if (root.promptController)
            root.promptController.acknowledge()
        root.close()
    }
 
    property int countdownSeconds: 3
    readonly property bool canAcknowledge: countdownSeconds <= 0
    readonly property bool qrisReady: promptController && promptController.qrisAvailable

    Overlay.modal: Rectangle {
        color: "#D0060A0E"
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

    onOpened: countdownSeconds = 0

    background: Item {
        Rectangle {
            anchors.fill: parent
            anchors.margins: -12
            radius: 24
            color: "#72000000"
        }

        Rectangle {
            anchors.fill: parent
            radius: 17
            border.width: 1
            border.color: "#31545A"
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#142027" }
                GradientStop { position: 0.20; color: "#0E171C" }
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

    // DONATION_READABLE_HIERARCHY_V2 — QR + gratitude are primary.
    // Supporting copy, links, legal note and dismissal are deliberately quieter.
    contentItem: ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.width < 420 ? 14 : 24
        spacing: 8

        // Fixed close control above the scrollable QRIS content.
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            Item { Layout.fillWidth: true }
            SoftButton {
                Layout.preferredWidth: 36
                Layout.preferredHeight: 34
                text: ""
                iconName: "x"
                iconOnly: true
                compact: true
                toolbar: true
                onClicked: root.dismiss()
            }
        }

        Flickable {
            id: donationScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            contentWidth: width
            contentHeight: donationBody.implicitHeight + 10
            ScrollBar.vertical: ScrollBar {
                policy: donationScroll.contentHeight > donationScroll.height + 1
                        ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
            }
            ColumnLayout {
                id: donationBody
                width: Math.max(1, donationScroll.width - 8)
                spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 42
            spacing: 12

            Rectangle {
                Layout.preferredWidth: 42
                Layout.preferredHeight: 42
                radius: 21
                color: "#24F36B6B"
                border.width: 1
                border.color: "#55F36B6B"

                // LUCIDE_HEART_FILLED_V1 — use the existing Lucide renderer,
                // not a text glyph, so the support mark stays optically crisp.
                LucideIcon {
                    anchors.centerIn: parent
                    width: 25
                    height: 25
                    name: "heart"
                    color: Theme.red
                    strokeWidth: 1.7
                    filled: true
                }
            }

            Text {
                Layout.fillWidth: true
                text: "DUKUNG PENGEMBANGAN SONKUPIK"
                color: Theme.accent
                renderType: Text.NativeRendering
                font.family: Theme.fontFamily
                font.pixelSize: 11
                font.weight: Font.DemiBold
                font.letterSpacing: .72
                elide: Text.ElideRight
            }
        }

        Item { Layout.preferredHeight: 10 }

        Text {
            Layout.fillWidth: true
            text: "Terima kasih sudah menggunakan K500"
            color: "#F1F6F8"
            renderType: Text.NativeRendering
            font.family: Theme.fontFamily
            font.pixelSize: 24
            font.weight: Font.DemiBold
            font.hintingPreference: Font.PreferFullHinting
            wrapMode: Text.Wrap
            lineHeight: 1.08
        }

        Item { Layout.preferredHeight: 8 }

        Text {
            Layout.fillWidth: true
            text: "Jika aplikasi dan preset SonKuPik membantu, donasi sukarela Anda ikut mendukung pengembangan preset-preset baru yang lebih baik."
            color: Theme.textSoft
            renderType: Text.NativeRendering
            font.family: Theme.fontFamily
            font.pixelSize: 14
            font.weight: Font.Medium
            font.hintingPreference: Font.PreferFullHinting
            wrapMode: Text.Wrap
            lineHeight: 1.34
        }

        Item { Layout.preferredHeight: 18 }

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            readonly property real qrSide: Math.min(390, Math.max(180,
                Math.min(donationBody.width - 12, root.height - 290)))
            Layout.preferredWidth: qrSide
            Layout.preferredHeight: qrSide
            radius: 15
            color: "#FAFCFD"
            border.width: 1
            border.color: root.qrisReady ? "#81E9ED" : "#C8D0D5"

            Rectangle {
                anchors.fill: parent
                anchors.margins: -2
                radius: 17
                color: "transparent"
                border.width: 1
                border.color: "#3024E9F2"
                visible: root.qrisReady
            }

            Image {
                anchors.fill: parent
                anchors.margins: 14
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
                width: parent.width - 56
                spacing: 10
                visible: !root.qrisReady

                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: "QRIS RESMI"
                    color: "#1B252B"
                    font.family: Theme.fontFamily
                    font.pixelSize: 18
                    font.weight: Font.DemiBold
                }

                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: "Belum dibundel pada build ini.\nTidak ada QR pengganti yang dibuat otomatis."
                    color: "#53616A"
                    font.family: Theme.fontFamily
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    wrapMode: Text.Wrap
                    lineHeight: 1.25
                }
            }
        }

        Item { Layout.preferredHeight: 14 }

        // DONATION_TEXT_LINKS_V2 — text hyperlinks, not button-shaped cards.
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredHeight: 34
            spacing: 18

            Item {
                id: youtubeLink
                Layout.preferredWidth: youtubeText.implicitWidth + 18
                Layout.preferredHeight: 34
                activeFocusOnTab: true

                Text {
                    id: youtubeText
                    anchors.centerIn: parent
                    text: "Tutorial YouTube SonKuPik"
                    color: youtubeLink.activeFocus || youtubeLinkMouse.containsMouse ? "#A8FAFD" : Theme.accent
                    renderType: Text.NativeRendering
                    font.family: Theme.fontFamily
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    font.underline: true
                }

                MouseArea {
                    id: youtubeLinkMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onPressed: youtubeLink.forceActiveFocus()
                    onClicked: Qt.openUrlExternally(SupportLinks.youtubeUrl)
                }

                Keys.onPressed: function(event) {
                    if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space) {
                        Qt.openUrlExternally(SupportLinks.youtubeUrl)
                        event.accepted = true
                    }
                }
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.preferredHeight: 20
                color: "#3D5360"
            }

            Item {
                id: tokopediaLink
                Layout.preferredWidth: tokopediaText.implicitWidth + 18
                Layout.preferredHeight: 34
                activeFocusOnTab: true

                Text {
                    id: tokopediaText
                    anchors.centerIn: parent
                    text: "Order K500 di Tokopedia"
                    color: tokopediaLink.activeFocus || tokopediaLinkMouse.containsMouse ? "#A8FAFD" : Theme.accent
                    renderType: Text.NativeRendering
                    font.family: Theme.fontFamily
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    font.underline: true
                }

                MouseArea {
                    id: tokopediaLinkMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onPressed: tokopediaLink.forceActiveFocus()
                    onClicked: Qt.openUrlExternally(SupportLinks.tokopediaUrl)
                }

                Keys.onPressed: function(event) {
                    if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space) {
                        Qt.openUrlExternally(SupportLinks.tokopediaUrl)
                        event.accepted = true
                    }
                }
            }
        }

        Item { Layout.preferredHeight: 8 }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#27363E"
        }

        Item { Layout.preferredHeight: 10 }

        Text {
            Layout.fillWidth: true
            text: "Donasi sepenuhnya sukarela. Semua fitur K500 tetap tersedia tanpa donasi."
            color: Theme.textDim
            renderType: Text.NativeRendering
            font.family: Theme.fontFamily
            font.pixelSize: 11
            font.weight: Font.Medium
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }

            }
        }

        // Fixed footer remains available even on short-height displays.
        SoftButton {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 176
            Layout.preferredHeight: 42
            text: "Tutup"
            labelPixelSize: 13
            compact: false
            mixerSelect: true
            primaryAction: true
            checked: root.canAcknowledge
            enabled: true
            onClicked: root.dismiss()
        }
    }
}
