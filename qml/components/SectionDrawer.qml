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
                border.color: active ? "#6324E9F2" : navPointer.containsMouse ? "#293740" : "transparent"
                transformOrigin: Item.Center
                scale: navPointer.pressed ? .988 : 1

                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position:0;color:active?"#2024E9F2":navPointer.containsMouse?"#0CFFFFFF":"#00101418" }
                    GradientStop { position:.62;color:active?"#0C24E9F2":navPointer.containsMouse?"#06FFFFFF":"#00101418" }
                    GradientStop { position:1;color:"#00101418" }
                }

                Rectangle {
                    visible: navItem.active
                    anchors.left: parent.left
                    anchors.leftMargin: 1
                    anchors.verticalCenter: parent.verticalCenter
                    width: 2
                    height: 26
                    radius: 1
                    color: Theme.accent
                    opacity: .78
                }

                Rectangle {
                    id: iconShell
                    anchors.left: parent.left
                    anchors.leftMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    width: 27
                    height: 27
                    radius: 6
                    color: navItem.active ? "#10242B2F" : "#09101518"
                    border.width: 1
                    border.color: navItem.active ? "#8A24E9F2" : navPointer.containsMouse ? "#3A4750" : Theme.borderSoft

                    LucideIcon {
                        anchors.centerIn: parent
                        width: 15
                        height: 15
                        name: navItem.modelData.icon
                        color: navItem.active ? Theme.accent : navPointer.containsMouse ? Theme.textSoft : Theme.textDim
                        strokeWidth: 1.75
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

                Behavior on border.color { ColorAnimation { duration: 85 } }
                Behavior on scale { NumberAnimation { duration: 55; easing.type: Easing.OutQuad } }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
