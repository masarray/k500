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
    signal clicked()

    readonly property bool activeAccent: root.checked || root.neonAccent
    readonly property color resolvedAccent: root.amber ? Theme.amber : Theme.accent

    activeFocusOnTab: true
    clip: false

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            root.clicked()
            event.accepted = true
        }
    }

    implicitWidth: root.transport ? 29 : root.compact ? 52 : 70
    implicitHeight: root.transport ? 28 : root.compact ? 26 : 30
    transformOrigin: Item.Center
    scale: mouse.pressed ? (root.transport ? 0.972 : 0.965) : 1.0
    transform: Translate {
        y: mouse.pressed ? 1 : 0
        Behavior on y { NumberAnimation { duration: 55; easing.type: Easing.OutQuad } }
    }

    radius: root.transport ? 6 : 8
    border.width: 1
    border.color: root.activeFocus ? root.resolvedAccent
                 : root.danger ? "#7C3038"
                 : root.activeAccent ? Qt.rgba(root.resolvedAccent.r, root.resolvedAccent.g, root.resolvedAccent.b, 0.68)
                 : mouse.containsMouse ? "#3D4952"
                 : "#070A0D"

    // Match the Web chrome-button stack: a lighter metallic crown, darker
    // lower face and a restrained cyan/amber active tint.  The old Qt button
    // used a nearly-flat dark fill, which made the transport and toolbar look
    // lifeless next to the Web build.
    gradient: Gradient {
        GradientStop {
            position: 0.0
            color: root.danger ? (mouse.pressed ? "#241116" : "#3B1B22")
                 : root.activeAccent ? (mouse.pressed ? "#17373C" : "#27525A")
                 : mouse.pressed ? "#151B20"
                 : mouse.containsMouse ? "#3B454D"
                 : "#343D44"
        }
        GradientStop {
            position: 0.22
            color: root.danger ? "#2B151A"
                 : root.activeAccent ? "#1D4046"
                 : mouse.containsMouse ? "#303941"
                 : "#2A3239"
        }
        GradientStop {
            position: 0.58
            color: root.danger ? "#1A0D11"
                 : root.activeAccent ? "#163036"
                 : mouse.pressed ? "#0F1418"
                 : "#1A2127"
        }
        GradientStop {
            position: 1.0
            color: root.danger ? "#090608"
                 : root.activeAccent ? "#0A171B"
                 : "#0B1014"
        }
    }

    Behavior on border.color { ColorAnimation { duration: 85 } }
    Behavior on scale { NumberAnimation { duration: 50; easing.type: Easing.OutQuad } }

    // Dark lower lip creates the same physical button depth as the Web
    // box-shadow without requiring a blur effect.
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 2
        anchors.rightMargin: 2
        anchors.bottomMargin: 1
        height: 2
        radius: 1
        color: "#000000"
        opacity: mouse.pressed ? 0.28 : 0.60
    }

    // Fine inner bevel.  In active state this doubles as the cyan inner rim
    // visible on the Web buttons.
    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: Math.max(3, parent.radius - 1)
        color: "transparent"
        border.width: 1
        border.color: root.activeAccent
                      ? Qt.rgba(root.resolvedAccent.r, root.resolvedAccent.g, root.resolvedAccent.b, 0.28)
                      : "#16FFFFFF"
    }

    // Active tint stays inside the control so it remains clean under native
    // DPI scaling and Layout clipping.
    Rectangle {
        anchors.fill: parent
        anchors.margins: 2
        radius: Math.max(2, parent.radius - 2)
        color: root.resolvedAccent
        opacity: root.activeAccent ? (root.transport ? 0.050 : 0.038) : 0
        Behavior on opacity { NumberAnimation { duration: 90 } }
    }

    // Console-style underglow, kept inside the hardware face.
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: root.transport ? 5 : 7
        anchors.rightMargin: root.transport ? 5 : 7
        anchors.bottomMargin: 2
        height: root.transport ? 3 : 2
        radius: height / 2
        color: root.resolvedAccent
        opacity: root.activeAccent ? (mouse.pressed ? 0.17 : root.transport ? 0.34 : 0.24) : 0
        Behavior on opacity { NumberAnimation { duration: 90 } }
    }

    // Metallic crown / inset highlight.
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: 4
        anchors.rightMargin: 4
        anchors.topMargin: 1
        height: 1
        radius: 1
        color: root.activeAccent ? root.resolvedAccent : "#FFFFFF"
        opacity: root.activeAccent ? 0.42 : mouse.containsMouse ? 0.19 : 0.13
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: 7
        anchors.rightMargin: 7
        anchors.topMargin: 2
        height: 1
        radius: 1
        color: "#FFFFFF"
        opacity: root.activeAccent ? 0.08 : mouse.containsMouse ? 0.08 : 0.045
    }

    Row {
        anchors.centerIn: parent
        spacing: root.iconOnly || label.text.length === 0 ? 0 : 5

        Item {
            visible: root.iconName.length > 0
            width: root.transport ? 14 : root.compact ? 13 : 15
            height: width
            anchors.verticalCenter: parent.verticalCenter

            // Two-pass glyph gives the small focused light that makes Web
            // transport controls look illuminated without a large outer glow.
            LucideIcon {
                anchors.centerIn: parent
                width: parent.width + (root.transport ? 3 : 2)
                height: width
                name: root.iconName
                color: root.danger ? Theme.red : root.resolvedAccent
                strokeWidth: root.transport ? 3.0 : 2.8
                filled: root.iconFilled
                opacity: root.activeAccent ? (root.transport ? 0.16 : 0.11) : 0
            }
            LucideIcon {
                anchors.centerIn: parent
                width: parent.width
                height: width
                name: root.iconName
                color: root.danger ? "#FFD8D8"
                     : root.activeAccent || root.accentIcon ? root.resolvedAccent
                     : root.transport ? "#C4D0D6" : "#D0D7DC"
                strokeWidth: root.transport ? 1.9 : 1.8
                filled: root.iconFilled
            }
        }

        Text {
            id: label
            visible: !root.iconOnly && text.length > 0
            anchors.verticalCenter: parent.verticalCenter
            color: root.danger ? "#FFD8D8"
                 : root.checked ? "#F2F7F8"
                 : root.activeAccent ? "#DDF9F6" : "#D8DFE4"
            font.family: Theme.fontFamily
            font.pixelSize: root.compact ? Theme.textXS : Theme.textS
            font.weight: root.checked || root.activeAccent ? Font.DemiBold : Font.Medium
            font.letterSpacing: 0.20
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
}
