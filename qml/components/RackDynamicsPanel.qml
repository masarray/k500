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
            Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:1;color:Theme.borderSoft;opacity:.72 }
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
                color: "#040608"
                border.width: 1
                border.color: "#020304"

                Canvas {
                    id: graph
                    anchors.fill: parent
                    anchors.margins: 7
                    antialiasing: true
                    onWidthChanged: requestPaint()
                    onHeightChanged: requestPaint()
                    onPaint: {
                        var c=getContext("2d");c.reset()
                        var minDb=-60,maxDb=0
                        var left=width*(36/400),right=width*(22/400),top=height*(18/206),bottom=height*(30/206)
                        var plotW=Math.max(1,width-left-right),plotH=Math.max(1,height-top-bottom)
                        function clamp(v,a,b){return Math.max(a,Math.min(b,v))}
                        function xx(db){return left+(db-minDb)/(maxDb-minDb)*plotW}
                        function yy(db){var t=(clamp(db,minDb,maxDb)-minDb)/(maxDb-minDb);return height-bottom-t*plotH}
                        function outDb(input){
                            var th=clamp(Number(root.threshold)||-20,minDb,-1),r=clamp(Number(root.ratio)||1,1,100),knee=4
                            if(input<=th-knee/2)return input
                            if(input>=th+knee/2)return th+(input-th)/r
                            var u=(input-(th-knee/2))/knee,hard=th+(input-th)/r
                            return input+(hard-input)*u*u*(3-2*u)
                        }

                        // Web CompressorGraph grid.
                        c.globalAlpha=1;c.lineWidth=1;c.strokeStyle="#182128"
                        var gx=[-60,-48,-36,-24,-12,0],gy=[-48,-36,-24,-12,0]
                        for(var i=0;i<gx.length;i++){c.beginPath();c.moveTo(xx(gx[i]),top);c.lineTo(xx(gx[i]),height-bottom);c.stroke()}
                        for(i=0;i<gy.length;i++){c.beginPath();c.moveTo(left,yy(gy[i]));c.lineTo(width-right,yy(gy[i]));c.stroke()}

                        // Reference unity line, kept quiet behind the transfer curve.
                        c.globalAlpha=.22;c.strokeStyle="#FFFFFF";c.lineWidth=1
                        for(i=0;i<12;i+=2){
                            var a=minDb+i*5,b=minDb+(i+1)*5
                            c.beginPath();c.moveTo(xx(a),yy(a));c.lineTo(xx(b),yy(b));c.stroke()
                        }

                        var th=clamp(Number(root.threshold)||-20,minDb,-1)
                        // Subtle compression-area fill after threshold.
                        c.globalAlpha=.12;c.fillStyle=Theme.amber.toString();c.beginPath();c.moveTo(xx(th),yy(th))
                        for(i=0;i<=48;i++){var db=th+(maxDb-th)*i/48;c.lineTo(xx(db),yy(outDb(db)))}
                        c.lineTo(xx(maxDb),yy(maxDb));c.closePath();c.fill()

                        // Transfer curve glow + cyan→amber core, matching the Web hierarchy.
                        c.globalAlpha=.15;c.strokeStyle=Theme.accent.toString();c.lineWidth=6;c.lineCap="round";c.beginPath()
                        for(i=0;i<=100;i++){db=minDb+(maxDb-minDb)*i/100;if(i===0)c.moveTo(xx(db),yy(outDb(db)));else c.lineTo(xx(db),yy(outDb(db)))}c.stroke()
                        c.globalAlpha=1;c.lineWidth=2.2
                        var grad=c.createLinearGradient(left,height-bottom,width-right,top)
                        grad.addColorStop(0,Theme.accent.toString());grad.addColorStop(.58,Theme.amber.toString());grad.addColorStop(1,Theme.accent.toString())
                        c.strokeStyle=grad;c.beginPath()
                        for(i=0;i<=100;i++){db=minDb+(maxDb-minDb)*i/100;if(i===0)c.moveTo(xx(db),yy(outDb(db)));else c.lineTo(xx(db),yy(outDb(db)))}c.stroke()

                        // Threshold guide and marker.
                        c.strokeStyle=Theme.amber.toString();c.globalAlpha=.72;c.lineWidth=1
                        var tx=xx(th)
                        for(var sy=top;sy<height-bottom;sy+=8){c.beginPath();c.moveTo(tx,sy);c.lineTo(tx,Math.min(sy+4,height-bottom));c.stroke()}
                        c.globalAlpha=.18;c.fillStyle=Theme.amber.toString();c.beginPath();c.arc(tx,yy(th),6,0,Math.PI*2);c.fill()
                        c.globalAlpha=1;c.beginPath();c.arc(tx,yy(th),2.8,0,Math.PI*2);c.fill()
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
                    from: 1; to: 100; step: 1; decimals: 0; unit: ""; valuePrefix: "1:"
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
