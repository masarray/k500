import QtQuick

Rectangle {
    id: root

    property bool inset: false
    property bool accentTop: false
    property color accentColor: Theme.accent

    radius: 14
    border.width: 1
    border.color: root.inset ? "#020304" : "#252F36"

    // Console-panel depth: restrained metal crown, neutral mid body, dark lower falloff.
    gradient: Gradient {
        GradientStop { position: 0.00; color: root.inset ? "#040507" : "#1B232A" }
        GradientStop { position: 0.22; color: root.inset ? "#050709" : "#171E24" }
        GradientStop { position: 0.56; color: root.inset ? "#05080A" : "#141A20" }
        GradientStop { position: 1.00; color: root.inset ? "#040507" : "#10161B" }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: parent.radius + 4
        anchors.rightMargin: parent.radius + 4
        anchors.topMargin: 1
        height: 1
        radius: .5
        color: root.accentTop ? root.accentColor : "#FFFFFF"
        opacity: root.inset ? 0.018 : root.accentTop ? 0.11 : 0.045
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: Math.max(2, parent.radius - 1)
        color: "transparent"
        border.width: 1
        border.color: root.inset ? "#10000000" : "#09FFFFFF"
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: parent.radius + 7
        anchors.rightMargin: parent.radius + 7
        anchors.bottomMargin: 2
        height: 1
        radius: .5
        color: "#000000"
        opacity: root.inset ? 0.30 : 0.38
    }
}
