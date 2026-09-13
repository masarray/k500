import QtQuick

Item {
    id: root

    required property var bandModel
    property var engine: null
    property real hpfFreq: 20
    property real lpfFreq: 20000

    implicitHeight: 500

    // MUSIC_CROSSOVER_STARTUP_DEVICE_SEMANTICS_V2
    // The shared graph still contains a legacy offline BYPASS priming step.
    // Restore Music's known native Butterworth-12 type after child completion;
    // startup occurs with LIVE disabled, so this cannot replay to hardware.
    Component.onCompleted: Qt.callLater(function() {
        if (!root.engine) return
        root.engine.hpType = "HP Butter 12"
        root.engine.lpType = "LP Butter 12"
    })

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
