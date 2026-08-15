import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    required property var studioEngine
    visible: true
    width: 1540
    height: 940
    minimumWidth: 1260
    minimumHeight: 800
    title: "SONKUPIK STUDIO — Karaoke Processor"
    color: Theme.bg
    readonly property int lowerRackHeight: 286
    property bool transportPlaying: false
    property bool transportMuted: false
    property int selectedSection: 0

    background: Rectangle {
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#10171D" }
            GradientStop { position: 0.48; color: Theme.bg }
            GradientStop { position: 1.0; color: "#070B0F" }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 58
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#1A232B" }
                GradientStop { position: 0.22; color: "#141C23" }
                GradientStop { position: 1.0; color: "#0B1015" }
            }
            Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: 1; color: "#25313A" }
            Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; height: 1; color: "#FFFFFF"; opacity: 0.035 }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                spacing: 8

                Rectangle {
                    Layout.preferredWidth: 38
                    Layout.preferredHeight: 38
                    radius: 7
                    color: Theme.recessed
                    border.width: 1
                    border.color: Theme.border
                    Image { anchors.fill: parent; anchors.margins: 6; source: "qrc:/assets/sonkupik-logo.png"; fillMode: Image.PreserveAspectFit; smooth: true }
                }
                ColumnLayout {
                    spacing: -1
                    Text { text: "SONKUPIK STUDIO"; color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: Theme.textL; font.weight: Font.DemiBold; font.letterSpacing: 0.2 }
                    Text { text: "KARAOKE PROCESSOR"; color: Theme.textDim; font.family: Theme.fontFamily; font.pixelSize: 8; font.weight: Font.Medium; font.letterSpacing: 1.2 }
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    Layout.preferredWidth: 148
                    Layout.preferredHeight: 36
                    radius: 8
                    border.width: 1
                    border.color: "#2A3741"
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "#151E25" }
                        GradientStop { position: 0.45; color: "#0E151B" }
                        GradientStop { position: 1.0; color: "#070B0F" }
                    }
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 4
                        spacing: 3
                        SoftButton { Layout.preferredWidth: 27; Layout.fillHeight: true; transport: true; iconName: "skip-back"; iconOnly: true; iconFilled: true }
                        SoftButton {
                            Layout.preferredWidth: 31; Layout.fillHeight: true; transport: true
                            iconName: root.transportPlaying ? "pause" : "play"; iconOnly: true; iconFilled: true
                            neonAccent: true; checked: root.transportPlaying
                            onClicked: root.transportPlaying = !root.transportPlaying
                        }
                        SoftButton { Layout.preferredWidth: 27; Layout.fillHeight: true; transport: true; iconName: "skip-forward"; iconOnly: true; iconFilled: true }
                        Rectangle { Layout.preferredWidth: 1; Layout.preferredHeight: 16; color: "#2E3A43" }
                        SoftButton {
                            Layout.preferredWidth: 27; Layout.fillHeight: true; transport: true
                            iconName: "volume-x"; iconOnly: true; accentIcon: !root.transportMuted
                            checked: root.transportMuted; danger: root.transportMuted
                            onClicked: root.transportMuted = !root.transportMuted
                        }
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 186
                    Layout.preferredHeight: 34
                    radius: 6
                    color: "#080C0F"
                    border.width: 1
                    border.color: "#373019"
                    Text { anchors.centerIn: parent; text: "DEFAULT FLAT"; color: Theme.amber; font.family: Theme.fontFamily; font.pixelSize: Theme.textS; font.weight: Font.Bold; font.letterSpacing: 0.35 }
                }

                RowLayout {
                    spacing: 4
                    Rectangle { width: 6; height: 6; radius: 3; color: Theme.textFaint }
                    Text { text: "LIVE"; color: Theme.textDim; font.family: Theme.fontFamily; font.pixelSize: 8; font.weight: Font.Bold; font.letterSpacing: 0.8 }
                    SoftButton { Layout.preferredWidth: 48; text: "BT"; iconName: "bluetooth"; compact: true; checked: true }
                    SoftButton { Layout.preferredWidth: 54; text: "USB"; iconName: "usb"; compact: true }
                    SoftButton { Layout.preferredWidth: 86; text: "CONNECT"; iconName: "cable"; compact: true }
                }

                Rectangle {
                    Layout.preferredWidth: 72
                    Layout.preferredHeight: 28
                    radius: 6
                    color: "#12130D"
                    border.width: 1
                    border.color: "#493C16"
                    Text { anchors.centerIn: parent; text: "OFFLINE"; color: Theme.amber; font.family: Theme.fontFamily; font.pixelSize: 8; font.weight: Font.Bold; font.letterSpacing: 0.7 }
                }
                SoftButton { Layout.preferredWidth: 74; text: "IMPORT"; iconName: "upload"; compact: true }
                SoftButton { Layout.preferredWidth: 74; text: "EXPORT"; iconName: "download"; compact: true }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 11
            spacing: 10

            StudioPanel {
                Layout.preferredWidth: 162
                Layout.minimumWidth: 162
                Layout.maximumWidth: 162
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 5
                    Text { text: "SECTIONS"; color: Theme.textDim; font.family: Theme.fontFamily; font.pixelSize: 8; font.weight: Font.DemiBold; font.letterSpacing: 1.2; leftPadding: 6; bottomPadding: 5 }

                    Repeater {
                        model: [
                            {name:"Music", sub:"Source & tone", icon:"music-2"},
                            {name:"Mic", sub:"Dual vocal input", icon:"mic-2"},
                            {name:"Reverb", sub:"Room tail", icon:"sparkles"},
                            {name:"Echo", sub:"Delay engine", icon:"repeat"},
                            {name:"Main", sub:"Front output", icon:"speaker"},
                            {name:"Surround", sub:"Rear field", icon:"waves"},
                            {name:"Center", sub:"Vocal focus", icon:"radio-tower"},
                            {name:"Sub", sub:"Bass management", icon:"activity"},
                            {name:"System", sub:"Global setup", icon:"settings-2"}
                        ]
                        delegate: Rectangle {
                            id: navItem
                            required property var modelData
                            required property int index
                            readonly property bool active: index === root.selectedSection
                            Layout.fillWidth: true
                            Layout.preferredHeight: 47
                            radius: 7
                            color: "transparent"
                            gradient: Gradient {
                                orientation: Gradient.Horizontal
                                GradientStop { position: 0.0; color: navItem.active ? "#263D42" : navPointer.containsMouse ? "#1B252D" : "#00101518" }
                                GradientStop { position: 0.48; color: navItem.active ? "#182D31" : navPointer.containsMouse ? "#131B21" : "#00101518" }
                                GradientStop { position: 1.0; color: navItem.active ? "#07171A" : "#00101518" }
                            }
                            border.width: 1
                            border.color: navItem.active ? Theme.accentSoft : navPointer.containsMouse ? Theme.borderSoft : "transparent"

                            Rectangle {
                                id: navIconShell
                                x: 8
                                anchors.verticalCenter: parent.verticalCenter
                                width: 28; height: 28; radius: 6
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: navItem.active ? "#1B4A4D" : "#182128" }
                                    GradientStop { position: 1.0; color: navItem.active ? "#0C2427" : "#0C1217" }
                                }
                                border.width: 1
                                border.color: navItem.active ? Theme.accentSoft : Theme.borderSoft
                                LucideIcon { anchors.centerIn: parent; width: 15; height: 15; name: modelData.icon; color: navItem.active ? Theme.accent : Theme.textDim; strokeWidth: 1.85 }
                            }
                            Column {
                                anchors.left: navIconShell.right; anchors.leftMargin: 10
                                anchors.right: parent.right; anchors.rightMargin: 6
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 0
                                Text { width: parent.width; text: modelData.name; color: navItem.active ? Theme.accent : Theme.textSoft; font.family: Theme.fontFamily; font.pixelSize: Theme.textS; font.weight: Font.DemiBold }
                                Text { width: parent.width; text: modelData.sub; color: navItem.active ? "#9BB3BA" : Theme.textDim; font.family: Theme.fontFamily; font.pixelSize: 8 }
                            }
                            MouseArea { id: navPointer; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.selectedSection = index }
                        }
                    }
                    Item { Layout.fillHeight: true }
                }
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: root.selectedSection === 0 ? 0 : 1

                Item {
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 10

                        EqGraph {
                            bandModel: root.studioEngine.musicEqBands
                            hpfFreq: root.studioEngine.hpfHz
                            lpfFreq: root.studioEngine.lpfHz
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.minimumHeight: 430
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: root.lowerRackHeight
                            Layout.minimumHeight: root.lowerRackHeight
                            Layout.maximumHeight: root.lowerRackHeight
                            spacing: 10
                            MusicInputPanel { engine: root.studioEngine; Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 455; Layout.minimumWidth: 400 }
                            MusicTonePanel { engine: root.studioEngine; Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 350; Layout.minimumWidth: 310 }
                            FilterPanel { engine: root.studioEngine; Layout.preferredWidth: 236; Layout.minimumWidth: 228; Layout.fillHeight: true }
                            MasterStripPanel { engine: root.studioEngine; Layout.preferredWidth: 212; Layout.minimumWidth: 202; Layout.maximumWidth: 226; Layout.fillHeight: true }
                        }
                    }
                }

                SectionWorkspace {
                    engine: root.studioEngine
                    sectionIndex: root.selectedSection
                }
            }
        }
    }
}
