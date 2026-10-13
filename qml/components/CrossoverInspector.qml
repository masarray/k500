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

    // INSPECTOR_OPTICAL_PADDING_V3 — structured header and controls have
    // 16px horizontal / 14px vertical inner space without growing the graph.
    implicitWidth: 340
    implicitHeight: 120

    readonly property bool highPass: root.mode === "hpf"
    readonly property bool bypassed: String(root.filterType).trim().toUpperCase() === "BYPASS"
    readonly property var typeOptions: highPass
        ? ["Bypass","HP Butter 12","HP Butter 18","HP Butter 24","HP LR 24","HP Bessel 12","HP Bessel 18","HP Bessel 24"]
        : ["Bypass","LP Butter 12","LP Butter 18","LP Butter 24","LP LR 24","LP Bessel 12","LP Bessel 18","LP Bessel 24"]

    // FINAL_INSPECTOR_SCOPE_COPY_V1
    // This action returns only the selected crossover edge frequency to its
    // boundary. It does not change filter TYPE, so label it RESET EDGE rather
    // than a generic Reset/Reset Filter action.

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
        color: root.bypassed ? Theme.textDim : root.accentColor
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
                    text: root.highPass ? "HPF" : "LPF"
                    color: Theme.textDim
                    font.family: Theme.monoFamily
                    font.pixelSize: 9
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0.75
                }
                Text {
                    text: root.highPass ? "High Pass Filter" : "Low Pass Filter"
                    color: root.bypassed ? Theme.textSoft : Theme.text
                    font.family: Theme.fontFamily
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                }
            }

            Item { Layout.fillWidth: true }

            SoftButton {
                Layout.preferredWidth: 74
                Layout.preferredHeight: 23
                text: "RESET EDGE"
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
                    font.pixelSize: 9
                    font.letterSpacing: 0.65
                }
                StudioComboBox {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 28
                    model: root.typeOptions
                    value: root.bypassed ? "Bypass" : root.filterType
                    accentColor: root.accentColor
                    onValueEdited: function(v) { root.typeEdited(v) }
                }
            }
        }
    }
}
