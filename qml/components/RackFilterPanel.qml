import QtQuick
import QtQuick.Layouts

StudioPanel {
    id: root

    property string eyebrow: ""
    property string title: "Band Limits"
    property var fields: []
    property string hpType: "HP LR 24"
    property string lpType: "LP LR 24"
    property bool showTypes: true
    property color accentColor: Theme.amber
    signal fieldEdited(int index, real value)
    signal hpTypeEdited(string value)
    signal lpTypeEdited(string value)
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
            Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:1;color:Theme.borderSoft;opacity:.78 }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 10
            Layout.rightMargin: 10
            Layout.topMargin: 10
            Layout.bottomMargin: 10
            spacing: 9

            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                spacing: 7
                Repeater {
                    model: root.fields
                    delegate: ValueField {
                        required property int index
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: 50
                        title: String(modelData.label || "") + (String(modelData.unit || "").length ? "  ·  " + String(modelData.unit || "").toUpperCase() : "")
                        value: Number(modelData.value)
                        from: Number(modelData.from)
                        to: Number(modelData.to)
                        step: Number(modelData.step || 1)
                        decimals: Number(modelData.decimals || 0)
                        unit: ""
                        defaultValue: Number(modelData.value)
                        accentColor: Theme.amber
                        onValueEdited: function(v) { root.fieldEdited(index, v) }
                    }
                }
                Item { Layout.fillHeight: true }
            }

            ColumnLayout {
                visible: root.showTypes
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                spacing: 7

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text { text:"HP TYPE";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.letterSpacing:1.0 }
                    StudioComboBox {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 30
                        value: root.hpType
                        model: ["HP Butter 12","HP Butter 18","HP Butter 24","HP LR 24","HP Bessel 12","HP Bessel 18"]
                        accentColor: Theme.amber
                        onValueEdited: function(v){ root.hpTypeEdited(v) }
                    }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text { text:"LP TYPE";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.letterSpacing:1.0 }
                    StudioComboBox {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 30
                        value: root.lpType
                        model: ["LP Butter 12","LP Butter 18","LP Butter 24","LP LR 24","LP Bessel 12","LP Bessel 18"]
                        accentColor: Theme.amber
                        onValueEdited: function(v){ root.lpTypeEdited(v) }
                    }
                }
                Item { Layout.fillHeight: true }
            }
        }
    }
}
