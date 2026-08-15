import QtQuick
import QtQuick.Layouts

StudioPanel {
    id: root
    required property var engine
    implicitHeight: 304
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
                text: "MASTER STRIP"
                color: Theme.text
                font.family: Theme.monoFamily
                font.pixelSize: 10
                font.weight: Font.Bold
                font.letterSpacing: 1.05
            }
            Rectangle {
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                width: 8; height: 8; radius: 4
                color: Theme.amber
            }
            Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:1;color:Theme.borderSoft;opacity:.78 }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 7
            Layout.rightMargin: 7
            Layout.topMargin: 8
            Layout.bottomMargin: 8
            spacing: 1

            Repeater {
                model: [
                    {label:"MUSIC", value:root.engine.masterMusic, field:"music"},
                    {label:"MIC", value:root.engine.masterMic, field:"mic"},
                    {label:"FX", value:root.engine.masterFx, field:"fx"}
                ]
                delegate: Item {
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 4
                        Item { Layout.fillHeight: true }
                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: modelData.label
                            color: Theme.textDim
                            font.family: Theme.monoFamily
                            font.pixelSize: 9
                        }
                        StudioFader {
                            Layout.preferredHeight: 126
                            Layout.minimumHeight: 126
                            Layout.maximumHeight: 126
                            Layout.preferredWidth: 48
                            Layout.alignment: Qt.AlignHCenter
                            value: Number(modelData.value)
                            from: 0
                            to: 84
                            defaultValue: 35
                            step: 1
                            accentColor: Theme.accent
                            onValueEdited: function(v) {
                                if (modelData.field === "music") root.engine.masterMusic = v
                                else if (modelData.field === "mic") root.engine.masterMic = v
                                else root.engine.masterFx = v
                            }
                        }
                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: 48
                            Layout.preferredHeight: 23
                            radius: 8
                            color: "#080C10"
                            border.width: 1
                            border.color: "#050708"
                            Text { anchors.centerIn:parent;text:Math.round(Number(modelData.value));color:Theme.amber;font.family:Theme.monoFamily;font.pixelSize:9;font.weight:Font.Bold }
                        }
                        Item { Layout.fillHeight: true }
                    }
                }
            }
        }
    }
}
