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
    property bool mixerSelect: false
    signal clicked()

    // MIXER_SOURCE_ILLUMINATED_SELECT_V2
    // Digital-console source selectors read like physical illuminated keys:
    // OFF = dark/raised, ON = cyan/recessed with inverted dark legend.
    // The reversed bevel is intentional so ON looks mechanically pressed in.
    readonly property bool mixerLit: root.mixerSelect && root.checked
    readonly property bool mixerDepressed: root.mixerLit
    readonly property bool activeAccent: root.checked || root.neonAccent
    readonly property color resolvedAccent: root.amber ? Theme.amber : Theme.accent
    readonly property real activeBorderAlpha: root.amber ? .44 : .54
    readonly property bool labelHighlighted: root.mixerSelect ? root.mixerLit
                                                               : root.contextHighlighted || root.activeAccent || root.activeFocus

    activeFocusOnTab: true
    clip: false
    implicitWidth: root.transport ? 29 : root.compact ? 52 : 70
    implicitHeight: root.transport ? 28 : root.compact ? 26 : 30
    radius: root.transport ? 6 : 7
    transformOrigin: Item.Center
    scale: mouse.pressed ? .972 : root.mixerDepressed ? .982 : 1

    border.width: 1
    border.color: root.mixerSelect ? (root.mixerLit ? "#087A82"
                                      : root.activeFocus ? "#56666F"
                                      : mouse.containsMouse ? "#43515A" : "#1D272E")
                 : root.activeFocus ? root.resolvedAccent
                 : root.danger ? "#71323A"
                 : root.activeAccent ? Qt.rgba(root.resolvedAccent.r,root.resolvedAccent.g,root.resolvedAccent.b,root.activeBorderAlpha)
                 : mouse.containsMouse ? (root.toolbar ? "#46545E" : "#3B4851")
                 : root.toolbar ? "#263139" : "#11171C"

    gradient: Gradient {
        GradientStop {
            position: 0
            color: root.mixerSelect ? (root.mixerLit ? (mouse.pressed ? "#17BEC6" : "#19C8D0")
                                                      : mouse.pressed ? "#0A0F13"
                                                      : mouse.containsMouse ? "#182128" : "#0E1419")
                 : root.danger ? "#31181E"
                 : root.activeAccent ? (root.amber ? "#2B2818" : "#193B40")
                 : mouse.pressed ? "#171D22"
                 : mouse.containsMouse ? (root.toolbar ? "#313A42" : "#343E45")
                 : root.toolbar ? "#252E35" : "#2C353C"
        }
        GradientStop {
            position: .18
            color: root.mixerSelect ? (root.mixerLit ? "#25DCE3" : "#0B1115")
                 : root.danger ? "#231218"
                 : root.activeAccent ? (root.amber ? "#18170E" : "#132D32")
                 : mouse.containsMouse ? "#283139"
                 : root.toolbar ? "#1A2228" : "#222A30"
        }
        GradientStop {
            position: .72
            color: root.mixerSelect ? (root.mixerLit ? "#31E8EE" : "#070B0E")
                 : root.danger ? "#10090C"
                 : root.activeAccent ? (root.amber ? "#0C0D08" : "#0C2024")
                 : "#12181D"
        }
        GradientStop {
            position: 1
            color: root.mixerSelect ? (root.mixerLit ? "#3BEFF4" : "#04070A")
                 : root.danger ? "#070507" : "#080C10"
        }
    }

    Rectangle {
        visible: root.toolbar && root.activeAccent && !root.mixerSelect
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
        border.color: root.mixerSelect ? (root.mixerLit ? "#2B073D42" : "#10FFFFFF")
                      : root.activeAccent
                      ? Qt.rgba(root.resolvedAccent.r,root.resolvedAccent.g,root.resolvedAccent.b,root.amber ? .13 : .16)
                      : root.toolbar ? "#0EFFFFFF" : "#12FFFFFF"
    }

    // Recessed mixer-key bevel. Dark top/left + bright bottom/right produces
    // the visual depth cue of a pressed hardware switch while it stays lit.
    Rectangle {
        visible: root.mixerDepressed
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: 2
        anchors.rightMargin: 2
        anchors.topMargin: 1
        height: 2
        radius: 1
        color: "#07545A"
        opacity: .78
    }
    Rectangle {
        visible: root.mixerDepressed
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.leftMargin: 1
        anchors.topMargin: 2
        anchors.bottomMargin: 2
        width: 2
        radius: 1
        color: "#08636A"
        opacity: .62
    }
    Rectangle {
        visible: root.mixerDepressed
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 3
        anchors.rightMargin: 3
        anchors.bottomMargin: 1
        height: 2
        radius: 1
        color: "#A6FCFF"
        opacity: .42
    }
    Rectangle {
        visible: root.mixerDepressed
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.rightMargin: 1
        anchors.topMargin: 3
        anchors.bottomMargin: 3
        width: 1
        radius: 1
        color: "#A6FCFF"
        opacity: .30
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
        color: root.mixerSelect && root.mixerLit ? "#062F33"
             : root.activeAccent ? root.resolvedAccent : "#FFFFFF"
        opacity: root.mixerSelect && root.mixerLit ? .70
               : root.activeAccent ? (root.amber ? .18 : .22)
               : mouse.containsMouse ? .13 : root.toolbar ? .08 : .10
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
        color: root.mixerSelect && root.mixerLit ? "#B9FDFF" : "#000000"
        opacity: root.mixerSelect && root.mixerLit ? .38 : mouse.pressed ? .18 : .50
    }

    Rectangle {
        visible: root.activeAccent && !root.mixerSelect
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
        anchors.verticalCenterOffset: root.mixerDepressed ? 1 : 0
        spacing: root.iconOnly || label.text.length===0 ? 0 : 5

        LucideIcon {
            visible: root.iconName.length > 0
            width: root.transport ? 14 : root.compact ? 13 : 15
            height: width
            anchors.verticalCenter: parent.verticalCenter
            name: root.iconName
            color: root.mixerLit ? "#031013"
                 : root.danger ? "#FFD8D8"
                 : root.activeAccent || root.accentIcon ? root.resolvedAccent
                 : root.toolbar ? "#C3CFD5" : "#D5DDE1"
            strokeWidth: root.transport ? 1.75 : root.toolbar ? 1.70 : 1.75
            filled: root.toolbar ? false : root.iconFilled
        }

        Text {
            id: label
            visible: !root.iconOnly && text.length > 0
            anchors.verticalCenter: parent.verticalCenter
            color: root.mixerSelect ? (root.mixerLit ? "#031013" : "#EFF5F7")
                 : root.danger ? "#FFD8D8"
                 : root.contextHighlighted ? root.resolvedAccent
                 : root.amber && root.activeAccent ? Theme.amber
                 : root.checked ? "#EEF5F6"
                 : root.activeAccent ? "#DDF9F6" : root.toolbar ? "#D2DADF" : "#D8DFE4"
            style: !root.mixerSelect && root.contextHighlighted ? Text.Outline : Text.Normal
            styleColor: !root.mixerSelect && root.contextHighlighted
                        ? Qt.rgba(root.resolvedAccent.r,root.resolvedAccent.g,root.resolvedAccent.b,.32)
                        : "transparent"
            font.family: Theme.fontFamily
            font.pixelSize: root.compact ? Theme.textXS : Theme.textS
            font.weight: root.mixerLit ? Font.Bold : root.labelHighlighted ? Font.DemiBold : Font.Medium
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
    Behavior on scale { NumberAnimation { duration:70; easing.type:Easing.OutQuad } }
}
