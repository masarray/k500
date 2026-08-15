import QtQuick
import QtQuick.Layouts

StudioPanel {
    id: root

    property string eyebrow: "INPUT MIXER"
    property string title: "Mic Inputs"
    property var channels: []
    property color accentColor: Theme.accent

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 11
        spacing: 7

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0
            Text {
                text: root.eyebrow.toUpperCase()
                color: Theme.textDim
                font.family: Theme.fontFamily
                font.pixelSize: 8
                font.weight: Font.DemiBold
                font.letterSpacing: 1.0
            }
            Text {
                text: root.title.toUpperCase()
                color: Theme.text
                font.family: Theme.fontFamily
                font.pixelSize: 10
                font.weight: Font.Bold
                font.letterSpacing: .35
            }
        }

        Rectangle { Layout.fillWidth:true; Layout.preferredHeight:1; color:Theme.borderSoft; opacity:.78 }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 3

            Repeater {
                model: root.channels
                delegate: Item {
                    id: channel
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumWidth: 60
                    property real localValue: Number(modelData.value)

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 4

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: String(channel.modelData.label || "")
                            color: Theme.textDim
                            font.family: Theme.fontFamily
                            font.pixelSize: 8
                            font.weight: Font.DemiBold
                            font.letterSpacing: .45
                        }
                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            visible: String(channel.modelData.badge || "").length > 0
                            text: String(channel.modelData.badge || "")
                            color: Theme.textFaint
                            font.family: Theme.fontFamily
                            font.pixelSize: 7
                        }

                        StudioFader {
                            Layout.fillHeight: true
                            Layout.preferredWidth: 62
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
                            radius: 5
                            color: "#080C10"
                            border.width: 1
                            border.color: Theme.borderSoft
                            Row {
                                anchors.centerIn: parent
                                spacing: 3
                                Text {
                                    text: channel.localValue.toFixed(Number(channel.modelData.decimals || 0))
                                    color: Theme.amber
                                    font.family: Theme.fontFamily
                                    font.pixelSize: 9
                                    font.weight: Font.Bold
                                }
                                Text {
                                    visible: String(channel.modelData.unit || "").length > 0
                                    text: String(channel.modelData.unit || "")
                                    color: Theme.textDim
                                    font.family: Theme.fontFamily
                                    font.pixelSize: 7
                                    anchors.baseline: parent.children[0].baseline
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
