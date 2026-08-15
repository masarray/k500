import QtQuick
import QtQuick.Layouts

StudioPanel {
    id: root
    required property var engine
    implicitHeight: 304
    accentTop: false

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

            ParameterSlider { Layout.fillWidth:true;label:"LPF";value:root.engine.lpfHz;from:20;to:20000;step:100;defaultValue:20000;decimals:0;unit:"Hz";logarithmic:true;accentColor:Theme.accent;onValueEdited:function(v){root.engine.lpfHz=v} }
            Text { text:"LP TYPE";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.letterSpacing:1.0 }
            StudioComboBox {
                Layout.fillWidth:true
                Layout.preferredHeight:30
                model:["LP Bessel 12","LP Butter 12","LP Bessel 18","LP Butter 18","LP Bessel 24","LP Butter 24","LP LR 24"]
                value:root.engine.lpType
                onValueEdited:function(v){root.engine.lpType=v}
            }
            Item { Layout.preferredHeight: 2 }
            ParameterSlider { Layout.fillWidth:true;label:"HPF";value:root.engine.hpfHz;from:20;to:20000;step:5;defaultValue:20;decimals:0;unit:"Hz";logarithmic:true;accentColor:Theme.accent;onValueEdited:function(v){root.engine.hpfHz=v} }
            Text { text:"HP TYPE";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.letterSpacing:1.0 }
            StudioComboBox {
                Layout.fillWidth:true
                Layout.preferredHeight:30
                model:["HP Bessel 12","HP Butter 12","HP Bessel 18","HP Butter 18","HP Bessel 24","HP Butter 24","HP LR 24"]
                value:root.engine.hpType
                onValueEdited:function(v){root.engine.hpType=v}
            }
            Item { Layout.fillHeight:true }
        }
    }
}
