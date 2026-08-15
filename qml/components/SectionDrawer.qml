import QtQuick
import QtQuick.Layouts

StudioPanel {
    id: root
    property int selectedSection: 0
    signal sectionSelected(int index)
    accentTop: false

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 4

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            Text {
                anchors.left: parent.left
                anchors.leftMargin: 7
                anchors.verticalCenter: parent.verticalCenter
                text: "SECTIONS"
                color: Theme.textDim
                font.family: Theme.monoFamily
                font.pixelSize: 9
                font.weight: Font.Medium
                font.letterSpacing: 1.35
            }
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
                Layout.preferredHeight: 48
                radius: 8
                border.width: 1
                border.color: active ? "#4724E9F2" : navPointer.containsMouse ? "#27343D" : "transparent"
                transformOrigin: Item.Center
                scale: navPointer.pressed ? .991 : 1

                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position:0;color:active?"#1724E9F2":navPointer.containsMouse?"#09FFFFFF":"#00101418" }
                    GradientStop { position:.58;color:active?"#0924E9F2":navPointer.containsMouse?"#04FFFFFF":"#00101418" }
                    GradientStop { position:1;color:"#00101418" }
                }

                Rectangle {
                    visible: navItem.active
                    anchors.left: parent.left
                    anchors.leftMargin: 1
                    anchors.verticalCenter: parent.verticalCenter
                    width: 1
                    height: 25
                    radius: .5
                    color: Theme.accent
                    opacity: .58
                }

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 1
                    radius: 7
                    color: "transparent"
                    border.width: 1
                    border.color: navItem.active ? "#1024E9F2" : navPointer.containsMouse ? "#0AFFFFFF" : "transparent"
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    anchors.topMargin: 1
                    height: 1
                    radius: .5
                    color: navItem.active ? Theme.accent : "#FFFFFF"
                    opacity: navItem.active ? .11 : navPointer.containsMouse ? .05 : 0
                }

                Rectangle {
                    id: iconShell
                    anchors.left: parent.left
                    anchors.leftMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    width: 27
                    height: 27
                    radius: 6
                    color: navItem.active ? "#0C242B2F" : "#07101518"
                    border.width: 1
                    border.color: navItem.active ? "#6824E9F2" : navPointer.containsMouse ? "#35434C" : Theme.borderSoft

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.leftMargin: 4
                        anchors.rightMargin: 4
                        anchors.topMargin: 1
                        height: 1
                        radius: .5
                        color: navItem.active ? Theme.accent : "#FFFFFF"
                        opacity: navItem.active ? .15 : .04
                    }

                    LucideIcon {
                        anchors.centerIn: parent
                        width: 15
                        height: 15
                        name: navItem.modelData.icon
                        color: navItem.active ? Theme.accent : navPointer.containsMouse ? Theme.textSoft : Theme.textDim
                        strokeWidth: 1.7
                    }
                }

                Column {
                    anchors.left: iconShell.right
                    anchors.leftMargin: 10
                    anchors.right: parent.right
                    anchors.rightMargin: 7
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: -1
                    Text {
                        width: parent.width
                        text: navItem.modelData.name
                        color: navItem.active ? Theme.accent : Theme.text
                        font.family: Theme.displayFamily
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                    }
                    Text {
                        width: parent.width
                        text: navItem.modelData.sub
                        color: Theme.textDim
                        font.family: Theme.fontFamily
                        font.pixelSize: 9
                        elide: Text.ElideRight
                    }
                }

                MouseArea {
                    id: navPointer
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.sectionSelected(navItem.index)
                }

                Behavior on border.color { ColorAnimation { duration: 80 } }
                Behavior on scale { NumberAnimation { duration: 50; easing.type: Easing.OutQuad } }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
