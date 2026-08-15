import QtQuick
import QtQuick.Layouts

StudioPanel {
    id: root

    property string title: "Vocal Dynamics"
    property bool includeGate: false
    property real gate: -70
    property real threshold: -12
    property real ratio: 3
    property real attack: 10
    property real release: 200
    property color accentColor: Theme.accent
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
                text: root.title.toUpperCase()
                color: Theme.text
                font.family: Theme.monoFamily
                font.pixelSize: 10
                font.weight: Font.Bold
                font.letterSpacing: 1.05
            }
            RowLayout {
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                spacing: 7
                Rectangle {
                    Layout.preferredWidth: 72; Layout.preferredHeight: 25; radius:8; color:"#0A0B08"; border.width:1; border.color:"#342A10"
                    Text { anchors.centerIn:parent; text:"TH  "+Math.round(root.threshold)+" dB"; color:Theme.amber; font.family:Theme.monoFamily; font.pixelSize:9; font.weight:Font.Bold }
                }
                Rectangle {
                    Layout.preferredWidth: 44; Layout.preferredHeight: 25; radius:8; color:"#071113"; border.width:1; border.color:Theme.accentSoft
                    Text { anchors.centerIn:parent; text:"1:"+Math.round(root.ratio); color:Theme.accent; font.family:Theme.monoFamily; font.pixelSize:9; font.weight:Font.Bold }
                }
            }
            Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:1;color:Theme.borderSoft;opacity:.78 }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 15
            Layout.rightMargin: 15
            Layout.topMargin: 12
            Layout.bottomMargin: 12
            spacing: 14

            Rectangle {
                Layout.preferredWidth: root.includeGate ? 170 : 122
                Layout.preferredHeight: 140
                Layout.alignment: Qt.AlignVCenter
                radius: 8
                color: "#050709"
                border.width: 1
                border.color: "#020304"

                Canvas {
                    id: graph
                    anchors.fill: parent
                    anchors.margins: 8
                    antialiasing: true
                    onWidthChanged: requestPaint()
                    onHeightChanged: requestPaint()
                    onPaint: {
                        var c=getContext("2d");c.reset()
                        c.strokeStyle="#1B242B";c.lineWidth=1;c.globalAlpha=.75
                        for(var i=1;i<4;i++){c.beginPath();c.moveTo(i*width/4,0);c.lineTo(i*width/4,height);c.stroke();c.beginPath();c.moveTo(0,i*height/4);c.lineTo(width,i*height/4);c.stroke()}
                        var t=Math.max(0,Math.min(1,(root.threshold+50)/50)),tx=t*width,ty=height-t*height
                        c.globalAlpha=1;c.strokeStyle=Theme.accent.toString();c.lineWidth=1.2;c.beginPath();c.moveTo(0,height);c.lineTo(width,0);c.stroke()
                        c.strokeStyle=Theme.amber.toString();c.lineWidth=1.6;c.beginPath();c.moveTo(0,height);c.lineTo(tx,ty);c.lineTo(width,Math.max(8,ty-(width-tx)/Math.max(1,root.ratio)));c.stroke()
                        c.fillStyle=Theme.amber.toString();c.beginPath();c.arc(tx,ty,3,0,Math.PI*2);c.fill()
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                spacing: 3

                StudioKnob {
                    visible: root.includeGate
                    Layout.fillWidth: true
                    compact: true
                    title: "GATE"; value: root.gate
                    from: -80; to: 0; step: 1; decimals: 0; unit: "dB"
                    accentColor: root.accentColor
                    onValueEdited: function(v){ root.gate=v }
                }
                StudioKnob {
                    Layout.fillWidth: true; compact: true
                    title: "THRES"; value: root.threshold
                    from: -50; to: 0; step: 1; decimals: 0; unit: "dB"
                    accentColor: root.accentColor
                    onValueEdited: function(v){ root.threshold=v; graph.requestPaint() }
                }
                StudioKnob {
                    Layout.fillWidth: true; compact: true
                    title: "RATIO"; value: root.ratio
                    from: 1; to: 100; step: 1; decimals: 0; unit: ""
                    accentColor: root.accentColor
                    onValueEdited: function(v){ root.ratio=v; graph.requestPaint() }
                }
                StudioKnob {
                    Layout.fillWidth: true; compact: true
                    title: "ATTACK"; value: root.attack
                    from: 1; to: 100; step: 1; decimals: 0; unit: "ms"
                    accentColor: root.accentColor
                    onValueEdited: function(v){ root.attack=v }
                }
                StudioKnob {
                    Layout.fillWidth: true; compact: true
                    title: "RELEASE"; value: root.release
                    from: 20; to: 5000; step: 10; decimals: 0; unit: "ms"
                    accentColor: root.accentColor
                    onValueEdited: function(v){ root.release=v }
                }
            }
        }
    }
}
