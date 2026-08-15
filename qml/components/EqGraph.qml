import QtQuick

Item {
    id: root

    required property var bandModel
    property var engine: null
    property real hpfFreq: 20
    property real lpfFreq: 20000

    implicitHeight: 500

    SectionEqGraph {
        anchors.fill: parent
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
