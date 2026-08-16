import QtQuick
import QtQuick.Layouts

Item {
    id: root
    property string label: "INPUT"
    property real value: -3
    property real from: -12
    property real to: 12
    property color accentColor: Theme.accent
    property bool active: false
    property bool selected: false
    signal valueEdited(real newValue)
    signal activated()

    implicitWidth: 76
    implicitHeight: 238

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        SoftButton {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 58
            Layout.preferredHeight: 25
            text: root.label
            compact: true
            checked: root.active
            contextHighlighted: inputFader.highlighted
            onClicked: {
                root.active = !root.active
                root.activated()
            }
        }

        Item { Layout.preferredHeight: 4 }

        StudioFader {
            id: inputFader
            Layout.preferredHeight: 160
            Layout.minimumHeight: 160
            Layout.maximumHeight: 160
            Layout.preferredWidth: 48
            Layout.alignment: Qt.AlignHCenter
            value: root.value
            from: root.from
            to: root.to
            defaultValue: 0
            step: 0.5
            accentColor: root.accentColor
            selected: root.selected
            onActivated: root.activated()
            onValueEdited: function(v) { root.valueEdited(v) }
        }

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 58
            Layout.preferredHeight: 23
            radius: 8
            color: inputFader.highlighted ? "#081013" : "#05080A"
            border.width: 1
            border.color: inputFader.highlighted ? root.accentColor : "#020304"
            Row {
                anchors.centerIn: parent
                spacing: 3
                Text {
                    text: root.value <= root.from + 0.1 ? "-∞" : root.value.toFixed(0)
                    color: Theme.amber
                    font.family: Theme.monoFamily
                    font.pixelSize: 9
                    font.weight: Font.Bold
                }
                Text {
                    visible: root.value > root.from + 0.1
                    text: "dB"
                    color: inputFader.highlighted ? Theme.textSoft : Theme.textDim
                    font.family: Theme.monoFamily
                    font.pixelSize: 7
                    anchors.baseline: parent.children[0].baseline
                    Behavior on color { ColorAnimation { duration:75 } }
                }
            }
            Behavior on border.color { ColorAnimation { duration: 75 } }
        }

        Item { Layout.fillHeight: true }
    }
}
