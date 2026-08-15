import QtQuick
import QtQuick.Layouts

StudioPanel {
    id: root

    property string eyebrow: ""
    property string title: "Mic Inputs"
    property var channels: []
    property color accentColor: Theme.accent
    property bool compactCluster: title === "Reverb" || title === "Echo"
    readonly property real faderHeight: 160
    accentTop: false

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 35
            Text {
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                text: root.title.toUpperCase()
                color: Theme.text
                font.family: Theme.monoFamily
                font.pixelSize: 10
                font.weight: Font.Bold
                font.letterSpacing: 1.05
            }
            Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:1;color:Theme.borderSoft;opacity:.72 }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 10
            Layout.rightMargin: 10
            Layout.topMargin: 10
            Layout.bottomMargin: 10
            spacing: 2

            Repeater {
                model: root.channels
                delegate: Item {
                    id: channel
                    required property var modelData
                    Layout.fillWidth: !root.compactCluster
                    Layout.preferredWidth: root.compactCluster ? 62 : -1
                    Layout.minimumWidth: root.compactCluster ? 62 : 48
                    Layout.fillHeight: true
                    property real localValue: Number(modelData.value)

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 4

                        // Reserve exactly the same top control slot used by
                        // MusicInputPanel. This keeps every fader track on the
                        // same vertical datum throughout the lower rack.
                        Item {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: Math.max(48,channel.width-4)
                            Layout.preferredHeight: 25
                            Column {
                                anchors.centerIn: parent
                                spacing: -1
                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: String(channel.modelData.label || "")
                                    color: Theme.textDim
                                    font.family: Theme.monoFamily
                                    font.pixelSize: 9
                                    font.weight: Font.DemiBold
                                    font.letterSpacing: .35
                                }
                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    visible: String(channel.modelData.badge || "").length > 0
                                    text: String(channel.modelData.badge || "")
                                    color: Theme.textFaint
                                    font.family: Theme.monoFamily
                                    font.pixelSize: 8
                                }
                            }
                        }

                        Item { Layout.preferredHeight: 4 }

                        StudioFader {
                            Layout.preferredHeight: root.faderHeight
                            Layout.minimumHeight: root.faderHeight
                            Layout.maximumHeight: root.faderHeight
                            Layout.preferredWidth: 48
                            Layout.alignment: Qt.AlignHCenter
                            value: channel.localValue
                            from: Number(channel.modelData.from)
                            to: Number(channel.modelData.to)
                            step: Number(channel.modelData.step || 1)
                            defaultValue: Number(channel.modelData.value)
                            accentColor: root.accentColor
                            onValueEdited: function(v) { channel.localValue = v }
                        }

                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: 58
                            Layout.preferredHeight: 23
                            radius: 8
                            color: "#05080A"
                            border.width: 1
                            border.color: "#020304"
                            Row {
                                anchors.centerIn: parent
                                spacing: 3
                                Text {
                                    text: channel.localValue.toFixed(Number(channel.modelData.decimals || 0))
                                    color: Theme.amber
                                    font.family: Theme.monoFamily
                                    font.pixelSize: 9
                                    font.weight: Font.Bold
                                }
                                Text {
                                    visible: String(channel.modelData.unit || "").length > 0
                                    text: String(channel.modelData.unit || "")
                                    color: Theme.textDim
                                    font.family: Theme.monoFamily
                                    font.pixelSize: 7
                                    anchors.baseline: parent.children[0].baseline
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }
            }

            Item { visible: root.compactCluster; Layout.fillWidth: true; Layout.fillHeight: true }
        }
    }
}
