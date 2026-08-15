import QtQuick

Rectangle {
    id: root

    property bool inset: false
    // Kept for source compatibility. Panel chrome is intentionally neutral;
    // cyan/amber accents belong to controls and active states only.
    property bool accentTop: false
    property color accentColor: Theme.accent

    radius: Theme.radiusLarge
    border.width: 1
    border.color: root.inset ? "#05080A" : "#34404A"

    gradient: Gradient {
        GradientStop { position: 0.00; color: root.inset ? "#06090C" : "#242E37" }
        GradientStop { position: 0.055; color: root.inset ? "#080B0F" : "#202932" }
        GradientStop { position: 0.22; color: root.inset ? "#080C10" : "#1A222A" }
        GradientStop { position: 0.68; color: root.inset ? "#070A0D" : "#141B22" }
        GradientStop { position: 0.93; color: root.inset ? "#06090C" : "#10171D" }
        GradientStop { position: 1.00; color: root.inset ? "#050709" : "#0C1217" }
    }

    // Top machined bevel. It stops before the rounded corners so the panel
    // reads as a single milled part rather than a card with a colored rule.
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
        opacity: root.inset ? 0.028 : 0.115
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: parent.radius + 5
        anchors.rightMargin: parent.radius + 5
        anchors.topMargin: 2
        height: 1
        radius: 1
        color: "#000000"
        opacity: root.inset ? 0.16 : 0.24
    }

    // Inset rim creates thickness on all four sides while preserving the
    // root rectangle's rounded outer border. No child crosses the corners.
    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: Math.max(2, parent.radius - 1)
        color: "transparent"
        border.width: 1
        border.color: root.inset ? "#07000000" : "#13FFFFFF"
    }

    // A recessed lower lip is placed above (not on) the outer border. This
    // keeps the lower rounded corners fully visible and avoids the previously
    // chopped/cut-off appearance at the bottom of rack cards.
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
        opacity: root.inset ? 0.28 : 0.46
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: parent.radius + 8
        anchors.rightMargin: parent.radius + 8
        anchors.bottomMargin: 3
        height: 1
        radius: 1
        color: "#FFFFFF"
        opacity: root.inset ? 0.012 : 0.026
    }
}
