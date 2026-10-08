import QtQuick

Item {
    id: root

    required property var bandModel
    property var engine: null
    property real hpfFreq: 20
    property real lpfFreq: 20000

    implicitHeight: 500

    // OFFLINE_CROSSOVER_SINGLE_AUTHORITY_V1 — use the canonical engine
    // default/Preview/readback; do not force Butterworth after child creation.

    SectionEqGraph {
        anchors.fill: parent
        engine: root.engine
        bandModel: root.bandModel
        sectionLabel: "Music"
        showMicSelector: false
        onCrossoverTypeRequested: function(which, value) {
            if (!root.engine) return
            if (which === "hpf") root.engine.hpType = value
            else root.engine.lpType = value
        }
    }
}
