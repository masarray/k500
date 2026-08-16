import QtQuick
import QtQuick.Layouts

StudioPanel {
    id: root
    required property var engine
    property int selectedFader: 2
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
                text: "MUSIC INPUT"
                color: Theme.text
                font.family: Theme.monoFamily
                font.pixelSize: 10
                font.weight: Font.Bold
                font.letterSpacing: 1.05
            }
            Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:1;color:Theme.borderSoft;opacity:.78 }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.topMargin: 10
            Layout.bottomMargin: 10
            spacing: 0

            InputFader { Layout.fillWidth:true;Layout.fillHeight:true;label:"IN 1";value:root.engine.input1Gain;from:-12;to:12;selected:root.selectedFader===0;onActivated:root.selectedFader=0;onValueEdited:function(v){root.engine.input1Gain=v};accentColor:Theme.blue }
            InputFader { Layout.fillWidth:true;Layout.fillHeight:true;label:"IN 2";value:root.engine.input2Gain;from:-12;to:12;selected:root.selectedFader===1;onActivated:root.selectedFader=1;onValueEdited:function(v){root.engine.input2Gain=v};accentColor:Theme.blue }
            InputFader { Layout.fillWidth:true;Layout.fillHeight:true;label:"BT";value:root.engine.bluetoothGain;from:-12;to:12;selected:root.selectedFader===2;onActivated:root.selectedFader=2;onValueEdited:function(v){root.engine.bluetoothGain=v};active:true;accentColor:Theme.accent }
            InputFader { Layout.fillWidth:true;Layout.fillHeight:true;label:"UDISK";value:root.engine.uDiskGain;from:-12;to:12;selected:root.selectedFader===3;onActivated:root.selectedFader=3;onValueEdited:function(v){root.engine.uDiskGain=v};accentColor:Theme.violet }
            InputFader { Layout.fillWidth:true;Layout.fillHeight:true;label:"DIG";value:root.engine.digitalGain;from:-12;to:12;selected:root.selectedFader===4;onActivated:root.selectedFader=4;onValueEdited:function(v){root.engine.digitalGain=v};accentColor:Theme.amber }
        }
    }
}
