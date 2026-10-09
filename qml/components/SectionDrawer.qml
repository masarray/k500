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
                renderType: Text.NativeRendering
                font.family: Theme.monoFamily
                font.pixelSize: 10
                font.weight: Font.Medium
                font.hintingPreference: Font.PreferFullHinting
                font.letterSpacing: 0.85
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
                readonly property bool hovered: navPointer.containsMouse
                readonly property bool keyboardFocused: navPointer.activeFocus

                Layout.fillWidth: true
                Layout.preferredHeight: Theme.navItemHeight
                radius: Theme.radius
                color: active ? Theme.navActiveSurface
                              : hovered ? Theme.navHoverSurface : Theme.navIdleSurface
                border.width: 1
                border.color: keyboardFocused ? Theme.focus
                              : active ? "#365860" : hovered ? Theme.border : "transparent"

                // MICRO_TYPE_OPTICAL_POLISH_V1 — retain native text optical
                // position during pointer press; avoid scale/font rerasterization.
                transform: Translate {
                    y: navPointer.pressed ? 1 : 0
                    Behavior on y { NumberAnimation { duration: 50; easing.type: Easing.OutQuad } }
                }

                // One strong focus cue instead of stacked neon outlines.
                Rectangle {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    width: 3
                    height: 27
                    radius: 1.5
                    color: Theme.navAccentRail
                    visible: navItem.active
                }

                Rectangle {
                    id: iconShell
                    anchors.left: parent.left
                    anchors.leftMargin: 9
                    anchors.verticalCenter: parent.verticalCenter
                    width: Theme.navIconSize
                    height: Theme.navIconSize
                    radius: 6
                    color: navItem.active ? "#15343B" : "transparent"
                    border.width: 1
                    border.color: navItem.active ? "#456B74" : Theme.borderSoft

                    LucideIcon {
                        anchors.centerIn: parent
                        width: 16
                        height: 16
                        name: navItem.modelData.icon
                        color: navItem.active ? Theme.accent
                              : navItem.hovered ? Theme.textSoft : Theme.textDim
                        strokeWidth: 1.85
                    }
                }

                Column {
                    anchors.left: iconShell.right
                    anchors.leftMargin: 10
                    anchors.right: parent.right
                    anchors.rightMargin: 6
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 1
                    Text {
                        width: parent.width
                        text: navItem.modelData.name
                        color: navItem.active ? Theme.navActiveText : Theme.text
                        renderType: Text.NativeRendering
                        font.family: Theme.displayFamily
                        font.pixelSize: Theme.navTitleSize
                        font.weight: Font.DemiBold
                        font.hintingPreference: Font.PreferFullHinting
                        elide: Text.ElideRight
                    }
                    Text {
                        width: parent.width
                        text: navItem.modelData.sub
                        color: navItem.active ? Theme.navSubtitleText : Theme.textDim
                        renderType: Text.NativeRendering
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.navSubtitleSize
                        font.weight: Font.Normal
                        font.hintingPreference: Font.PreferFullHinting
                        elide: Text.ElideRight
                    }
                }

                MouseArea {
                    id: navPointer
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    acceptedButtons: Qt.LeftButton
                    onClicked: root.sectionSelected(navItem.index)
                }

                Behavior on color { ColorAnimation { duration: 90 } }
                Behavior on border.color { ColorAnimation { duration: 90 } }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
