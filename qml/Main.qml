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
            GradientStop { position: 0.0; color: "#0C1116" }
            GradientStop { position: 0.48; color: Theme.bg }
            GradientStop { position: 1.0; color: "#04070A" }
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
            border.color: "#27323A"
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#1B242B" }
                GradientStop { position: 0.20; color: "#141C22" }
                GradientStop { position: 0.62; color: "#0D1419" }
                GradientStop { position: 1.0; color: "#070C10" }
            }

            // Web panel-bevel uses both an inset highlight and a dark lower
            // edge.  Recreate those layers explicitly so the toolbar does not
            // read as a flat Qt rectangle.
            Rectangle {
                anchors.fill: parent
                anchors.margins: 1
                radius: 13
                color: "transparent"
                border.width: 1
                border.color: "#0F2FFFFFF"
            }
            Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.top:parent.top;anchors.leftMargin:11;anchors.rightMargin:11;anchors.topMargin:1;height:1;color:"#FFFFFF";opacity:.075 }
            Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;anchors.leftMargin:12;anchors.rightMargin:12;anchors.bottomMargin:1;height:1;color:"#000000";opacity:.58 }

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
                        border.width: 1
                        border.color: "#080B0D"
                        gradient: Gradient {
                            GradientStop { position:0;color:"#323A40" }
                            GradientStop { position:.36;color:"#1A2228" }
                            GradientStop { position:1;color:"#0A0E12" }
                        }
                        Rectangle { anchors.fill:parent;anchors.margins:1;radius:5;color:"transparent";border.width:1;border.color:"#18FFFFFF" }
                        Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.top:parent.top;anchors.leftMargin:5;anchors.rightMargin:5;anchors.topMargin:1;height:1;color:"#FFFFFF";opacity:.13 }
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
                    border.color: "#070A0D"
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "#202930" }
                        GradientStop { position: 0.24; color: "#171F25" }
                        GradientStop { position: 1.0; color: "#070B0F" }
                    }
                    Rectangle { anchors.fill:parent;anchors.margins:1;radius:8;color:"transparent";border.width:1;border.color:"#14FFFFFF" }
                    Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.top:parent.top;anchors.leftMargin:5;anchors.rightMargin:5;anchors.topMargin:1;height:1;color:"#FFFFFF";opacity:.10 }
                    Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;anchors.leftMargin:5;anchors.rightMargin:5;anchors.bottomMargin:1;height:1;color:"#000000";opacity:.60 }
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
                        Rectangle { Layout.preferredWidth:1;Layout.preferredHeight:17;color:"#38454E";opacity:.80 }
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
                    border.width: 1
                    border.color: "#4A3A10"
                    gradient: Gradient {
                        GradientStop { position:0;color:"#17170F" }
                        GradientStop { position:.48;color:"#0B0D09" }
                        GradientStop { position:1;color:"#050706" }
                    }
                    Rectangle { anchors.fill:parent;anchors.margins:1;radius:7;color:"transparent";border.width:1;border.color:"#16FFB200" }
                    Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.top:parent.top;anchors.leftMargin:6;anchors.rightMargin:6;anchors.topMargin:1;height:1;color:Theme.amber;opacity:.12 }
                    Text { anchors.centerIn:parent;text:"DEFAULT FLAT";color:Theme.amber;font.family:Theme.monoFamily;font.pixelSize:11;font.weight:Font.Bold }
                }

                RowLayout {
                    spacing: 5
                    Rectangle {
                        width:7;height:7;radius:4;color:Theme.textFaint
                        Rectangle { anchors.centerIn:parent;width:3;height:3;radius:2;color:"#D0D7DC";opacity:.22 }
                    }
                    Text { text:"LIVE";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.weight:Font.Bold;font.letterSpacing:.8 }
                    SoftButton { Layout.preferredWidth:48;text:"BT";iconName:"bluetooth";compact:true;checked:true }
                    SoftButton { Layout.preferredWidth:48;text:"USB";iconName:"usb";compact:true }
                    SoftButton { Layout.preferredWidth:74;text:"Connect";iconName:"cable";compact:true }
                }

                Rectangle {
                    Layout.preferredWidth: 72
                    Layout.preferredHeight: 29
                    radius: 8
                    border.width: 1
                    border.color: "#493C16"
                    gradient: Gradient {
                        GradientStop { position:0;color:"#17170F" }
                        GradientStop { position:1;color:"#080906" }
                    }
                    Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.top:parent.top;anchors.leftMargin:5;anchors.rightMargin:5;anchors.topMargin:1;height:1;color:Theme.amber;opacity:.10 }
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
                            transformOrigin: Item.Center
                            scale: navPointer.pressed ? 0.987 : 1
                            transform: Translate {
                                y: navPointer.pressed ? 1 : 0
                                Behavior on y { NumberAnimation { duration:55; easing.type:Easing.OutQuad } }
                            }
                            gradient: Gradient {
                                orientation: Gradient.Horizontal
                                // Exact Web intent: cyan at ~12% on the left,
                                // fading almost completely into the rail.
                                GradientStop { position:0;color:navItem.active?"#2024E9F2":navPointer.containsMouse?"#101FFFFFF":"#00101518" }
                                GradientStop { position:.48;color:navItem.active?"#1024E9F2":navPointer.containsMouse?"#091FFFFFF":"#00101518" }
                                GradientStop { position:1;color:navItem.active?"#0024E9F2":"#00101518" }
                            }
                            border.width: 1
                            border.color: navItem.active ? "#6624E9F2" : navPointer.containsMouse ? "#26323B" : "transparent"
                            Behavior on border.color { ColorAnimation { duration:90 } }
                            Behavior on scale { NumberAnimation { duration:55; easing.type:Easing.OutQuad } }

                            Rectangle {
                                anchors.fill:parent
                                anchors.margins:1
                                radius:7
                                color:"transparent"
                                border.width:1
                                border.color:navItem.active?"#1224E9F2":navPointer.containsMouse?"#0DFFFFFF":"transparent"
                            }
                            Rectangle {
                                anchors.left:parent.left
                                anchors.right:parent.right
                                anchors.top:parent.top
                                anchors.leftMargin:8
                                anchors.rightMargin:8
                                anchors.topMargin:1
                                height:1
                                color:navItem.active?Theme.accent:"#FFFFFF"
                                opacity:navItem.active?.17:navPointer.containsMouse?.08:0
                            }

                            Rectangle {
                                id: navIconShell
                                x: 7
                                anchors.verticalCenter: parent.verticalCenter
                                width: 28; height: 28; radius: 6
                                border.width: 1
                                border.color: navItem.active ? "#9924E9F2" : navPointer.containsMouse ? "#384650" : Theme.borderSoft
                                gradient: Gradient {
                                    GradientStop { position:0;color:navItem.active?"#1824E9F2":navPointer.containsMouse?"#15232B31":"#06101518" }
                                    GradientStop { position:1;color:navItem.active?"#0824E9F2":"#00101518" }
                                }
                                Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.top:parent.top;anchors.leftMargin:4;anchors.rightMargin:4;anchors.topMargin:1;height:1;color:navItem.active?Theme.accent:"#FFFFFF";opacity:navItem.active?.25:.05 }
                                LucideIcon { anchors.centerIn:parent;width:15;height:15;name:modelData.icon;color:navItem.active?Theme.accent:navPointer.containsMouse?Theme.textSoft:Theme.textDim;strokeWidth:1.8 }
                            }
                            Column {
                                anchors.left:navIconShell.right;anchors.leftMargin:10
                                anchors.right:parent.right;anchors.rightMargin:5
                                anchors.verticalCenter:parent.verticalCenter
                                spacing:0
                                Text { width:parent.width;text:modelData.name;color:navItem.active?Theme.accent:Theme.text;font.family:Theme.displayFamily;font.pixelSize:12;font.weight:Font.DemiBold }
                                Text { width:parent.width;text:modelData.sub;color:navItem.active?"#9AA8B2":Theme.textDim;font.family:Theme.fontFamily;font.pixelSize:9 }
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
                                Layout.minimumWidth:224
                            }
                            FilterPanel {
                                engine:root.studioEngine
                                Layout.preferredWidth:180
                                Layout.minimumWidth:180
                                Layout.maximumWidth:180
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
