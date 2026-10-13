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

    // INSPECTOR_OPTICAL_PADDING_V3 — structured header and controls have
    // 16px horizontal / 14px vertical inner space without growing the graph.
    implicitWidth: 340
    implicitHeight: 120

    // FINAL_INSPECTOR_SCOPE_COPY_V1
    // The page toolbar already owns EQ RESET (all PEQ bands). Keep the floating
    // inspector action explicitly band-scoped so rapid mixer work never makes a
    // single-band reset look like another global reset affordance.

    function shortType() {
        if (!bandModel) return "P"
        var t = String(bandModel.get(bandIndex).typeName || "BELL")
        return t === "LOW SHELF" ? "LS" : t === "HIGH SHELF" ? "HS" : "P"
    }
    function longType(v) { return v === "LS" ? "LOW SHELF" : v === "HS" ? "HIGH SHELF" : "BELL" }

    // VISUAL_INSPECTOR_SURFACE_V1 — purely presentational: no pointer
    // interception, model replacement, shadow layers, or device writes.
    // Keep the floating inspector non-modal; only its visual content grows.
    Rectangle {
        anchors.fill: parent
        anchors.margins: 0
        radius: 9
        color: "#10171D"
        border.width: 1
        border.color: "#3A4850"
        z: -1
    }

    Rectangle {
        anchors.left: parent.left
        anchors.leftMargin: 6
        anchors.verticalCenter: parent.verticalCenter
        width: 3
        height: 27
        radius: 1.5
        color: root.accentColor
        opacity: 0.85
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.topMargin: 14
        anchors.bottomMargin: 14
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            spacing: 8

            ColumnLayout {
                spacing: 3
                Text {
                    text: "BAND " + (root.bandIndex + 1)
                    color: Theme.textDim
                    font.family: Theme.monoFamily
                    font.pixelSize: 9
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0.75
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
                Layout.preferredWidth: 74
                Layout.preferredHeight: 23
                text: "RESET BAND"
                compact: true
                onClicked: root.resetRequested()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            spacing: 7

            ColumnLayout {
                Layout.preferredWidth: 64
                spacing: 2
                Text { text: "TYPE"; color: Theme.textDim; font.family: Theme.monoFamily; font.pixelSize: 9; font.letterSpacing: 0.65 }
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
                Layout.preferredWidth: 90
                title: "FREQ"
                value: root.frequency
                from: 20; to: 20000; step: 5; defaultValue: root.frequency
                decimals: 0; unit: ""; accentColor: Theme.amber
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
