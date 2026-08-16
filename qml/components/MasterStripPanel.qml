import QtQuick
import QtQuick.Layouts

StudioPanel {
    id: root
    required property var engine
    property int selectedFader: -1
    implicitWidth: 188
    implicitHeight: 304
    Layout.minimumWidth: 188
    Layout.preferredWidth: 188
    Layout.maximumWidth: 188
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
            Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:1;color:Theme.borderSoft;opacity:.72 }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 7
            Layout.rightMargin: 7
            Layout.topMargin: 10
            Layout.bottomMargin: 10
            spacing: 1

            Repeater {
                model: [
                    {label:"MUSIC", field:"music"},
                    {label:"MIC", field:"mic"},
                    {label:"FX", field:"fx"}
                ]
                delegate: Item {
                    id: channel
                    required property int index
                    required property var modelData
                    readonly property real liveValue: modelData.field === "music" ? Number(root.engine.masterMusic)
                                                    : modelData.field === "mic" ? Number(root.engine.masterMic)
                                                    : Number(root.engine.masterFx)
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 4

                        Item {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredHeight: 25
                            Layout.preferredWidth: 48
                            Text {
                                anchors.centerIn: parent
                                text: modelData.label
                                color: masterFader.highlighted ? masterFader.accentColor : Theme.textDim
                                style: masterFader.highlighted ? Text.Outline : Text.Normal
                                styleColor: masterFader.highlighted ? Qt.rgba(masterFader.accentColor.r,masterFader.accentColor.g,masterFader.accentColor.b,.34) : "transparent"
                                font.family: Theme.monoFamily
                                font.pixelSize: 9
                                font.weight: masterFader.highlighted ? Font.Bold : Font.DemiBold
                                font.letterSpacing: .35
                                Behavior on color { ColorAnimation { duration:75 } }
                                Behavior on styleColor { ColorAnimation { duration:75 } }
                            }
                        }

                        Item { Layout.preferredHeight: 4 }

                        StudioFader {
                            id: masterFader
                            Layout.preferredHeight: 160
                            Layout.minimumHeight: 160
                            Layout.maximumHeight: 160
                            Layout.preferredWidth: 48
                            Layout.alignment: Qt.AlignHCenter
                            value: channel.liveValue
                            from: 0
                            to: 84
                            defaultValue: 35
                            step: 1
                            accentColor: Theme.accent
                            selected: root.selectedFader === channel.index
                            onActivated: root.selectedFader = channel.index
                            onValueEdited: function(v) {
                                if (modelData.field === "music") root.engine.masterMusic = v
                                else if (modelData.field === "mic") root.engine.masterMic = v
                                else root.engine.masterFx = v
                            }
                        }

                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: 54
                            Layout.preferredHeight: 23
                            radius: 8
                            color: masterFader.highlighted ? "#081013" : "#05080A"
                            border.width: 1
                            border.color: masterFader.highlighted ? Theme.accentSoft : "#020304"
                            Text {
                                anchors.centerIn: parent
                                text: Math.round(channel.liveValue)
                                color: Theme.amber
                                font.family: Theme.monoFamily
                                font.pixelSize: 9
                                font.weight: Font.Bold
                            }
                            Behavior on border.color { ColorAnimation { duration: 75 } }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }
            }
        }
    }
}
