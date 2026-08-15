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
                text: "PITCH SHIFTER"
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
            spacing: 6

            KeyControl {
                Layout.fillWidth: true
                Layout.preferredHeight: 56
                key: root.engine.musicKey
                onKeyEdited: function(v) { root.engine.musicKey = v }
            }
            ParameterSlider { Layout.fillWidth:true;label:"NOISE GATE";value:root.engine.noiseGate;from:-80;to:0;step:1;defaultValue:-70;decimals:0;unit:"dB";onValueEdited:function(v){root.engine.noiseGate=v} }
            ParameterSlider { Layout.fillWidth:true;label:"BASS";value:root.engine.bass;from:-12;to:12;step:.1;defaultValue:0;decimals:1;unit:"dB";onValueEdited:function(v){root.engine.bass=v} }
            ParameterSlider { Layout.fillWidth:true;label:"MID";value:root.engine.mid;from:-12;to:12;step:.1;defaultValue:0;decimals:1;unit:"dB";onValueEdited:function(v){root.engine.mid=v} }
            ParameterSlider { Layout.fillWidth:true;label:"MID FREQ";value:root.engine.midFreq;from:80;to:8000;step:10;defaultValue:1000;decimals:0;unit:"Hz";logarithmic:true;onValueEdited:function(v){root.engine.midFreq=v} }
            ParameterSlider { Layout.fillWidth:true;label:"TREBLE";value:root.engine.treble;from:-12;to:12;step:.1;defaultValue:0;decimals:1;unit:"dB";onValueEdited:function(v){root.engine.treble=v} }
        }
    }
}
