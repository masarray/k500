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
    property bool primaryAction: false
    signal clicked()

    // MIXER_SOURCE_RAISED_ACTIVE_V5
    // Hardware-console keys must read as pushable controls before the pointer
    // ever reaches them. OFF therefore uses a clearly raised graphite face;
    // ON keeps the whole face illuminated cyan. Depth comes from a top specular
    // edge + lower lip, never from an inset rectangle inside the button.
    readonly property bool mixerLit: root.mixerSelect && root.checked
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

    // MIXER_RAISED_SURFACE_V1
    // Only the physical press compresses/moves the key. A selected key never
    // remains shrunken, so its illuminated face always fills the full control.
    scale: mouse.pressed ? .988 : 1
    transform: Translate {
        id: pressShift
        y: mouse.pressed ? 1 : 0
        Behavior on y { NumberAnimation { duration:55; easing.type:Easing.OutQuad } }
    }

    border.width: 1
    border.color: root.mixerSelect ? (root.mixerLit ? "#7BFAFD"
                                      : root.primaryAction ? "#259BA3"
                                      : root.activeFocus ? "#60737D"
                                      : mouse.containsMouse ? "#566772" : "#34434C")
                 : root.activeFocus ? root.resolvedAccent
                 : root.danger ? "#71323A"
                 : root.activeAccent ? Qt.rgba(root.resolvedAccent.r,root.resolvedAccent.g,root.resolvedAccent.b,root.activeBorderAlpha)
                 : mouse.containsMouse ? (root.toolbar ? "#46545E" : "#3B4851")
                 : root.toolbar ? "#263139" : "#11171C"

    gradient: Gradient {
        GradientStop {
            position: 0
            color: root.mixerSelect ? (root.mixerLit ? (mouse.pressed ? "#45E6EA" : "#6AF7F9")
                                                      : root.primaryAction ? (mouse.pressed ? "#15262C" : "#284149")
                                                      : mouse.pressed ? "#12191E"
                                                      : mouse.containsMouse ? "#303B43" : "#263139")
                 : root.danger ? "#31181E"
                 : root.activeAccent ? (root.amber ? "#2B2818" : "#193B40")
                 : mouse.pressed ? "#171D22"
                 : mouse.containsMouse ? (root.toolbar ? "#313A42" : "#343E45")
                 : root.toolbar ? "#252E35" : "#2C353C"
        }
        GradientStop {
            position: .46
            color: root.mixerSelect ? (root.mixerLit ? "#31E0E5"
                                                      : root.primaryAction ? "#122128" : "#141C21")
                 : root.danger ? "#231218"
                 : root.activeAccent ? (root.amber ? "#18170E" : "#132D32")
                 : mouse.containsMouse ? "#283139"
                 : root.toolbar ? "#1A2228" : "#222A30"
        }
        GradientStop {
            position: 1
            color: root.mixerSelect ? (root.mixerLit ? "#14B4BC"
                                                      : root.primaryAction ? "#070C10" : "#060A0D")
                 : root.danger ? "#070507" : "#080C10"
        }
    }

    // Raised-console treatment: one bright top edge and one dark lower lip.
    // These are directional depth cues, not an interior frame/rectangle.
    Rectangle {
        visible: root.mixerSelect
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: 5
        anchors.rightMargin: 5
        anchors.topMargin: 1
        height: 1
        radius: 1
        color: root.mixerLit ? "#E7FFFF" : root.primaryAction ? "#8FE8EA" : "#FFFFFF"
        opacity: root.mixerLit ? .62 : root.primaryAction ? .24 : .17
    }

    Rectangle {
        visible: root.mixerSelect
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 3
        anchors.rightMargin: 3
        anchors.bottomMargin: 1
        height: 2
        radius: 1
        color: root.mixerLit ? "#08717A" : "#000000"
        opacity: mouse.pressed ? .26 : root.mixerLit ? .52 : .68
    }

    // Small shadow/lip extends outside the face so the key sits above the panel.
    // Keeping this outside the button avoids the boxed-in artifact from V3.
    Rectangle {
        visible: root.mixerSelect && !mouse.pressed
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 5
        anchors.rightMargin: 5
        anchors.bottomMargin: -2
        height: 2
        radius: 1
        color: "#000000"
        opacity: .62
    }

    // Normal buttons keep their subtle glass treatment. Hardware/mixer keys use
    // the directional raised treatment above and never receive an inset frame.
    Rectangle {
        visible: !root.mixerSelect
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
        visible: !root.mixerSelect
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: 5
        anchors.rightMargin: 5
        anchors.topMargin: 1
        height: 1
        radius: 1
        color: root.activeAccent ? root.resolvedAccent : "#FFFFFF"
        opacity: root.activeAccent ? (root.amber ? .18 : .22)
               : mouse.containsMouse ? .13 : root.toolbar ? .08 : .10
    }

    Rectangle {
        visible: !root.mixerSelect
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
        spacing: root.iconOnly || label.text.length===0 ? 0 : 5

        LucideIcon {
            visible: root.iconName.length > 0
            width: root.transport ? 14 : root.compact ? 13 : 15
            height: width
            anchors.verticalCenter: parent.verticalCenter
            name: root.iconName
            color: root.mixerLit ? "#021012"
                 : root.primaryAction ? Theme.accent
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
            color: root.mixerSelect ? (root.mixerLit ? "#021012" : root.primaryAction ? "#9EF9FB" : "#EFF5F7")
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
            font.weight: root.mixerLit || root.primaryAction ? Font.Bold : root.labelHighlighted ? Font.DemiBold : Font.Medium
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
    Behavior on scale { NumberAnimation { duration:65; easing.type:Easing.OutQuad } }
}
