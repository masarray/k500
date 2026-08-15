import QtQuick

Rectangle {
    id: root

    property bool inset: false
    property bool accentTop: false
    property color accentColor: Theme.accent

    radius: 14
    border.width: 1
    border.color: root.inset ? "#020304" : "#29323A"

    // Approximate the web panel-bevel cascade: a very small top highlight over
    // surface-raised, falling into the darker panel token.
    gradient: Gradient {
        GradientStop { position: 0.00; color: root.inset ? "#040507" : "#1D242B" }
        GradientStop { position: 0.22; color: root.inset ? "#050709" : "#171D24" }
        GradientStop { position: 0.52; color: root.inset ? "#05080A" : "#141A20" }
        GradientStop { position: 1.00; color: root.inset ? "#040507" : "#11171D" }
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
        opacity: root.inset ? 0.02 : 0.06
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: Math.max(2, parent.radius - 1)
        color: "transparent"
        border.width: 1
        border.color: root.inset ? "#12000000" : "#0BFFFFFF"
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
        opacity: root.inset ? 0.34 : 0.42
    }
}
