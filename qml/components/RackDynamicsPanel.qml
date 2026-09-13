import QtQuick
import QtQuick.Layouts
import QtQuick.Window

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

    // P1_RACK_DYNAMICS_LIVE_BRIDGE_V1
    // P1_RACK_DYNAMICS_LIVE_BRIDGE_V2
    // Do not discover StudioEngine by walking visual parents: StackLayout and
    // layout internals can interrupt that chain. Main.qml is the stable window
    // boundary and exposes both studioEngine and selectedSection.
    function studioContext() {
        var w = root.Window.window
        if (!w) return null
        var engine = w["studioEngine"]
        var section = Number(w["selectedSection"])
        if (!engine || typeof engine.editDevicePath !== "function") return null
        return { engine:engine, sectionIndex:section }
    }
    function dispatchLive(field, value) {
        var ctx = studioContext()
        if (!ctx) return
        var path = ""
        if (root.title === "Vocal Dynamics") {
            // Donor has no verified Mic gate write; gate remains read-only.
            if (field === "threshold") path = "mic.compThresholdDb"
            else if (field === "ratio") path = "mic.compRatio"
            else if (field === "attack") path = "mic.attackMs"
            else if (field === "release") path = "mic.releaseSec"
        } else if (root.title === "Output Compressor") {
            var section = ctx.sectionIndex === 4 ? "main"
                        : ctx.sectionIndex === 5 ? "surround"
                        : ctx.sectionIndex === 6 ? "center"
                        : ctx.sectionIndex === 7 ? "sub" : ""
            if (section.length) {
                if (field === "threshold") path = "outputs." + section + ".compThresholdDb"
                else if (field === "ratio") path = "outputs." + section + ".compRatio"
                else if (field === "attack") path = "outputs." + section + ".attackMs"
                else if (field === "release") path = "outputs." + section + ".releaseSec"
            }
        }
        if (!path.length) return
        ctx.engine.editDevicePath(path, field === "release" ? Number(value) / 1000.0 : value)
    }

    onThresholdChanged: if (graph) graph.requestPaint()
    onRatioChanged: if (graph) graph.requestPaint()

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

        // LOWER_RACK_SPACE_UTILIZATION_V2
        // Controls stay in a balanced two-row grid. The graph is deliberately
        // shorter than the rack body so its bottom edge aligns with the value
        // caption/readout baseline of the second knob row.
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.topMargin: 8
            Layout.bottomMargin: 10
            spacing: 10

            Rectangle {
                // COMPRESSOR_GRAPH_CAPTION_BASELINE_V1
                Layout.preferredWidth: root.includeGate ? 194 : 184
                Layout.minimumWidth: 164
                Layout.maximumWidth: 216
                Layout.preferredHeight: 227
                Layout.minimumHeight: 227
                Layout.maximumHeight: 227
                Layout.alignment: Qt.AlignTop
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
                        function finiteOr(v,fallback){var n=Number(v);return isFinite(n)?n:fallback}
                        function xx(db){return left+(db-minDb)/(maxDb-minDb)*plotW}
                        function yy(db){var t=(clamp(db,minDb,maxDb)-minDb)/(maxDb-minDb);return height-bottom-t*plotH}
                        // COMPRESSOR_GRAPH_ZERO_THRESHOLD_V2
                        // Never use `Number(value) || fallback`: 0 dB is a valid
                        // threshold and JavaScript would otherwise replace it by
                        // -20 dB. Also allow the UI's documented maximum 0 dB.
                        function outDb(input){
                            var th=clamp(finiteOr(root.threshold,-20),minDb,maxDb)
                            var r=clamp(finiteOr(root.ratio,1),1,100),knee=4
                            if(input<=th-knee/2)return input
                            if(input>=th+knee/2)return th+(input-th)/r
                            var u=(input-(th-knee/2))/knee,hard=th+(input-th)/r
                            return input+(hard-input)*u*u*(3-2*u)
                        }

                        c.globalAlpha=1;c.lineWidth=1;c.strokeStyle="#182128"
                        var gx=[-60,-48,-36,-24,-12,0],gy=[-48,-36,-24,-12,0]
                        for(var i=0;i<gx.length;i++){c.beginPath();c.moveTo(xx(gx[i]),top);c.lineTo(xx(gx[i]),height-bottom);c.stroke()}
                        for(i=0;i<gy.length;i++){c.beginPath();c.moveTo(left,yy(gy[i]));c.lineTo(width-right,yy(gy[i]));c.stroke()}

                        c.globalAlpha=.22;c.strokeStyle="#FFFFFF";c.lineWidth=1
                        for(i=0;i<12;i+=2){
                            var a=minDb+i*5,b=minDb+(i+1)*5
                            c.beginPath();c.moveTo(xx(a),yy(a));c.lineTo(xx(b),yy(b));c.stroke()
                        }

                        var th=clamp(finiteOr(root.threshold,-20),minDb,maxDb)
                        c.globalAlpha=.12;c.fillStyle=Theme.amber.toString();c.beginPath();c.moveTo(xx(th),yy(th))
                        for(i=0;i<=48;i++){var db=th+(maxDb-th)*i/48;c.lineTo(xx(db),yy(outDb(db)))}
                        c.lineTo(xx(maxDb),yy(maxDb));c.closePath();c.fill()

                        c.globalAlpha=.15;c.strokeStyle=Theme.accent.toString();c.lineWidth=6;c.lineCap="round";c.beginPath()
                        for(i=0;i<=100;i++){db=minDb+(maxDb-minDb)*i/100;if(i===0)c.moveTo(xx(db),yy(outDb(db)));else c.lineTo(xx(db),yy(outDb(db)))}c.stroke()
                        c.globalAlpha=1;c.lineWidth=2.2
                        var grad=c.createLinearGradient(left,height-bottom,width-right,top)
                        grad.addColorStop(0,Theme.accent.toString());grad.addColorStop(.58,Theme.amber.toString());grad.addColorStop(1,Theme.accent.toString())
                        c.strokeStyle=grad;c.beginPath()
                        for(i=0;i<=100;i++){db=minDb+(maxDb-minDb)*i/100;if(i===0)c.moveTo(xx(db),yy(outDb(db)));else c.lineTo(xx(db),yy(outDb(db)))}c.stroke()

                        c.strokeStyle=Theme.amber.toString();c.globalAlpha=.72;c.lineWidth=1
                        var tx=xx(th)
                        for(var sy=top;sy<height-bottom;sy+=8){c.beginPath();c.moveTo(tx,sy);c.lineTo(tx,Math.min(sy+4,height-bottom));c.stroke()}
                        c.globalAlpha=.18;c.fillStyle=Theme.amber.toString();c.beginPath();c.arc(tx,yy(th),6,0,Math.PI*2);c.fill()
                        c.globalAlpha=1;c.beginPath();c.arc(tx,yy(th),2.8,0,Math.PI*2);c.fill()
                    }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                columns: root.includeGate ? 3 : 2
                columnSpacing: 4
                rowSpacing: 5

                StudioKnob {
                    visible: root.includeGate
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumWidth: 72
                    Layout.minimumHeight: 92
                    compact: true
                    editable: false
                    title: "GATE"; value: root.gate
                    from: -80; to: 0; step: 1; decimals: 0; unit: "dB"
                    accentColor: root.accentColor
                }
                StudioKnob {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    Layout.minimumWidth: 72; Layout.minimumHeight: 92
                    compact: true
                    title: "THRES"; value: root.threshold
                    from: -50; to: 0; step: 1; decimals: 0; unit: "dB"
                    accentColor: root.accentColor
                    onValueEdited: function(v){ root.threshold=v; root.dispatchLive("threshold",v); graph.requestPaint() }
                }
                StudioKnob {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    Layout.minimumWidth: 72; Layout.minimumHeight: 92
                    compact: true
                    title: "RATIO"; value: root.ratio
                    from: 1; to: 100; step: 1; decimals: 0; unit: ""; valuePrefix: "1:"
                    accentColor: root.accentColor
                    onValueEdited: function(v){ root.ratio=v; root.dispatchLive("ratio",v); graph.requestPaint() }
                }
                StudioKnob {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    Layout.minimumWidth: 72; Layout.minimumHeight: 92
                    compact: true
                    title: "ATTACK"; value: root.attack
                    from: 1; to: 100; step: 1; decimals: 0; unit: "ms"
                    accentColor: root.accentColor
                    onValueEdited: function(v){ root.attack=v; root.dispatchLive("attack",v) }
                }
                StudioKnob {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    Layout.minimumWidth: 72; Layout.minimumHeight: 92
                    compact: true
                    title: "RELEASE"; value: root.release
                    from: 20; to: 5000; step: 10; decimals: 0; unit: "ms"
                    accentColor: root.accentColor
                    onValueEdited: function(v){ root.release=v; root.dispatchLive("release",v) }
                }
                Item {
                    visible: root.includeGate
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumWidth: 72
                    Layout.minimumHeight: 92
                }
            }
        }
    }
}
