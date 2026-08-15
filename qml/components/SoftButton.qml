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
    implicitWidth: root.transport ? 29 : root.compact ? 52 : 70
    implicitHeight: root.transport ? 28 : root.compact ? 26 : 30
    radius: root.transport ? 6 : 7
    transformOrigin: Item.Center
    scale: mouse.pressed ? .975 : 1

    border.width: 1
    border.color: root.activeFocus ? root.resolvedAccent
                 : root.danger ? "#71323A"
                 : root.activeAccent ? Qt.rgba(root.resolvedAccent.r,root.resolvedAccent.g,root.resolvedAccent.b,.62)
                 : mouse.containsMouse ? "#46535D"
                 : "#11171C"

    gradient: Gradient {
        GradientStop {
            position: 0
            color: root.danger ? "#351A20"
                 : root.activeAccent ? "#25484E"
                 : mouse.pressed ? "#1A2025"
                 : mouse.containsMouse ? "#3A444C"
                 : "#303940"
        }
        GradientStop {
            position: .20
            color: root.danger ? "#251319"
                 : root.activeAccent ? "#1A373D"
                 : mouse.containsMouse ? "#2D363D"
                 : "#242C32"
        }
        GradientStop {
            position: .68
            color: root.danger ? "#120B0E"
                 : root.activeAccent ? "#10252A"
                 : "#141A1F"
        }
        GradientStop { position: 1; color: root.danger ? "#080608" : "#090D11" }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: Math.max(3,parent.radius-1)
        color: "transparent"
        border.width: 1
        border.color: root.activeAccent
                      ? Qt.rgba(root.resolvedAccent.r,root.resolvedAccent.g,root.resolvedAccent.b,.20)
                      : "#12FFFFFF"
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
        opacity: root.activeAccent ? .30 : mouse.containsMouse ? .16 : .11
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
        opacity: mouse.pressed ? .22 : .58
    }

    Rectangle {
        visible: root.activeAccent
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 2
        width: Math.max(10,parent.width-14)
        height: 2
        radius: 1
        color: root.resolvedAccent
        opacity: root.transport ? .34 : .22
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
                 : root.transport ? "#C9D3D8" : "#D5DDE1"
            strokeWidth: root.transport ? 1.85 : 1.75
            filled: root.iconFilled
        }

        Text {
            id: label
            visible: !root.iconOnly && text.length > 0
            anchors.verticalCenter: parent.verticalCenter
            color: root.danger ? "#FFD8D8"
                 : root.amber && root.activeAccent ? Theme.amber
                 : root.checked ? "#F1F7F8"
                 : root.activeAccent ? "#DDF9F6" : "#D8DFE4"
            font.family: Theme.fontFamily
            font.pixelSize: root.compact ? Theme.textXS : Theme.textS
            font.weight: root.activeAccent ? Font.DemiBold : Font.Medium
            font.letterSpacing: .15
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

    Behavior on border.color { ColorAnimation { duration:85 } }
    Behavior on scale { NumberAnimation { duration:55; easing.type:Easing.OutQuad } }
}
