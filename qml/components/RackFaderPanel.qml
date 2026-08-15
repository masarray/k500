import QtQuick
import QtQuick.Layouts

StudioPanel {
    id: root

    property string eyebrow: ""
    property string title: "Mic Inputs"
    property var channels: []
    property color accentColor: Theme.accent
    property bool compactCluster: title === "Reverb" || title === "Echo"
    readonly property real webFaderHeight: title === "Startup Limits" ? 150 : 126
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
            Layout.topMargin: 8
            Layout.bottomMargin: 8
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

                        Item { Layout.fillHeight: true }

                        ColumnLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: -1
                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: String(channel.modelData.label || "")
                                color: Theme.textDim
                                font.family: Theme.monoFamily
                                font.pixelSize: 10
                                font.weight: Font.DemiBold
                                font.letterSpacing: .45
                            }
                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                visible: String(channel.modelData.badge || "").length > 0
                                text: String(channel.modelData.badge || "")
                                color: Theme.textFaint
                                font.family: Theme.monoFamily
                                font.pixelSize: 9
                            }
                        }

                        StudioFader {
                            Layout.preferredHeight: root.webFaderHeight
                            Layout.minimumHeight: root.webFaderHeight
                            Layout.maximumHeight: root.webFaderHeight
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
