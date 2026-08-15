import QtQuick

Item {
    id: root

    required property var bandModel
    property real hpfFreq: 20
    property real lpfFreq: 20000

    implicitHeight: 500

    SectionEqGraph {
        anchors.fill: parent
        bandModel: root.bandModel
        sectionLabel: "Music"
        showMicSelector: false
    }
}
