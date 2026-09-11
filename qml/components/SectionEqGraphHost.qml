import QtQuick
import QtQuick.Layouts

// CRASH_SAFE_FIXED_EQ_PAGES_V1
// Each processor owns a persistent SectionEqGraph with a model that never
// changes for the lifetime of that graph. Switching sections changes only the
// StackLayout page, eliminating 10-band/7-band/5-band model hot-swaps inside
// Canvas/Repeater/Inspector objects and avoiding dynamic destroy/recreate.
StackLayout {
    id: root

    required property var engine
    property int sectionIndex: 1
    property int micChannel: 0
    property bool eqLinked: false

    signal micChannelRequested(int channel)
    signal eqLinkRequested(bool linked)

    currentIndex: {
        if (root.sectionIndex === 1)
            return root.micChannel === 0 ? 0 : 1
        return Math.max(2, Math.min(7, root.sectionIndex))
    }

    SectionEqGraph {
        engine: root.engine
        bandModel: root.engine.micAEqBands
        sectionLabel: "Mic A"
        showMicSelector: true
        micChannel: root.micChannel
        eqLinked: root.eqLinked
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumHeight: 0
        onMicChannelRequested: function(channel) { root.micChannelRequested(channel) }
        onEqLinkRequested: function(linked) { root.eqLinkRequested(linked) }
    }

    SectionEqGraph {
        engine: root.engine
        bandModel: root.engine.micBEqBands
        sectionLabel: "Mic B"
        showMicSelector: true
        micChannel: root.micChannel
        eqLinked: root.eqLinked
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumHeight: 0
        onMicChannelRequested: function(channel) { root.micChannelRequested(channel) }
        onEqLinkRequested: function(linked) { root.eqLinkRequested(linked) }
    }

    SectionEqGraph {
        engine: root.engine
        bandModel: root.engine.reverbEqBands
        sectionLabel: "Reverb"
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumHeight: 0
    }

    SectionEqGraph {
        engine: root.engine
        bandModel: root.engine.echoEqBands
        sectionLabel: "Echo"
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumHeight: 0
    }

    SectionEqGraph {
        engine: root.engine
        bandModel: root.engine.mainEqBands
        sectionLabel: "Main"
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumHeight: 0
    }

    SectionEqGraph {
        engine: root.engine
        bandModel: root.engine.surroundEqBands
        sectionLabel: "Surround"
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumHeight: 0
    }

    SectionEqGraph {
        engine: root.engine
        bandModel: root.engine.centerEqBands
        sectionLabel: "Center"
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumHeight: 0
    }

    SectionEqGraph {
        engine: root.engine
        bandModel: root.engine.subEqBands
        sectionLabel: "Subwoofer"
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumHeight: 0
    }
}