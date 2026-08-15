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
    readonly property bool stackedControls: root.title === "Band Limits"
    readonly property bool effectTone: root.title === "Tone"
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
            Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:1;color:Theme.borderSoft;opacity:.72 }
        }

        // Mic Band Limits follows the web crossover-control-stack: compact
        // numeric field, full-width type selector, then LPF + selector.
        ColumnLayout {
            visible: root.stackedControls
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 9
            Layout.rightMargin: 9
            Layout.topMargin: 9
            Layout.bottomMargin: 9
            spacing: 5

            ValueField {
                Layout.preferredWidth: Math.min(78, parent.width * .52)
                Layout.minimumWidth: 66
                Layout.preferredHeight: 45
                title: root.fields.length > 0 ? String(root.fields[0].label || "") + (String(root.fields[0].unit || "").length ? "  ·  " + String(root.fields[0].unit).toUpperCase() : "") : "HPF"
                value: root.fields.length > 0 ? Number(root.fields[0].value) : 20
                from: root.fields.length > 0 ? Number(root.fields[0].from) : 20
                to: root.fields.length > 0 ? Number(root.fields[0].to) : 20000
                step: root.fields.length > 0 ? Number(root.fields[0].step || 1) : 1
                decimals: root.fields.length > 0 ? Number(root.fields[0].decimals || 0) : 0
                unit: ""
                defaultValue: value
                accentColor: Theme.amber
                onValueEdited: function(v) { if (root.fields.length > 0) root.fieldEdited(0, v) }
            }

            ColumnLayout {
                visible: root.showTypes
                Layout.fillWidth: true
                spacing: 2
                Text { text:"HP TYPE";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:9;font.letterSpacing:.8 }
                StudioComboBox {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 29
                    value: root.hpType
                    model: ["HP Butter 12","HP Butter 18","HP Butter 24","HP LR 24","HP Bessel 12","HP Bessel 18"]
                    accentColor: Theme.amber
                    onValueEdited: function(v){ root.hpTypeEdited(v) }
                }
            }

            ValueField {
                Layout.preferredWidth: Math.min(78, parent.width * .52)
                Layout.minimumWidth: 66
                Layout.preferredHeight: 45
                title: root.fields.length > 1 ? String(root.fields[1].label || "") + (String(root.fields[1].unit || "").length ? "  ·  " + String(root.fields[1].unit).toUpperCase() : "") : "LPF"
                value: root.fields.length > 1 ? Number(root.fields[1].value) : 20000
                from: root.fields.length > 1 ? Number(root.fields[1].from) : 20
                to: root.fields.length > 1 ? Number(root.fields[1].to) : 20000
                step: root.fields.length > 1 ? Number(root.fields[1].step || 1) : 1
                decimals: root.fields.length > 1 ? Number(root.fields[1].decimals || 0) : 0
                unit: ""
                defaultValue: value
                accentColor: Theme.amber
                onValueEdited: function(v) { if (root.fields.length > 1) root.fieldEdited(1, v) }
            }

            ColumnLayout {
                visible: root.showTypes
                Layout.fillWidth: true
                spacing: 2
                Text { text:"LP TYPE";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:9;font.letterSpacing:.8 }
                StudioComboBox {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 29
                    value: root.lpType
                    model: ["LP Butter 12","LP Butter 18","LP Butter 24","LP LR 24","LP Bessel 12","LP Bessel 18"]
                    accentColor: Theme.amber
                    onValueEdited: function(v){ root.lpTypeEdited(v) }
                }
            }
            Item { Layout.fillHeight: true }
        }

        // Effect/output rails use the web two-column cascade. The numeric
        // side stays narrow so the type labels receive the larger column.
        RowLayout {
            visible: !root.stackedControls
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 9
            Layout.rightMargin: 9
            Layout.topMargin: 9
            Layout.bottomMargin: 9
            spacing: 8

            ColumnLayout {
                Layout.preferredWidth: root.effectTone ? 66 : 72
                Layout.minimumWidth: root.effectTone ? 66 : 72
                Layout.maximumWidth: root.effectTone ? 66 : 82
                Layout.alignment: Qt.AlignTop
                spacing: root.effectTone ? 9 : 5
                Repeater {
                    model: root.fields
                    delegate: ValueField {
                        required property int index
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.effectTone ? 48 : 45
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
                Layout.minimumWidth: root.effectTone ? 106 : 106
                Layout.alignment: Qt.AlignTop
                spacing: root.effectTone ? 9 : 5

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text { text:"HP TYPE";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:9;font.letterSpacing:.8 }
                    StudioComboBox {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 29
                        value: root.hpType
                        model: ["HP Butter 12","HP Butter 18","HP Butter 24","HP LR 24","HP Bessel 12","HP Bessel 18"]
                        accentColor: Theme.amber
                        onValueEdited: function(v){ root.hpTypeEdited(v) }
                    }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text { text:"LP TYPE";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:9;font.letterSpacing:.8 }
                    StudioComboBox {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 29
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
