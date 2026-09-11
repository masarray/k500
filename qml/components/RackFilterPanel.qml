import QtQuick
import QtQuick.Layouts

StudioPanel {
    id: root

    property string eyebrow: ""
    property string title: "Band Limits"
    property var fields: []
    property string hpType: "HP LR 24"
    property string lpType: "LP LR 24"
    property bool showTypes: true
    property color accentColor: Theme.amber

    signal fieldEdited(int index, real value)
    signal hpTypeEdited(string value)
    signal lpTypeEdited(string value)
    accentTop: false

    // CROSSOVER_RACK_LAYOUT_V2
    // A single vertical grammar is used for Mic, effects and output sections:
    // optional delay controls first, then HPF and LPF, each with a dedicated
    // responsive slider and type selector. No text field may sit on top of a track.
    readonly property int leftDelayIndex: fieldIndex("L DELAY")
    readonly property int rightDelayIndex: fieldIndex("R DELAY")
    readonly property int hpfIndex: fieldIndex("HPF")
    readonly property int lpfIndex: fieldIndex("LPF")
    readonly property bool hasDelay: leftDelayIndex >= 0 || rightDelayIndex >= 0

    function fieldIndex(label) {
        var wanted = String(label).toUpperCase()
        for (var i = 0; i < root.fields.length; ++i)
            if (String(root.fields[i].label || "").toUpperCase() === wanted) return i
        return -1
    }
    function fieldAt(index) {
        return index >= 0 && index < root.fields.length ? root.fields[index] : ({})
    }
    function numberAt(index, key, fallback) {
        var f = fieldAt(index)
        var n = Number(f[key])
        return isFinite(n) ? n : fallback
    }

    // P1_RACK_FILTER_LIVE_BRIDGE_V1
    // Reusable rack controls resolve StudioEngine through the owning workspace.
    // QML never bypasses the canonical edit bridge or touches raw transport.
    function studioContext() {
        var p = root
        while (p) {
            if (p.engine && typeof p.engine.editDevicePath === "function")
                return { engine:p.engine, sectionIndex:Number(p.sectionIndex) }
            p = p.parent
        }
        return null
    }
    function dispatchVerifiedAux(index, value) {
        if (index < 0 || index >= root.fields.length) return
        var label = String(root.fields[index].label || "").toUpperCase()
        if (label !== "L DELAY" && label !== "R DELAY") return
        var ctx = studioContext()
        // SURROUND_DELAY_VERIFIED_V1 — donor captures prove D16/D17 and D18/D19
        // in Surround CMD 0x0E. No Main/Center delay byte is guessed here.
        if (!ctx || ctx.sectionIndex !== 5) return
        ctx.engine.editDevicePath(label === "L DELAY" ? "outputs.surround.lDelayMs" : "outputs.surround.rDelayMs", value)
    }
    function editField(index, value) {
        if (index < 0) return
        root.fieldEdited(index, value)
        root.dispatchVerifiedAux(index, value)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 35
            Text {
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                text: root.title.toUpperCase()
                color: Theme.text
                font.family: Theme.monoFamily
                font.pixelSize: 10
                font.weight: Font.Bold
                font.letterSpacing: 1.05
                elide: Text.ElideRight
                width: parent.width - 24
            }
            Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:1;color:Theme.borderSoft;opacity:.72 }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 10
            Layout.rightMargin: 10
            Layout.topMargin: 8
            Layout.bottomMargin: 8
            spacing: 4

            Text {
                visible: root.hasDelay
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? 11 : 0
                text: "OUTPUT DELAY"
                color: Theme.textDim
                font.family: Theme.monoFamily
                font.pixelSize: 8
                font.weight: Font.DemiBold
                font.letterSpacing: .9
            }

            ParameterSlider {
                visible: root.leftDelayIndex >= 0
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? 30 : 0
                label: "L"
                value: root.numberAt(root.leftDelayIndex,"value",0)
                from: root.numberAt(root.leftDelayIndex,"from",0)
                to: root.numberAt(root.leftDelayIndex,"to",50)
                step: root.numberAt(root.leftDelayIndex,"step",1)
                defaultValue: value
                decimals: root.numberAt(root.leftDelayIndex,"decimals",0)
                unit: String(root.fieldAt(root.leftDelayIndex).unit || "ms")
                accentColor: Theme.accent
                captionWidth: 15; readoutWidth: 50; controlGap: 5; trackGap: 4
                onValueEdited: function(v){ root.editField(root.leftDelayIndex,v) }
            }

            ParameterSlider {
                visible: root.rightDelayIndex >= 0
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? 30 : 0
                label: "R"
                value: root.numberAt(root.rightDelayIndex,"value",0)
                from: root.numberAt(root.rightDelayIndex,"from",0)
                to: root.numberAt(root.rightDelayIndex,"to",50)
                step: root.numberAt(root.rightDelayIndex,"step",1)
                defaultValue: value
                decimals: root.numberAt(root.rightDelayIndex,"decimals",0)
                unit: String(root.fieldAt(root.rightDelayIndex).unit || "ms")
                accentColor: Theme.accent
                captionWidth: 15; readoutWidth: 50; controlGap: 5; trackGap: 4
                onValueEdited: function(v){ root.editField(root.rightDelayIndex,v) }
            }

            Rectangle {
                visible: root.hasDelay
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? 1 : 0
                Layout.topMargin: visible ? 2 : 0
                Layout.bottomMargin: visible ? 2 : 0
                color: Theme.borderSoft
                opacity: .48
            }

            ParameterSlider {
                visible: root.hpfIndex >= 0
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? 31 : 0
                label: "HPF"
                value: root.numberAt(root.hpfIndex,"value",20)
                from: root.numberAt(root.hpfIndex,"from",20)
                to: root.numberAt(root.hpfIndex,"to",20000)
                step: root.numberAt(root.hpfIndex,"step",1)
                defaultValue: value
                decimals: root.numberAt(root.hpfIndex,"decimals",0)
                unit: String(root.fieldAt(root.hpfIndex).unit || "Hz")
                logarithmic: true
                accentColor: Theme.amber
                captionWidth: 28; readoutWidth: 56; controlGap: 5; trackGap: 4
                onValueEdited: function(v){ root.editField(root.hpfIndex,v) }
            }

            StudioComboBox {
                visible: root.showTypes && root.hpfIndex >= 0
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? 28 : 0
                value: String(root.hpType)
                model: ["Bypass","HP Bessel 12","HP Butter 12","HP Bessel 18","HP Butter 18","HP Bessel 24","HP Butter 24","HP LR 24"]
                accentColor: Theme.amber
                onValueEdited: function(v){ root.hpTypeEdited(v) }
            }

            ParameterSlider {
                visible: root.lpfIndex >= 0
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? 31 : 0
                label: "LPF"
                value: root.numberAt(root.lpfIndex,"value",20000)
                from: root.numberAt(root.lpfIndex,"from",20)
                to: root.numberAt(root.lpfIndex,"to",20000)
                step: root.numberAt(root.lpfIndex,"step",1)
                defaultValue: value
                decimals: root.numberAt(root.lpfIndex,"decimals",0)
                unit: String(root.fieldAt(root.lpfIndex).unit || "Hz")
                logarithmic: true
                accentColor: Theme.amber
                captionWidth: 28; readoutWidth: 56; controlGap: 5; trackGap: 4
                onValueEdited: function(v){ root.editField(root.lpfIndex,v) }
            }

            StudioComboBox {
                visible: root.showTypes && root.lpfIndex >= 0
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? 28 : 0
                value: String(root.lpType)
                model: ["Bypass","LP Bessel 12","LP Butter 12","LP Bessel 18","LP Butter 18","LP Bessel 24","LP Butter 24","LP LR 24"]
                accentColor: Theme.amber
                onValueEdited: function(v){ root.lpTypeEdited(v) }
            }

            Item { Layout.fillHeight: true }
        }
    }
}