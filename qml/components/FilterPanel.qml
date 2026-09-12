import QtQuick
import QtQuick.Layouts

StudioPanel {
    id: root
    required property var engine
    implicitHeight: 304
    accentTop: false

    // CROSSOVER_TYPE_BYPASS_V2
    // Bypass is the filter TYPE. Frequency remains where the user placed the
    // anchor, exactly like the native K500 app. The renderer/protocol decides
    // whether that anchor contributes roll-off from the selected type.
    function setHpType(value) { root.engine.hpType = value }
    function setLpType(value) { root.engine.lpType = value }

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
            Layout.leftMargin: 10
            Layout.rightMargin: 10
            Layout.topMargin: 9
            Layout.bottomMargin: 9
            spacing: 5

            ParameterSlider {
                Layout.fillWidth:true
                Layout.preferredHeight:32
                label:"LPF"
                value:root.engine.lpfHz
                from:20;to:20000;step:10;defaultValue:20000;decimals:0;unit:"Hz";logarithmic:true
                accentColor:Theme.accent
                captionWidth:25
                readoutWidth:56
                controlGap:5
                trackGap:5
                onValueEdited:function(v){root.engine.lpfHz=v}
            }
            StudioComboBox {
                Layout.fillWidth:true
                Layout.preferredHeight:30
                model:["Bypass","LP Bessel 12","LP Butter 12","LP Bessel 18","LP Butter 18","LP Bessel 24","LP Butter 24","LP LR 24"]
                value:String(root.engine.lpType)
                onValueEdited:function(v){root.setLpType(v)}
            }

            Rectangle { Layout.fillWidth:true;Layout.preferredHeight:1;color:Theme.borderSoft;opacity:.45;Layout.topMargin:2;Layout.bottomMargin:2 }

            ParameterSlider {
                Layout.fillWidth:true
                Layout.preferredHeight:32
                label:"HPF"
                value:root.engine.hpfHz
                from:20;to:20000;step:5;defaultValue:20;decimals:0;unit:"Hz";logarithmic:true
                accentColor:Theme.accent
                captionWidth:25
                readoutWidth:56
                controlGap:5
                trackGap:5
                onValueEdited:function(v){root.engine.hpfHz=v}
            }
            StudioComboBox {
                Layout.fillWidth:true
                Layout.preferredHeight:30
                model:["Bypass","HP Bessel 12","HP Butter 12","HP Bessel 18","HP Butter 18","HP Bessel 24","HP Butter 24","HP LR 24"]
                value:String(root.engine.hpType)
                onValueEdited:function(v){root.setHpType(v)}
            }
            Item { Layout.fillHeight:true }
        }
    }
}