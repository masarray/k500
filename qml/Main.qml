import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    required property var studioEngine
    visible: true
    width: 1484
    height: 920
    minimumWidth: 1260
    minimumHeight: 800
    title: "SONKUPIK STUDIO — Karaoke Processor"
    color: Theme.bg
    readonly property int lowerRackHeight: 304
    property bool transportPlaying: false
    property bool transportMuted: false
    property int selectedSection: 0

    background: Rectangle {
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#111820" }
            GradientStop { position: 0.48; color: Theme.bg }
            GradientStop { position: 1.0; color: "#070B0F" }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            radius: 14
            border.width: 1
            border.color: "#28333C"
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#202932" }
                GradientStop { position: 0.18; color: "#161E25" }
                GradientStop { position: 1.0; color: "#0C1116" }
            }
            Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.top:parent.top;anchors.leftMargin:10;anchors.rightMargin:10;height:1;color:"#FFFFFF";opacity:.05 }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 10

                RowLayout {
                    Layout.preferredWidth: 300
                    Layout.minimumWidth: 260
                    spacing: 11
                    Rectangle {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        radius: 6
                        color: "#151A1F"
                        border.width: 1
                        border.color: "#343E47"
                        Image { anchors.fill:parent;anchors.margins:4;source:"qrc:/assets/sonkupik-logo.png";fillMode:Image.PreserveAspectFit;smooth:true }
                    }
                    ColumnLayout {
                        spacing: 0
                        RowLayout {
                            spacing: 3
                            Text {
                                text: "SONKUPIK"
                                color: Theme.text
                                font.family: Theme.displayFamily
                                font.pixelSize: 14
                                font.weight: Font.Bold
                            }
                            Text {
                                text: "STUDIO"
                                color: Theme.amber
                                font.family: Theme.displayFamily
                                font.pixelSize: 14
                                font.weight: Font.Bold
                            }
                        }
                        Text {
                            text: "KARAOKE PROCESSOR"
                            color: Theme.textDim
                            font.family: Theme.monoFamily
                            font.pixelSize: 8
                            font.letterSpacing: 1.35
                        }
                    }
                    Item { Layout.fillWidth: true }
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    Layout.preferredWidth: 148
                    Layout.preferredHeight: 34
                    radius: 9
                    border.width: 1
                    border.color: "#252D34"
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "#151C22" }
                        GradientStop { position: 1.0; color: "#080C10" }
                    }
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 4
                        spacing: 3
                        SoftButton { Layout.preferredWidth:27;Layout.fillHeight:true;transport:true;iconName:"skip-back";iconOnly:true;iconFilled:true }
                        SoftButton {
                            Layout.preferredWidth:31;Layout.fillHeight:true;transport:true
                            iconName:root.transportPlaying?"pause":"play";iconOnly:true;iconFilled:true;neonAccent:true;checked:root.transportPlaying
                            onClicked:root.transportPlaying=!root.transportPlaying
                        }
                        SoftButton { Layout.preferredWidth:27;Layout.fillHeight:true;transport:true;iconName:"skip-forward";iconOnly:true;iconFilled:true }
                        Rectangle { Layout.preferredWidth:1;Layout.preferredHeight:17;color:"#2E3A43" }
                        SoftButton {
                            Layout.preferredWidth:27;Layout.fillHeight:true;transport:true;iconName:"volume-x";iconOnly:true
                            checked:root.transportMuted;danger:root.transportMuted;onClicked:root.transportMuted=!root.transportMuted
                        }
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 124
                    Layout.preferredHeight: 34
                    radius: 8
                    color: "#080C0F"
                    border.width: 1
                    border.color: "#3B3218"
                    Text { anchors.centerIn:parent;text:"DEFAULT FLAT";color:Theme.amber;font.family:Theme.monoFamily;font.pixelSize:11;font.weight:Font.Bold }
                }

                RowLayout {
                    spacing: 5
                    Rectangle { width:7;height:7;radius:4;color:Theme.textFaint }
                    Text { text:"LIVE";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.weight:Font.Bold;font.letterSpacing:.8 }
                    SoftButton { Layout.preferredWidth:48;text:"BT";iconName:"bluetooth";compact:true;checked:true }
                    SoftButton { Layout.preferredWidth:48;text:"USB";iconName:"usb";compact:true }
                    SoftButton { Layout.preferredWidth:74;text:"Connect";iconName:"cable";compact:true }
                }

                Rectangle {
                    Layout.preferredWidth: 72
                    Layout.preferredHeight: 29
                    radius: 8
                    color: "#12130D"
                    border.width: 1
                    border.color: "#493C16"
                    Text { anchors.centerIn:parent;text:"OFFLINE";color:Theme.amber;font.family:Theme.monoFamily;font.pixelSize:8;font.weight:Font.Bold;font.letterSpacing:.7 }
                }

                Item { Layout.fillWidth: true }

                SoftButton { Layout.preferredWidth:80;text:"Import";iconName:"upload";compact:true }
                SoftButton { Layout.preferredWidth:80;text:"Export";iconName:"download";compact:true;checked:true }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            StudioPanel {
                Layout.preferredWidth: 170
                Layout.minimumWidth: 170
                Layout.maximumWidth: 170
                Layout.fillHeight: true
                accentTop: false

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 9
                    spacing: 6

                    Text {
                        text: "SECTIONS"
                        color: Theme.textDim
                        font.family: Theme.monoFamily
                        font.pixelSize: 9
                        font.letterSpacing: 1.35
                        leftPadding: 7
                        topPadding: 2
                        bottomPadding: 4
                    }

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
                            radius: 8
                            gradient: Gradient {
                                orientation: Gradient.Horizontal
                                GradientStop { position:0;color:navItem.active?"#263D42":navPointer.containsMouse?"#1B252D":"#00101518" }
                                GradientStop { position:.55;color:navItem.active?"#172A2E":navPointer.containsMouse?"#131B21":"#00101518" }
                                GradientStop { position:1;color:navItem.active?"#081416":"#00101518" }
                            }
                            border.width: 1
                            border.color: navItem.active ? Theme.accentSoft : "transparent"

                            Rectangle {
                                id: navIconShell
                                x: 7
                                anchors.verticalCenter: parent.verticalCenter
                                width: 28; height: 28; radius: 6
                                color: navItem.active ? "#113338" : "transparent"
                                border.width: 1
                                border.color: navItem.active ? Theme.accentSoft : Theme.borderSoft
                                LucideIcon { anchors.centerIn:parent;width:15;height:15;name:modelData.icon;color:navItem.active?Theme.accent:Theme.textDim;strokeWidth:1.8 }
                            }
                            Column {
                                anchors.left:navIconShell.right;anchors.leftMargin:10
                                anchors.right:parent.right;anchors.rightMargin:5
                                anchors.verticalCenter:parent.verticalCenter
                                spacing:0
                                Text { width:parent.width;text:modelData.name;color:navItem.active?Theme.accent:Theme.text;font.family:Theme.displayFamily;font.pixelSize:12;font.weight:Font.DemiBold }
                                Text { width:parent.width;text:modelData.sub;color:Theme.textDim;font.family:Theme.fontFamily;font.pixelSize:9 }
                            }
                            MouseArea { id:navPointer;anchors.fill:parent;hoverEnabled:true;cursorShape:Qt.PointingHandCursor;onClicked:root.selectedSection=index }
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
                        spacing: 12

                        EqGraph {
                            bandModel: root.studioEngine.musicEqBands
                            hpfFreq: root.studioEngine.hpfHz
                            lpfFreq: root.studioEngine.lpfHz
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.minimumHeight: 0
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: root.lowerRackHeight
                            Layout.minimumHeight: root.lowerRackHeight
                            Layout.maximumHeight: root.lowerRackHeight
                            spacing: 12

                            MusicInputPanel {
                                engine:root.studioEngine
                                Layout.fillWidth:true
                                Layout.fillHeight:true
                                Layout.preferredWidth:557
                                Layout.minimumWidth:440
                            }
                            MusicTonePanel {
                                engine:root.studioEngine
                                Layout.fillWidth:true
                                Layout.fillHeight:true
                                Layout.preferredWidth:316
                                Layout.minimumWidth:240
                            }
                            FilterPanel {
                                engine:root.studioEngine
                                Layout.preferredWidth:218
                                Layout.minimumWidth:200
                                Layout.maximumWidth:230
                                Layout.fillHeight:true
                            }
                            MasterStripPanel {
                                engine:root.studioEngine
                                Layout.preferredWidth:188
                                Layout.minimumWidth:188
                                Layout.maximumWidth:188
                                Layout.fillHeight:true
                            }
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
