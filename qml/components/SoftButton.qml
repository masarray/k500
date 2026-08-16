import QtQuick

Rectangle {
    id: root

    property alias text: label.text
    property bool checked: false
    property bool compact: false
    property bool danger: false
    property bool amber: false
    property string iconName: ""
    property bool iconOnly: false
    property bool neonAccent: false
    property bool iconFilled: false
    property bool transport: false
    property bool accentIcon: false
    property bool toolbar: false
    property bool contextHighlighted: false
    signal clicked()

    readonly property bool activeAccent: root.checked || root.neonAccent
    readonly property color resolvedAccent: root.amber ? Theme.amber : Theme.accent
    readonly property real activeBorderAlpha: root.amber ? .44 : .54
    readonly property bool labelHighlighted: root.contextHighlighted || root.activeAccent || root.activeFocus

    activeFocusOnTab: true
    clip: false
    implicitWidth: root.transport ? 29 : root.compact ? 52 : 70
    implicitHeight: root.transport ? 28 : root.compact ? 26 : 30
    radius: root.transport ? 6 : 7
    transformOrigin: Item.Center
    scale: mouse.pressed ? .982 : 1

    border.width: 1
    border.color: root.activeFocus ? root.resolvedAccent
                 : root.danger ? "#71323A"
                 : root.activeAccent ? Qt.rgba(root.resolvedAccent.r,root.resolvedAccent.g,root.resolvedAccent.b,root.activeBorderAlpha)
                 : mouse.containsMouse ? (root.toolbar ? "#46545E" : "#3B4851")
                 : root.toolbar ? "#263139" : "#11171C"

    gradient: Gradient {
        GradientStop {
            position: 0
            color: root.danger ? "#31181E"
                 : root.activeAccent ? (root.amber ? "#2B2818" : "#193B40")
                 : mouse.pressed ? "#171D22"
                 : mouse.containsMouse ? (root.toolbar ? "#313A42" : "#343E45")
                 : root.toolbar ? "#252E35" : "#2C353C"
        }
        GradientStop {
            position: .22
            color: root.danger ? "#231218"
                 : root.activeAccent ? (root.amber ? "#18170E" : "#132D32")
                 : mouse.containsMouse ? "#283139"
                 : root.toolbar ? "#1A2228" : "#222A30"
        }
        GradientStop {
            position: .70
            color: root.danger ? "#10090C"
                 : root.activeAccent ? (root.amber ? "#0C0D08" : "#0C2024")
                 : "#12181D"
        }
        GradientStop { position: 1; color: root.danger ? "#070507" : "#080C10" }
    }

    Rectangle {
        visible: root.toolbar && root.activeAccent
        anchors.fill: parent
        anchors.margins: -1
        radius: parent.radius + 1
        color: "transparent"
        border.width: 1
        border.color: root.resolvedAccent
        opacity: root.amber ? .055 : .075
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: Math.max(3,parent.radius-1)
        color: "transparent"
        border.width: 1
        border.color: root.activeAccent
                      ? Qt.rgba(root.resolvedAccent.r,root.resolvedAccent.g,root.resolvedAccent.b,root.amber ? .13 : .16)
                      : root.toolbar ? "#0EFFFFFF" : "#12FFFFFF"
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: 5
        anchors.rightMargin: 5
        anchors.topMargin: 1
        height: 1
        radius: 1
        color: root.activeAccent ? root.resolvedAccent : "#FFFFFF"
        opacity: root.activeAccent ? (root.amber ? .18 : .22) : mouse.containsMouse ? .13 : root.toolbar ? .08 : .10
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 4
        anchors.rightMargin: 4
        anchors.bottomMargin: 1
        height: 1
        radius: 1
        color: "#000000"
        opacity: mouse.pressed ? .18 : .50
    }

    Rectangle {
        visible: root.activeAccent
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 2
        width: Math.max(10,parent.width-16)
        height: 1
        radius: .5
        color: root.resolvedAccent
        opacity: root.transport ? .30 : root.amber ? .16 : .20
    }

    Row {
        anchors.centerIn: parent
        spacing: root.iconOnly || label.text.length===0 ? 0 : 5

        LucideIcon {
            visible: root.iconName.length > 0
            width: root.transport ? 14 : root.compact ? 13 : 15
            height: width
            anchors.verticalCenter: parent.verticalCenter
            name: root.iconName
            color: root.danger ? "#FFD8D8"
                 : root.activeAccent || root.accentIcon ? root.resolvedAccent
                 : root.toolbar ? "#C3CFD5" : "#D5DDE1"
            strokeWidth: root.transport ? 1.75 : root.toolbar ? 1.70 : 1.75
            filled: root.toolbar ? false : root.iconFilled
        }

        Text {
            id: label
            visible: !root.iconOnly && text.length > 0
            anchors.verticalCenter: parent.verticalCenter
            color: root.danger ? "#FFD8D8"
                 : root.contextHighlighted ? root.resolvedAccent
                 : root.amber && root.activeAccent ? Theme.amber
                 : root.checked ? "#EEF5F6"
                 : root.activeAccent ? "#DDF9F6" : root.toolbar ? "#D2DADF" : "#D8DFE4"
            style: root.contextHighlighted ? Text.Outline : Text.Normal
            styleColor: root.contextHighlighted ? Qt.rgba(root.resolvedAccent.r,root.resolvedAccent.g,root.resolvedAccent.b,.32) : "transparent"
            font.family: Theme.fontFamily
            font.pixelSize: root.compact ? Theme.textXS : Theme.textS
            font.weight: root.labelHighlighted ? Font.DemiBold : Font.Medium
            font.letterSpacing: .12
            Behavior on color { ColorAnimation { duration:75 } }
            Behavior on styleColor { ColorAnimation { duration:75 } }
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onPressed: root.forceActiveFocus()
        onClicked: root.clicked()
    }

    Keys.onPressed: function(event) {
        if(event.key===Qt.Key_Space||event.key===Qt.Key_Return||event.key===Qt.Key_Enter){root.clicked();event.accepted=true}
    }

    Behavior on border.color { ColorAnimation { duration:80 } }
    Behavior on scale { NumberAnimation { duration:50; easing.type:Easing.OutQuad } }
}
