import QtQuick
import QtQuick.Layouts

StudioPanel {
    id: root

    property string eyebrow: "FILTERS"
    property string title: "Band Limits"
    property var fields: []
    property string hpType: "HP LR 24"
    property string lpType: "LP LR 24"
    property bool showTypes: true
    property color accentColor: Theme.amber

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 11
        spacing: 7

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0
            Text { text:root.eyebrow.toUpperCase(); color:Theme.textDim; font.family:Theme.fontFamily; font.pixelSize:8; font.weight:Font.DemiBold; font.letterSpacing:1.0 }
            Text { text:root.title.toUpperCase(); color:Theme.text; font.family:Theme.fontFamily; font.pixelSize:10; font.weight:Font.Bold; font.letterSpacing:.3 }
        }
        Rectangle { Layout.fillWidth:true; Layout.preferredHeight:1; color:Theme.borderSoft; opacity:.78 }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 7
                Repeater {
                    model: root.fields
                    delegate: ValueField {
                        required property var modelData
                        Layout.fillWidth: true
                        title: String(modelData.label || "")
                        value: Number(modelData.value)
                        from: Number(modelData.from)
                        to: Number(modelData.to)
                        step: Number(modelData.step || 1)
                        decimals: Number(modelData.decimals || 0)
                        unit: String(modelData.unit || "Hz")
                        defaultValue: Number(modelData.value)
                        accentColor: Theme.amber
                    }
                }
                Item { Layout.fillHeight: true }
            }

            ColumnLayout {
                visible: root.showTypes
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignTop
                spacing: 8
                Text { text:"HP TYPE"; color:Theme.textDim; font.family:Theme.fontFamily; font.pixelSize:8; font.weight:Font.DemiBold; font.letterSpacing:.7 }
                StudioComboBox {
                    Layout.fillWidth: true
                    value: root.hpType
                    model: ["HP Butter 12","HP Butter 18","HP LR 24","HP Bessel 12","HP Bessel 18"]
                    accentColor: Theme.amber
                    onValueEdited: function(v){ root.hpType=v }
                }
                Text { text:"LP TYPE"; color:Theme.textDim; font.family:Theme.fontFamily; font.pixelSize:8; font.weight:Font.DemiBold; font.letterSpacing:.7 }
                StudioComboBox {
                    Layout.fillWidth: true
                    value: root.lpType
                    model: ["LP Butter 12","LP Butter 18","LP LR 24","LP Bessel 12","LP Bessel 18"]
                    accentColor: Theme.amber
                    onValueEdited: function(v){ root.lpType=v }
                }
                Item { Layout.fillHeight: true }
            }
        }
    }
}
