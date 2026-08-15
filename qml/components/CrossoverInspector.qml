import QtQuick
import QtQuick.Layouts

Item {
    id: root

    property string mode: "hpf" // hpf | lpf
    property real frequency: 20
    property string filterType: mode === "hpf" ? "HP Butter 12" : "LP Butter 12"
    property color accentColor: Theme.amber

    signal frequencyEdited(real value)
    signal typeEdited(string value)
    signal resetRequested()

    implicitWidth: 320
    implicitHeight: 86

    readonly property bool highPass: root.mode === "hpf"
    readonly property var typeOptions: highPass
        ? ["HP Butter 12","HP Butter 18","HP Butter 24","HP LR 24","HP Bessel 12","HP Bessel 18"]
        : ["LP Butter 12","LP Butter 18","LP Butter 24","LP LR 24","LP Bessel 12","LP Bessel 18"]

    ColumnLayout {
        anchors.fill: parent
        spacing: 5

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 31
            spacing: 8

            ColumnLayout {
                spacing: -1
                Text {
                    text: root.highPass ? "HPF" : "LPF"
                    color: Theme.textDim
                    font.family: Theme.monoFamily
                    font.pixelSize: 8
                    font.weight: Font.DemiBold
                    font.letterSpacing: 1.2
                }
                Text {
                    text: root.highPass ? "High Pass Filter" : "Low Pass Filter"
                    color: Theme.text
                    font.family: Theme.fontFamily
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                }
            }

            Item { Layout.fillWidth: true }

            SoftButton {
                Layout.preferredWidth: 46
                Layout.preferredHeight: 23
                text: "Reset"
                compact: true
                onClicked: root.resetRequested()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            spacing: 8

            ValueField {
                Layout.preferredWidth: 98
                title: "FREQ"
                value: root.frequency
                from: 20
                to: 20000
                step: 5
                defaultValue: root.frequency
                decimals: 0
                unit: "Hz"
                accentColor: root.accentColor
                onValueEdited: function(v) { root.frequencyEdited(v) }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Text {
                    text: "TYPE"
                    color: Theme.textDim
                    font.family: Theme.monoFamily
                    font.pixelSize: 8
                    font.letterSpacing: 1.0
                }
                StudioComboBox {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 28
                    model: root.typeOptions
                    value: root.filterType
                    accentColor: root.accentColor
                    onValueEdited: function(v) { root.typeEdited(v) }
                }
            }
        }
    }
}
