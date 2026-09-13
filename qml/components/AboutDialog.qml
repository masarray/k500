import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root

    // ABOUT_FLOATING_CARD_V1
    // A modal floating card that intentionally reuses the K500 chassis language:
    // dark graphite surfaces, restrained cyan edge light, amber product accent,
    // native text rasterization and compact instrument-like link rows.
    parent: Overlay.overlay
    modal: true
    focus: true
    width: 560
    height: 442
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    readonly property string applicationName: "SonKuPik K500"
    readonly property string applicationPurpose: "Control & Tuning Studio untuk KTV Pro K500 Karaoke Processor"
    readonly property string applicationVersion: Qt.application.version && Qt.application.version.length
                                                  ? Qt.application.version : "0.0.0"
    readonly property string youtubeUrl: "https://www.youtube.com/@sonkupik"
    readonly property string tokopediaUrl: "https://www.tokopedia.com/dr-sonkupik/recording-tech-ktv-pro-k500-karaoke-effect-processor-4-input-6-output-digital-mixer-dengan-equalizer-compressor-anti-feedback-crossover-ktv-pro-k500"

    Overlay.modal: Rectangle {
        color: "#B8060A0E"
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 130; easing.type: Easing.OutCubic }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 90; easing.type: Easing.InCubic }
    }

    background: Item {
        Rectangle {
            anchors.fill: parent
            anchors.margins: -10
            radius: 22
            color: "#5C000000"
        }
        Rectangle {
            anchors.fill: parent
            radius: 16
            border.width: 1
            border.color: "#31444D"
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#151D23" }
                GradientStop { position: 0.24; color: "#0E151A" }
                GradientStop { position: 1.0; color: "#070B0F" }
            }
        }
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            anchors.topMargin: 1
            height: 1
            radius: .5
            color: Theme.accent
            opacity: .44
        }
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: 24
            anchors.rightMargin: 24
            anchors.bottomMargin: 1
            height: 1
            color: "#000000"
            opacity: .68
        }
    }

    contentItem: Item {
        anchors.fill: parent

        SoftButton {
            id: closeButton
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.topMargin: 14
            anchors.rightMargin: 14
            width: 30
            height: 30
            text: "×"
            compact: true
            toolbar: true
            onClicked: root.close()
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: 28
            anchors.rightMargin: 28
            anchors.topMargin: 28
            anchors.bottomMargin: 24
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 86
                spacing: 16

                Rectangle {
                    Layout.preferredWidth: 70
                    Layout.preferredHeight: 70
                    radius: 14
                    border.width: 1
                    border.color: "#24323A"
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "#2A333A" }
                        GradientStop { position: 0.42; color: "#151D22" }
                        GradientStop { position: 1.0; color: "#090D10" }
                    }
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 1
                        radius: 13
                        color: "transparent"
                        border.width: 1
                        border.color: "#10FFFFFF"
                    }
                    Image {
                        anchors.fill: parent
                        anchors.margins: 7
                        source: "qrc:/assets/SonKuPik-k500-logo.png"
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 3
                    Text {
                        text: root.applicationName
                        color: Theme.text
                        renderType: Text.NativeRendering
                        font.family: Theme.displayFamily
                        font.pixelSize: 22
                        font.weight: Font.Bold
                        font.hintingPreference: Font.PreferFullHinting
                    }
                    Text {
                        Layout.fillWidth: true
                        text: root.applicationPurpose
                        color: Theme.textDim
                        renderType: Text.NativeRendering
                        font.family: Theme.fontFamily
                        font.pixelSize: 11
                        font.weight: Font.Medium
                        font.hintingPreference: Font.PreferFullHinting
                        wrapMode: Text.WordWrap
                    }
                }

                Rectangle {
                    Layout.alignment: Qt.AlignTop
                    Layout.preferredWidth: 92
                    Layout.preferredHeight: 30
                    radius: 8
                    color: "#09171B"
                    border.width: 1
                    border.color: "#2B5960"
                    Text {
                        anchors.centerIn: parent
                        text: "Versi: " + root.applicationVersion
                        color: Theme.accent
                        renderType: Text.NativeRendering
                        font.family: Theme.monoFamily
                        font.pixelSize: 9
                        font.weight: Font.Bold
                        font.hintingPreference: Font.PreferFullHinting
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                Layout.topMargin: 12
                Layout.bottomMargin: 18
                color: Theme.borderSoft
            }

            Text {
                Layout.fillWidth: true
                text: "ABOUT"
                color: Theme.textFaint
                renderType: Text.NativeRendering
                font.family: Theme.monoFamily
                font.pixelSize: 9
                font.weight: Font.Bold
                font.hintingPreference: Font.PreferFullHinting
                font.letterSpacing: 1.25
            }

            Text {
                Layout.fillWidth: true
                Layout.topMargin: 6
                text: "Aplikasi desktop untuk kontrol, tuning, preset, dan monitoring KTV Pro K500 dalam satu workflow profesional."
                color: Theme.textSoft
                renderType: Text.NativeRendering
                font.family: Theme.fontFamily
                font.pixelSize: 11
                font.weight: Font.Medium
                font.hintingPreference: Font.PreferFullHinting
                wrapMode: Text.WordWrap
                lineHeight: 1.18
            }

            Text {
                Layout.fillWidth: true
                Layout.topMargin: 12
                text: "Copyright (c) 2026, SonKuPik"
                color: Theme.textDim
                renderType: Text.NativeRendering
                font.family: Theme.fontFamily
                font.pixelSize: 10
                font.weight: Font.Medium
                font.hintingPreference: Font.PreferFullHinting
            }

            Item { Layout.preferredHeight: 18 }

            Rectangle {
                id: youtubeRow
                Layout.fillWidth: true
                Layout.preferredHeight: 62
                radius: 11
                color: youtubeMouse.containsMouse ? "#151E24" : "#0C1217"
                border.width: 1
                border.color: youtubeMouse.containsMouse ? "#485861" : "#26323A"
                Behavior on color { ColorAnimation { duration: 80 } }
                Behavior on border.color { ColorAnimation { duration: 80 } }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 12

                    Rectangle {
                        Layout.preferredWidth: 38
                        Layout.preferredHeight: 38
                        radius: 10
                        color: "#231011"
                        border.width: 1
                        border.color: "#6A2A30"
                        Rectangle {
                            anchors.centerIn: parent
                            width: 24
                            height: 17
                            radius: 5
                            color: "#FF4B55"
                            Text {
                                anchors.centerIn: parent
                                anchors.horizontalCenterOffset: 1
                                text: "▶"
                                color: "white"
                                font.pixelSize: 10
                                font.weight: Font.Bold
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: -1
                        Text {
                            text: "YOUTUBE"
                            color: "#FF7A81"
                            renderType: Text.NativeRendering
                            font.family: Theme.monoFamily
                            font.pixelSize: 8
                            font.weight: Font.Bold
                            font.hintingPreference: Font.PreferFullHinting
                            font.letterSpacing: .9
                        }
                        Text {
                            text: "SonKuPik"
                            color: youtubeMouse.containsMouse ? Theme.text : Theme.textSoft
                            renderType: Text.NativeRendering
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                            font.hintingPreference: Font.PreferFullHinting
                        }
                    }

                    Text {
                        text: "↗"
                        color: youtubeMouse.containsMouse ? Theme.accent : Theme.textFaint
                        font.family: Theme.fontFamily
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }
                }

                MouseArea {
                    id: youtubeMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: Qt.openUrlExternally(root.youtubeUrl)
                }
            }

            Rectangle {
                id: tokopediaRow
                Layout.fillWidth: true
                Layout.preferredHeight: 62
                Layout.topMargin: 8
                radius: 11
                color: tokopediaMouse.containsMouse ? "#151E24" : "#0C1217"
                border.width: 1
                border.color: tokopediaMouse.containsMouse ? "#485861" : "#26323A"
                Behavior on color { ColorAnimation { duration: 80 } }
                Behavior on border.color { ColorAnimation { duration: 80 } }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 12

                    Rectangle {
                        Layout.preferredWidth: 38
                        Layout.preferredHeight: 38
                        radius: 10
                        color: "#0B1B11"
                        border.width: 1
                        border.color: "#275D39"

                        Item {
                            anchors.centerIn: parent
                            width: 25
                            height: 25
                            Rectangle {
                                x: 4; y: 8; width: 17; height: 14; radius: 4
                                color: "transparent"
                                border.width: 2
                                border.color: "#42B549"
                            }
                            Rectangle {
                                x: 7; y: 3; width: 11; height: 10; radius: 6
                                color: "transparent"
                                border.width: 2
                                border.color: "#42B549"
                            }
                            Rectangle { x: 8; y: 12; width: 3; height: 3; radius: 2; color: "#42B549" }
                            Rectangle { x: 14; y: 12; width: 3; height: 3; radius: 2; color: "#42B549" }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: -1
                        Text {
                            text: "TOKOPEDIA"
                            color: "#63D47C"
                            renderType: Text.NativeRendering
                            font.family: Theme.monoFamily
                            font.pixelSize: 8
                            font.weight: Font.Bold
                            font.hintingPreference: Font.PreferFullHinting
                            font.letterSpacing: .9
                        }
                        Text {
                            text: "Beli KTV Pro K500"
                            color: tokopediaMouse.containsMouse ? Theme.text : Theme.textSoft
                            renderType: Text.NativeRendering
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                            font.hintingPreference: Font.PreferFullHinting
                        }
                    }

                    Text {
                        text: "↗"
                        color: tokopediaMouse.containsMouse ? Theme.accent : Theme.textFaint
                        font.family: Theme.fontFamily
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }
                }

                MouseArea {
                    id: tokopediaMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: Qt.openUrlExternally(root.tokopediaUrl)
                }
            }

            Item { Layout.fillHeight: true }

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 18
                Text {
                    text: "SONKUPIK · K500 CONTROL SURFACE"
                    color: Theme.textFaint
                    renderType: Text.NativeRendering
                    font.family: Theme.monoFamily
                    font.pixelSize: 8
                    font.weight: Font.Medium
                    font.hintingPreference: Font.PreferFullHinting
                    font.letterSpacing: .9
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: "ESC  CLOSE"
                    color: Theme.textFaint
                    renderType: Text.NativeRendering
                    font.family: Theme.monoFamily
                    font.pixelSize: 8
                    font.weight: Font.Medium
                    font.hintingPreference: Font.PreferFullHinting
                    font.letterSpacing: .8
                }
            }
        }
    }
}
