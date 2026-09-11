import QtQuick
import QtQuick.Layouts

StudioPanel {
    id: root
    required property var engine
    implicitHeight: 304
    accentTop: false

    // CROSSOVER_EDGE_BYPASS_V1
    // Native K500 exposes "bypass" in the type dropdown, but no donor packet for
    // the bypass type byte is available yet. Do not guess an unverified protocol
    // byte: use the already verified transparent edge positions (HPF=20 Hz,
    // LPF=20 kHz) as the safe audible bypass and remember the previous cutoff.
    property real rememberedHpfHz: engine.hpfHz > 20.001 ? engine.hpfHz : 80
    property real rememberedLpfHz: engine.lpfHz < 19999.999 ? engine.lpfHz : 16000

    function hpDisplayType() { return root.engine.hpfHz <= 20.001 ? "Bypass" : root.engine.hpType }
    function lpDisplayType() { return root.engine.lpfHz >= 19999.999 ? "Bypass" : root.engine.lpType }
    function setHpType(value) {
        if (value === "Bypass") {
            if (root.engine.hpfHz > 20.001) root.rememberedHpfHz = root.engine.hpfHz
            root.engine.hpfHz = 20
            return
        }
        root.engine.hpType = value
        if (root.engine.hpfHz <= 20.001)
            root.engine.hpfHz = Math.max(21, root.rememberedHpfHz)
    }
    function setLpType(value) {
        if (value === "Bypass") {
            if (root.engine.lpfHz < 19999.999) root.rememberedLpfHz = root.engine.lpfHz
            root.engine.lpfHz = 20000
            return
        }
        root.engine.lpType = value
        if (root.engine.lpfHz >= 19999.999)
            root.engine.lpfHz = Math.min(19999, root.rememberedLpfHz)
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
                text: "HPF / LPF"
                color: Theme.text
                font.family: Theme.monoFamily
                font.pixelSize: 10
                font.weight: Font.Bold
                font.letterSpacing: 1.05
            }
            Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:1;color:Theme.borderSoft;opacity:.78 }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.topMargin: 10
            Layout.bottomMargin: 10
            spacing: 7

            ParameterSlider {
                Layout.fillWidth:true;label:"LPF";value:root.engine.lpfHz;from:20;to:20000;step:100;defaultValue:20000;decimals:0;unit:"Hz";logarithmic:true;accentColor:Theme.accent
                onValueEdited:function(v){root.engine.lpfHz=v;if(v<19999.999)root.rememberedLpfHz=v}
            }
            Text { text:"LP TYPE";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.letterSpacing:1.0 }
            StudioComboBox {
                Layout.fillWidth:true
                Layout.preferredHeight:30
                model:["Bypass","LP Bessel 12","LP Butter 12","LP Bessel 18","LP Butter 18","LP Bessel 24","LP Butter 24","LP LR 24"]
                value:root.lpDisplayType()
                onValueEdited:function(v){root.setLpType(v)}
            }
            Item { Layout.preferredHeight: 2 }
            ParameterSlider {
                Layout.fillWidth:true;label:"HPF";value:root.engine.hpfHz;from:20;to:20000;step:5;defaultValue:20;decimals:0;unit:"Hz";logarithmic:true;accentColor:Theme.accent
                onValueEdited:function(v){root.engine.hpfHz=v;if(v>20.001)root.rememberedHpfHz=v}
            }
            Text { text:"HP TYPE";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.letterSpacing:1.0 }
            StudioComboBox {
                Layout.fillWidth:true
                Layout.preferredHeight:30
                model:["Bypass","HP Bessel 12","HP Butter 12","HP Bessel 18","HP Butter 18","HP Bessel 24","HP Butter 24","HP LR 24"]
                value:root.hpDisplayType()
                onValueEdited:function(v){root.setHpType(v)}
            }
            Item { Layout.fillHeight:true }
        }
    }
}