import QtQuick
import QtQuick.Layouts

Item {
    id: root
    property var bandModel: null
    property int bandIndex: 0
    property real frequency: 80
    property real gain: 0
    property real q: 1
    property color accentColor: Theme.accent
    signal frequencyEdited(real value)
    signal gainEdited(real value)
    signal qEdited(real value)
    signal resetRequested()

    implicitWidth: 320
    implicitHeight: 86

    function shortType() {
        if (!bandModel) return "P"
        var t = String(bandModel.get(bandIndex).typeName || "BELL")
        return t === "LOW SHELF" ? "LS" : t === "HIGH SHELF" ? "HS" : "P"
    }
    function longType(v) { return v === "LS" ? "LOW SHELF" : v === "HS" ? "HIGH SHELF" : "BELL" }

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
                    text: "BAND " + (root.bandIndex + 1)
                    color: Theme.textDim
                    font.family: Theme.monoFamily
                    font.pixelSize: 8
                    font.weight: Font.DemiBold
                    font.letterSpacing: 1.2
                }
                Text {
                    text: {
                        if (!root.bandModel) return "Bell"
                        var t = String(root.bandModel.get(root.bandIndex).typeName || "BELL")
                        return t === "LOW SHELF" ? "Low Shelf" : t === "HIGH SHELF" ? "High Shelf" : "Bell"
                    }
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
            spacing: 7

            ColumnLayout {
                Layout.preferredWidth: 66
                spacing: 2
                Text { text: "TYPE"; color: Theme.textDim; font.family: Theme.monoFamily; font.pixelSize: 8; font.letterSpacing: 1.0 }
                StudioComboBox {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 28
                    enabled: root.bandModel !== null
                    model: ["P", "LS", "HS"]
                    value: root.shortType()
                    accentColor: Theme.amber
                    onValueEdited: function(v) { if (root.bandModel) root.bandModel.setBandType(root.bandIndex, root.longType(v)) }
                }
            }

            ValueField {
                Layout.preferredWidth: 92
                title: "FREQ"
                value: root.frequency
                from: 20; to: 20000; step: 5; defaultValue: root.frequency
                decimals: 0; unit: "Hz"; accentColor: Theme.amber
                onValueEdited: function(v) { root.frequencyEdited(v) }
            }
            ValueField {
                Layout.preferredWidth: 54
                title: "Q"
                value: root.q
                from: 0.1; to: 30; step: 0.1; defaultValue: 1
                decimals: 1; unit: ""; accentColor: Theme.amber
                onValueEdited: function(v) { root.qEdited(v) }
            }
            ValueField {
                Layout.preferredWidth: 66
                title: "GAIN"
                value: root.gain
                from: -24; to: 24; step: 0.1; defaultValue: 0
                decimals: 1; unit: ""; accentColor: Theme.amber
                onValueEdited: function(v) { root.gainEdited(v) }
            }
        }
    }
}
