import QtQuick

Rectangle {
    id: root

    property bool inset: false
    // Retained for source compatibility with existing panels. The old cyan
    // full-width top rule is intentionally removed; active accents belong to
    // controls and states, not panel chrome.
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
        GradientStop { position: 1.00; color: root.inset ? "#050709" : "#0D1318" }
    }

    // Neutral machined-metal top bevel. Kept inside the panel bounds so it
    // stays crisp and cannot produce the clipped cyan-line look.
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: parent.radius + 2
        anchors.rightMargin: parent.radius + 2
        height: 1
        color: "#FFFFFF"
        opacity: root.inset ? 0.035 : 0.13
    }

    // Narrow shaded lip directly below the highlight gives the panel a small
    // but visible physical thickness without adding glow or decorative lines.
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 1
        anchors.leftMargin: parent.radius + 4
        anchors.rightMargin: parent.radius + 4
        height: 1
        color: "#000000"
        opacity: root.inset ? 0.22 : 0.28
    }

    // Very restrained side bevels make adjacent modules read as separate
    // rugged metal pieces rather than flat cards.
    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: parent.radius + 3
        anchors.bottomMargin: parent.radius + 3
        width: 1
        color: "#FFFFFF"
        opacity: root.inset ? 0.018 : 0.045
    }
    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: parent.radius + 3
        anchors.bottomMargin: parent.radius + 3
        width: 1
        color: "#000000"
        opacity: root.inset ? 0.32 : 0.46
    }

    // Bottom bevel/shadow provides weight and a slightly thicker console
    // chassis appearance while remaining completely neutral in colour.
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: parent.radius + 2
        anchors.rightMargin: parent.radius + 2
        height: 1
        color: "#000000"
        opacity: root.inset ? 0.48 : 0.72
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: Math.max(1, parent.radius - 1)
        color: "transparent"
        border.width: 1
        border.color: root.inset ? "#08000000" : "#12FFFFFF"
    }
}
