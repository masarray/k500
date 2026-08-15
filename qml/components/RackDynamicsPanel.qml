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

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 11
        spacing: 7

        RowLayout {
            Layout.fillWidth: true
            ColumnLayout {
                spacing: 0
                Text { text:"DYNAMICS"; color:Theme.textDim; font.family:Theme.fontFamily; font.pixelSize:8; font.weight:Font.DemiBold; font.letterSpacing:1.0 }
                Text { text:root.title.toUpperCase(); color:Theme.text; font.family:Theme.fontFamily; font.pixelSize:10; font.weight:Font.Bold; font.letterSpacing:.3 }
            }
            Item { Layout.fillWidth:true }
            Rectangle {
                Layout.preferredWidth: 72; Layout.preferredHeight: 24; radius:5; color:"#080C10"; border.width:1; border.color:"#3A321A"
                Text { anchors.centerIn:parent; text:"TH  "+Math.round(root.threshold)+" dB"; color:Theme.amber; font.family:Theme.fontFamily; font.pixelSize:8; font.weight:Font.Bold }
            }
            Rectangle {
                Layout.preferredWidth: 42; Layout.preferredHeight: 24; radius:5; color:"#071113"; border.width:1; border.color:Theme.accentSoft
                Text { anchors.centerIn:parent; text:"1:"+Math.round(root.ratio); color:Theme.accent; font.family:Theme.fontFamily; font.pixelSize:8; font.weight:Font.Bold }
            }
        }

        Rectangle { Layout.fillWidth:true; Layout.preferredHeight:1; color:Theme.borderSoft; opacity:.78 }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            Rectangle {
                Layout.preferredWidth: 178
                Layout.preferredHeight: 142
                Layout.alignment: Qt.AlignVCenter
                radius: 7
                color: "#050709"
                border.width: 1
                border.color: Theme.borderSoft

                Canvas {
                    id: graph
                    anchors.fill: parent
                    anchors.margins: 8
                    antialiasing: true
                    onWidthChanged: requestPaint()
                    onHeightChanged: requestPaint()
                    onPaint: {
                        var c=getContext("2d");c.reset()
                        c.strokeStyle="#182129";c.lineWidth=1;c.globalAlpha=.9
                        for(var i=1;i<4;i++){c.beginPath();c.moveTo(i*width/4,0);c.lineTo(i*width/4,height);c.stroke();c.beginPath();c.moveTo(0,i*height/4);c.lineTo(width,i*height/4);c.stroke()}
                        var t=Math.max(0,Math.min(1,(root.threshold+50)/50));
                        var tx=t*width,ty=height-t*height;
                        c.globalAlpha=1;c.strokeStyle="#F0B928";c.lineWidth=1.4;c.beginPath();c.moveTo(0,height);c.lineTo(tx,ty);c.lineTo(width,Math.max(8,ty-(width-tx)/Math.max(1,root.ratio)));c.stroke()
                        c.strokeStyle="#62DED7";c.globalAlpha=.85;c.lineWidth=1;c.beginPath();c.moveTo(0,height);c.lineTo(width,0);c.stroke();c.globalAlpha=1
                        c.fillStyle="#F0B928";c.beginPath();c.arc(tx,ty,3,0,Math.PI*2);c.fill()
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                spacing: 5

                StudioKnob {
                    visible: root.includeGate
                    Layout.fillWidth: true
                    compact: true
                    title: "GATE"
                    value: root.gate
                    from: -80; to: 0; step: 1; decimals: 0; unit: "dB"
                    accentColor: root.accentColor
                    onValueEdited: function(v){ root.gate=v }
                }
                StudioKnob {
                    Layout.fillWidth: true
                    compact: true
                    title: "THRES"
                    value: root.threshold
                    from: -50; to: 0; step: 1; decimals: 0; unit: "dB"
                    accentColor: root.accentColor
                    onValueEdited: function(v){ root.threshold=v; graph.requestPaint() }
                }
                StudioKnob {
                    Layout.fillWidth: true
                    compact: true
                    title: "RATIO"
                    value: root.ratio
                    from: 1; to: 100; step: 1; decimals: 0; unit: ""
                    accentColor: root.accentColor
                    onValueEdited: function(v){ root.ratio=v; graph.requestPaint() }
                }
                StudioKnob {
                    Layout.fillWidth: true
                    compact: true
                    title: "ATTACK"
                    value: root.attack
                    from: 1; to: 100; step: 1; decimals: 0; unit: "ms"
                    accentColor: root.accentColor
                    onValueEdited: function(v){ root.attack=v }
                }
                StudioKnob {
                    Layout.fillWidth: true
                    compact: true
                    title: "RELEASE"
                    value: root.release
                    from: 100; to: 5000; step: 100; decimals: 0; unit: "ms"
                    accentColor: root.accentColor
                    onValueEdited: function(v){ root.release=v }
                }
            }
        }
    }
}
