import QtQuick

Rectangle {
    id: root

    property bool inset: false
    property bool accentTop: false
    property color accentColor: Theme.accent

    // Web panel-bevel uses 14 px corners and a quiet 8% highlight border.
    radius: 14
    border.width: 1
    border.color: root.inset ? "#05080A" : "#2A343D"

    gradient: Gradient {
        GradientStop { position: 0.00; color: root.inset ? "#06090C" : "#252E36" }
        GradientStop { position: 0.07; color: root.inset ? "#080B0F" : "#212A32" }
        GradientStop { position: 0.28; color: root.inset ? "#080C10" : "#1B232A" }
        GradientStop { position: 0.72; color: root.inset ? "#070A0D" : "#151C22" }
        GradientStop { position: 1.00; color: root.inset ? "#050709" : "#11171D" }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: parent.radius + 3
        anchors.rightMargin: parent.radius + 3
        anchors.topMargin: 1
        height: 1
        radius: 1
        color: "#FFFFFF"
        opacity: root.inset ? 0.028 : 0.075
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: Math.max(2, parent.radius - 1)
        color: "transparent"
        border.width: 1
        border.color: root.inset ? "#07000000" : "#0DFFFFFF"
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: parent.radius + 6
        anchors.rightMargin: parent.radius + 6
        anchors.bottomMargin: 2
        height: 1
        radius: 1
        color: "#000000"
        opacity: root.inset ? 0.28 : 0.38
    }
}
