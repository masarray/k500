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

    // MIXER_SOURCE_ILLUMINATED_SELECT_V3
    // Digital-console selector semantics: OFF is a dark raised key; ON is a
    // full-face cyan illuminated key with a recessed bevel. The persistent ON
    // state never scales the whole control inward because that exposed a dark
    // rectangular gutter around the cyan face and looked visually broken.
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
    // MIXER_FULL_FACE_ACTIVE_V1 — mouse press may compress briefly, but a
    // selected key remains full-size so cyan fills the complete switch face.
    scale: mouse.pressed ? .985 : 1

    border.width: 1
    border.color: root.mixerSelect ? (root.mixerLit ? "#07949C"
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
            color: root.mixerSelect ? (root.mixerLit ? (mouse.pressed ? "#16BEC6" : "#18CBD2")
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
            color: root.mixerSelect ? (root.mixerLit ? "#22DCE3" : "#0B1115")
                 : root.danger ? "#231218"
                 : root.activeAccent ? (root.amber ? "#18170E" : "#132D32")
                 : mouse.containsMouse ? "#283139"
                 : root.toolbar ? "#1A2228" : "#222A30"
        }
        GradientStop {
            position: .72
            color: root.mixerSelect ? (root.mixerLit ? "#2DE8EE" : "#070B0E")
                 : root.danger ? "#10090C"
                 : root.activeAccent ? (root.amber ? "#0C0D08" : "#0C2024")
                 : "#12181D"
        }
        GradientStop {
            position: 1
            color: root.mixerSelect ? (root.mixerLit ? "#38EEF4" : "#04070A")
                 : root.danger ? "#070507" : "#080C10"
        }
    }

    // A restrained one-pixel halo belongs outside the active face. This gives
    // the illuminated-console look without placing a second rectangle inside
    // the cyan button surface.
    Rectangle {
        visible: root.mixerLit
        anchors.fill: parent
        anchors.margins: -2
        radius: parent.radius + 2
        color: "transparent"
        border.width: 1
        border.color: Theme.accent
        opacity: .22
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
        // No inner frame while a mixer key is lit; it was the visible defect
        // reported around BT/other active source keys.
        border.width: root.mixerLit ? 0 : 1
        border.color: root.activeAccent
                      ? Qt.rgba(root.resolvedAccent.r,root.resolvedAccent.g,root.resolvedAccent.b,root.amber ? .13 : .16)
                      : root.toolbar ? "#0EFFFFFF" : "#12FFFFFF"
    }

    // Recessed mixer-key bevel. Dark top/left + bright bottom/right produces
    // depth without shrinking the actual cyan button face.
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
        opacity: .62
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
        opacity: .46
    }
    Rectangle {
        visible: root.mixerDepressed
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 3
        anchors.rightMargin: 3
        anchors.bottomMargin: 1
        height: 1
        radius: 1
        color: "#C6FEFF"
        opacity: .48
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
        color: "#B8FDFF"
        opacity: .32
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
        color: root.mixerSelect && root.mixerLit ? "#063C40"
             : root.activeAccent ? root.resolvedAccent : "#FFFFFF"
        opacity: root.mixerSelect && root.mixerLit ? .55
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
        color: root.mixerSelect && root.mixerLit ? "#D1FEFF" : "#000000"
        opacity: root.mixerSelect && root.mixerLit ? .46 : mouse.pressed ? .18 : .50
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
