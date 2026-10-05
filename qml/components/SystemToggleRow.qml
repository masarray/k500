import QtQuick
import QtQuick.Layouts

// SYSTEM_SETTING_TOGGLE_ROW_V1
// Reusable hardware-backed boolean setting. The whole row is clickable.
// When hardware is not READY, click is still acknowledged via blockedClicked()
// so the UI can explain the gate without falsifying device state.
Rectangle {
    id: root

    property string title: ""
    property string detail: ""
    property string statusText: ""
    property string iconName: ""
    property bool checked: false
    property bool available: false
    property bool pending: false
    property color statusColor: Theme.textDim

    signal toggleRequested(bool checked)
    signal blockedClicked()

    implicitHeight: 50
    radius: 8
    color: root.checked ? "#102329"
                        : interactionMouse.pressed ? "#151D23"
                        : interactionMouse.containsMouse ? "#111920"
                        : "#0D1318"
    border.width: 1
    border.color: root.pending ? Theme.amber
                               : root.checked ? Theme.accentSoft
                               : interactionMouse.containsMouse ? "#394852"
                               : Theme.borderSoft
    opacity: root.available || root.checked ? 1.0 : 0.84

    Behavior on color { ColorAnimation { duration: 90 } }
    Behavior on border.color { ColorAnimation { duration: 90 } }
    Behavior on opacity { NumberAnimation { duration: 90 } }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 9
        anchors.rightMargin: 9
        spacing: 8

        Rectangle {
            visible: root.iconName.length > 0
            Layout.preferredWidth: 26
            Layout.preferredHeight: 26
            Layout.alignment: Qt.AlignVCenter
            radius: 7
            color: root.checked ? "#123038" : "#0A0F13"
            border.width: 1
            border.color: root.checked ? Theme.accentSoft : "#29343C"

            LucideIcon {
                anchors.centerIn: parent
                width: 14
                height: 14
                name: root.iconName
                color: root.checked ? Theme.accent : Theme.textDim
                strokeWidth: 1.7
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 2

            Text {
                Layout.fillWidth: true
                text: root.title
                color: root.checked ? Theme.text : Theme.textSoft
                font.family: Theme.fontFamily
                font.pixelSize: 9
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 7

                Text {
                    Layout.fillWidth: true
                    text: root.detail
                    color: Theme.textDim
                    font.family: Theme.fontFamily
                    font.pixelSize: 7
                    elide: Text.ElideRight
                }

                Text {
                    visible: root.statusText.length > 0
                    Layout.maximumWidth: 112
                    text: root.pending ? "APPLYING…" : root.statusText
                    color: root.pending ? Theme.amber : root.statusColor
                    font.family: Theme.monoFamily
                    font.pixelSize: 7
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }
            }
        }

        Rectangle {
            Layout.preferredWidth: 38
            Layout.preferredHeight: 20
            Layout.alignment: Qt.AlignVCenter
            radius: 10
            color: root.pending ? "#30270E"
                                : root.checked ? "#153A40"
                                : "#070C10"
            border.width: 1
            border.color: root.pending ? Theme.amber
                                       : root.checked ? Theme.accent
                                       : root.available ? "#46545E" : "#303A42"

            Rectangle {
                width: 14
                height: 14
                radius: 7
                y: 3
                x: root.checked ? parent.width - width - 3 : 3
                color: root.pending ? Theme.amber
                                    : root.checked ? Theme.accent
                                    : root.available ? "#93A1AA" : "#697780"
                Behavior on x { NumberAnimation { duration: 110; easing.type: Easing.OutCubic } }
                Behavior on color { ColorAnimation { duration: 90 } }
            }
        }
    }

    MouseArea {
        id: interactionMouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            if (root.available && !root.pending)
                root.toggleRequested(!root.checked)
            else
                root.blockedClicked()
        }
    }
}
