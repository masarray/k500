import QtQuick
import QtQuick.Layouts

StudioPanel {
    id: root

    required property var bandModel
    property string sectionLabel: "Mic A"
    property bool showMicSelector: false
    property int micChannel: 0
    property bool eqLinked: false
    property int selectedIndex: 0
    property real selectedFreq: 80
    property real selectedGain: 0
    property real selectedQ: 1
    readonly property var bands: bandModel
    readonly property real leftPad: 56
    readonly property real rightPad: 22
    readonly property real topPad: 24
    readonly property real bottomPad: 38
    readonly property real plotBottom: Math.max(topPad + 120, graph.height - bottomPad)
    readonly property var colors: ["#5EDDD4", "#F0B928", "#F0B928", "#F0B928", "#F0B928", "#F0B928", "#F0B928", "#F0B928", "#F0B928", "#F0B928"]

    signal micChannelRequested(int channel)
    signal eqLinkRequested(bool linked)

    implicitHeight: 500
    accentTop: false

    function clamp(v,a,b){ return Math.max(a,Math.min(b,v)) }
    function safeQ(q){ return clamp(Number(q)||0.7,0.1,30) }
    function normF(f){ return Math.log(clamp(f,20,20000)/20)/Math.log(1000) }
    function freq(n){ return 20*Math.pow(1000,clamp(n,0,1)) }
    function xFor(f){ return leftPad+normF(f)*Math.max(1,graph.width-leftPad-rightPad) }
    function freqForX(x){
        var raw=freq((x-leftPad)/Math.max(1,graph.width-leftPad-rightPad))
        if(raw<100)return Math.round(raw)
        if(raw<1000)return Math.round(raw/5)*5
        return Math.round(raw/10)*10
    }
    function yFor(g){ return topPad+(24-clamp(g,-24,24))/48*Math.max(1,plotBottom-topPad) }
    function gainForY(y){
        var g=24-clamp((y-topPad)/Math.max(1,plotBottom-topPad),0,1)*48
        g=Math.round(g*10)/10
        return Math.abs(g)<0.3?0:g
    }
    function colorFor(i){ return colors[i%colors.length] }
    function dbLin(db){ return Math.pow(10,db/20) }
    function fmtF(f){ return f>=1000?((f/1000)%1===0?(f/1000).toFixed(0):(f/1000).toFixed(1))+"k":Math.round(f).toString() }
    function typeShort(t){ t=String(t||"BELL"); return t==="LOW SHELF"?"LS":t==="HIGH SHELF"?"HS":"P" }

    function peak(f,q,g){
        var sr=48000,A=dbLin(g/2),w=2*Math.PI*clamp(f,1,sr/2-1)/sr
        var a=Math.sin(w)/(2*safeQ(q)),c=Math.cos(w),a0=1+a/A
        return {b0:(1+a*A)/a0,b1:-2*c/a0,b2:(1-a*A)/a0,a1:-2*c/a0,a2:(1-a/A)/a0}
    }
    function shelf(f,q,g,hi){
        var sr=48000,A=dbLin(g/2),w=2*Math.PI*clamp(f,1,sr/2-1)/sr,c=Math.cos(w),s=Math.sin(w)
        var sl=clamp(safeQ(q),0.1,10),rad=Math.max(0.000001,(A+1/A)*(1/sl-1)+2)
        var a=s/2*Math.sqrt(rad),b=2*Math.sqrt(A)*a,a0
        if(!hi){
            a0=(A+1)+(A-1)*c+b
            return {b0:A*((A+1)-(A-1)*c+b)/a0,b1:2*A*((A-1)-(A+1)*c)/a0,b2:A*((A+1)-(A-1)*c-b)/a0,a1:-2*((A-1)+(A+1)*c)/a0,a2:((A+1)+(A-1)*c-b)/a0}
        }
        a0=(A+1)-(A-1)*c+b
        return {b0:A*((A+1)+(A-1)*c+b)/a0,b1:-2*A*((A-1)+(A+1)*c)/a0,b2:A*((A+1)+(A-1)*c-b)/a0,a1:2*((A-1)-(A+1)*c)/a0,a2:((A+1)-(A-1)*c-b)/a0}
    }
    function mag(c,f){
        var w=2*Math.PI*clamp(f,1,23999)/48000,c1=Math.cos(w),s1=Math.sin(w),c2=Math.cos(2*w),s2=Math.sin(2*w)
        var br=c.b0+c.b1*c1+c.b2*c2,bi=-(c.b1*s1+c.b2*s2),ar=1+c.a1*c1+c.a2*c2,ai=-(c.a1*s1+c.a2*s2)
        return 10*Math.log(Math.max((br*br+bi*bi)/Math.max(1e-12,ar*ar+ai*ai),1e-12))/Math.LN10
    }
    function bandDb(b,f){
        if(!b||Math.abs(b.gain)<0.001)return 0
        var t=String(b.typeName||"BELL").toUpperCase(),c
        if(t.indexOf("LOW")>=0||t==="LS")c=shelf(b.freq,b.q,b.gain,false)
        else if(t.indexOf("HIGH")>=0||t==="HS")c=shelf(b.freq,b.q,b.gain,true)
        else c=peak(b.freq,b.q,b.gain)
        return mag(c,f)
    }
    function bessel(order,r){
        var co=order===4?[105,105,45,10,1]:order===3?[15,15,6,1]:[3,3,1]
        var sc=order===4?2.113917674904216:order===3?1.7556723686812106:1.3616541287161308
        var xx=Math.max(0,r)*sc,re=0,im=0
        for(var p=0;p<co.length;++p){var m=co[p]*Math.pow(xx,p),ph=p*Math.PI/2;re+=m*Math.cos(ph);im+=m*Math.sin(ph)}
        return co[0]/Math.max(1e-12,Math.sqrt(re*re+im*im))
    }
    function crossOne(kind,label,cut,f){
        label=String(label||"LR 24").toUpperCase()
        var order=label.indexOf("24")>=0?4:label.indexOf("18")>=0?3:2
        var r=kind==="lpf"?Math.max(f,1)/Math.max(cut,1):Math.max(cut,1)/Math.max(f,1),m
        if(label.indexOf("BESSEL")>=0)m=bessel(order,r)
        else if(label.indexOf("LR")>=0){var b=1/Math.sqrt(1+Math.pow(r,4));m=b*b}
        else m=1/Math.sqrt(1+Math.pow(r,2*order))
        return 20*Math.log(Math.max(m,1e-12))/Math.LN10
    }
    function crossDb(f){
        var d=0,h=Number(bands.hpfHz)||20,l=Number(bands.lpfHz)||20000
        if(h>20.001)d+=crossOne("hpf",bands.hpType,h,f)
        if(l<19999.999)d+=crossOne("lpf",bands.lpType,l,f)
        return d
    }
    function totalDb(f){ var d=crossDb(f); for(var i=0;i<bands.count;++i)d+=bandDb(bands.get(i),f); return clamp(d,-48,48) }

    function selectBand(i){
        if(!bands||bands.count<1)return
        selectedIndex=clamp(i,0,bands.count-1)
        var b=bands.get(selectedIndex)
        selectedFreq=b.freq;selectedGain=b.gain;selectedQ=b.q
        curve.requestPaint()
    }
    function updateSelected(){ bands.setBand(selectedIndex,selectedFreq,selectedGain,selectedQ);curve.requestPaint() }
    function setSelectedFrequency(v){selectedFreq=clamp(v,20,20000);updateSelected()}
    function setSelectedGain(v){selectedGain=clamp(v,-24,24);updateSelected()}
    function setSelectedQValue(v){selectedQ=clamp(v,0.1,30);updateSelected()}
    function resetSelected(){bands.resetBand(selectedIndex);selectBand(selectedIndex)}
    function resetAll(){bands.resetAll();selectBand(0)}

    Connections {
        target: bands
        function onBandChanged(){root.selectBand(Math.min(root.selectedIndex,root.bands.count-1));curve.requestPaint()}
        function onCrossoverChanged(){curve.requestPaint()}
    }
    onBandModelChanged: Qt.callLater(function(){root.selectBand(0);curve.requestPaint()})
    Component.onCompleted: selectBand(0)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            spacing: 8

            ColumnLayout {
                spacing: -1
                Text {
                    text: "PARAMETRIC EQ   ·   " + root.bands.count + " BANDS"
                    color: Theme.textDim
                    font.family: Theme.monoFamily
                    font.pixelSize: 9
                    font.weight: Font.Medium
                    font.letterSpacing: 1.35
                }
                Text {
                    text: root.sectionLabel
                    color: Theme.text
                    font.family: Theme.displayFamily
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                }
            }

            Item { Layout.fillWidth: true }

            RowLayout {
                visible: root.showMicSelector
                spacing: 7
                SoftButton { Layout.preferredWidth:58; Layout.preferredHeight:27; text:"Mic A"; compact:true; checked:root.micChannel===0; onClicked:root.micChannelRequested(0) }
                SoftButton { Layout.preferredWidth:58; Layout.preferredHeight:27; text:"Mic B"; compact:true; checked:root.micChannel===1; onClicked:root.micChannelRequested(1) }
                SoftButton { Layout.preferredWidth:78; Layout.preferredHeight:27; text:"EQ LINK"; compact:true; checked:root.eqLinked; onClicked:root.eqLinkRequested(!root.eqLinked) }
            }

            RowLayout {
                visible: !root.showMicSelector
                spacing: 7
                SoftButton { Layout.preferredWidth:50; text:"FLAT"; compact:true; onClicked:root.resetAll() }
                SoftButton { Layout.preferredWidth:50; text:"A/B"; compact:true }
            }
        }

        Rectangle { Layout.fillWidth:true; Layout.preferredHeight:1; color:Theme.borderSoft; opacity:.75 }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 12
            Layout.bottomMargin: 7

            Rectangle {
                id: graph
                anchors.fill: parent
                radius: 10
                color: "#080B0E"
                border.width: 1
                border.color: "#020304"
                clip: true

                Repeater {
                    model:[20,30,50,70,100,200,500,1000,2000,5000,10000,20000]
                    delegate: Rectangle {
                        required property var modelData
                        x:root.xFor(modelData);y:root.topPad;width:1;height:root.plotBottom-root.topPad
                        color:"#FFFFFF";opacity:.05
                        Text {
                            anchors.horizontalCenter:parent.horizontalCenter
                            y:root.plotBottom+10
                            text:root.fmtF(modelData)
                            color:Theme.textDim
                            opacity:.82
                            font.family:Theme.monoFamily
                            font.pixelSize:9
                        }
                    }
                }
                Repeater {
                    model:[-24,-18,-12,-6,0,6,12,18,24]
                    delegate: Rectangle {
                        required property var modelData
                        x:root.leftPad;y:root.yFor(modelData);width:graph.width-root.leftPad-root.rightPad;height:modelData===0?1.1:1
                        color:modelData===0?Theme.accent:"#FFFFFF";opacity:modelData===0?.20:.05
                        Text {
                            anchors.right:parent.left;anchors.rightMargin:10;anchors.verticalCenter:parent.verticalCenter
                            text:modelData>0?"+"+modelData:modelData
                            color:Theme.textDim
                            font.family:Theme.monoFamily
                            font.pixelSize:9
                        }
                    }
                }

                Canvas {
                    id:curve
                    anchors.fill:parent
                    antialiasing:true
                    onWidthChanged:requestPaint()
                    onHeightChanged:requestPaint()
                    onPaint:{
                        var c=getContext("2d");c.reset()
                        var n=Math.max(280,Math.floor(width/3)),i,t,f,x,y,zero=root.yFor(0)

                        // Composite fill to the 0 dB datum, same visual hierarchy as the web editor.
                        c.beginPath()
                        for(i=0;i<=n;++i){t=i/n;f=root.freq(t);x=root.leftPad+t*(width-root.leftPad-root.rightPad);y=root.yFor(root.totalDb(f));if(i===0)c.moveTo(x,y);else c.lineTo(x,y)}
                        c.lineTo(width-root.rightPad,zero);c.lineTo(root.leftPad,zero);c.closePath()
                        var fill=c.createLinearGradient(0,root.topPad,0,root.plotBottom)
                        fill.addColorStop(0,"rgba(94,221,212,0.24)");fill.addColorStop(.55,"rgba(94,221,212,0.08)");fill.addColorStop(1,"rgba(94,221,212,0)")
                        c.fillStyle=fill;c.fill()

                        // Per-band response.
                        for(var b=0;b<root.bands.count;++b){
                            var band=root.bands.get(b);if(Math.abs(band.gain)<.05)continue
                            c.beginPath();for(i=0;i<=n;++i){t=i/n;f=root.freq(t);x=root.leftPad+t*(width-root.leftPad-root.rightPad);y=root.yFor(root.bandDb(band,f));if(i===0)c.moveTo(x,y);else c.lineTo(x,y)}
                            c.globalAlpha=b===root.selectedIndex?.65:.13;c.lineWidth=b===root.selectedIndex?1.6:1;c.strokeStyle=b===root.selectedIndex?Theme.amber.toString():Theme.accent.toString();c.stroke()
                        }

                        // Crossover response under the composite.
                        c.beginPath();for(i=0;i<=n;++i){t=i/n;f=root.freq(t);x=root.leftPad+t*(width-root.leftPad-root.rightPad);y=root.yFor(root.crossDb(f));if(i===0)c.moveTo(x,y);else c.lineTo(x,y)}
                        c.globalAlpha=.40;c.lineWidth=1.2;c.strokeStyle=Theme.amber.toString();c.stroke()

                        // Composite: dark separation, cyan glow, cyan→amber→cyan core.
                        c.beginPath();for(i=0;i<=n;++i){t=i/n;f=root.freq(t);x=root.leftPad+t*(width-root.leftPad-root.rightPad);y=root.yFor(root.totalDb(f));if(i===0)c.moveTo(x,y);else c.lineTo(x,y)}
                        c.globalAlpha=.88;c.lineWidth=6.2;c.strokeStyle="#020304";c.stroke()
                        c.globalAlpha=.18;c.lineWidth=8;c.strokeStyle=Theme.accent.toString();c.stroke()
                        c.globalAlpha=1;c.lineWidth=3.2
                        var stroke=c.createLinearGradient(root.leftPad,0,width-root.rightPad,0)
                        stroke.addColorStop(0,Theme.accent.toString());stroke.addColorStop(.55,Theme.amber.toString());stroke.addColorStop(1,Theme.accent.toString())
                        c.strokeStyle=stroke;c.stroke();c.globalAlpha=1
                    }
                }

                Item {
                    x:root.xFor(root.bands.hpfHz)-12;y:root.topPad;width:24;height:root.plotBottom-root.topPad
                    Repeater { model:Math.max(1,Math.floor(parent.height/9)); delegate:Rectangle{required property int index;width:1;height:4;x:12;y:index*9;color:Theme.amber;opacity:.35} }
                    Text { x:24;y:6;text:Math.round(root.bands.hpfHz)+" Hz";color:Theme.amber;font.family:Theme.monoFamily;font.pixelSize:9;font.weight:Font.Bold }
                    Rectangle {
                        anchors.horizontalCenter:parent.horizontalCenter;y:root.yFor(0)-root.topPad-9;width:18;height:18;radius:9
                        color:Theme.amber;border.width:1;border.color:"#090805"
                        Text{anchors.centerIn:parent;text:"HP";color:"#191204";font.family:Theme.monoFamily;font.pixelSize:7;font.weight:Font.Bold;font.italic:true}
                    }
                    MouseArea { anchors.fill:parent;cursorShape:Qt.SizeHorCursor;onPositionChanged:function(e){if(pressed){var p=mapToItem(graph,e.x,e.y);root.bands.setHpfHz(root.freqForX(p.x))}} }
                }
                Item {
                    x:root.xFor(root.bands.lpfHz)-12;y:root.topPad;width:24;height:root.plotBottom-root.topPad
                    Repeater { model:Math.max(1,Math.floor(parent.height/9)); delegate:Rectangle{required property int index;width:1;height:4;x:12;y:index*9;color:Theme.amber;opacity:.35} }
                    Text { anchors.right:parent.horizontalCenter;anchors.rightMargin:12;y:6;text:root.fmtF(root.bands.lpfHz)+" Hz";color:Theme.amber;font.family:Theme.monoFamily;font.pixelSize:9;font.weight:Font.Bold }
                    Rectangle {
                        anchors.horizontalCenter:parent.horizontalCenter;y:root.yFor(0)-root.topPad-9;width:18;height:18;radius:9
                        color:Theme.amber;border.width:1;border.color:"#090805"
                        Text{anchors.centerIn:parent;text:"LP";color:"#191204";font.family:Theme.monoFamily;font.pixelSize:7;font.weight:Font.Bold;font.italic:true}
                    }
                    MouseArea { anchors.fill:parent;cursorShape:Qt.SizeHorCursor;onPositionChanged:function(e){if(pressed){var p=mapToItem(graph,e.x,e.y);root.bands.setLpfHz(root.freqForX(p.x))}} }
                }

                Repeater {
                    model:root.bands
                    delegate:Item {
                        required property int index
                        required property real freq
                        required property real gain
                        required property real q
                        required property string typeName
                        x:root.xFor(freq)-18;y:root.yFor(gain)-18;width:36;height:36
                        Rectangle { anchors.centerIn:parent;width:index===root.selectedIndex?36:26;height:width;radius:width/2;color:index===root.selectedIndex?Theme.accent:Theme.amber;opacity:index===root.selectedIndex?.17:.12 }
                        Rectangle {
                            anchors.centerIn:parent;width:index===root.selectedIndex?21:16;height:width;radius:width/2
                            color:index===root.selectedIndex?Theme.accent:Theme.amber;border.width:1.5;border.color:"#090B0C"
                            Text{anchors.centerIn:parent;text:index+1;color:"#0A0D0F";font.family:Theme.monoFamily;font.pixelSize:9;font.weight:Font.Bold}
                        }
                        MouseArea {
                            anchors.fill:parent;cursorShape:Qt.SizeAllCursor
                            onPressed:root.selectBand(index)
                            onPositionChanged:function(e){if(!pressed)return;var p=mapToItem(graph,e.x,e.y);root.selectedFreq=root.freqForX(p.x);root.selectedGain=root.gainForY(p.y);root.updateSelected()}
                            onDoubleClicked:{root.selectBand(index);root.setSelectedGain(0)}
                        }
                    }
                }

                BandInspector {
                    id:inspector
                    bandModel:root.bands
                    bandIndex:root.selectedIndex
                    frequency:root.selectedFreq
                    gain:root.selectedGain
                    q:root.selectedQ
                    accentColor:root.colorFor(root.selectedIndex)
                    width:Math.min(320,graph.width-30)
                    height:86
                    x:root.clamp(root.xFor(root.selectedFreq)-width/2,15,graph.width-width-15)
                    readonly property bool placeBelow: root.yFor(root.selectedGain) < graph.height-132
                    y:placeBelow?Math.min(graph.height-height-12,root.yFor(root.selectedGain)+18):Math.max(12,root.yFor(root.selectedGain)-height-18)
                    Behavior on x { SmoothedAnimation { velocity:1800 } }
                    onFrequencyEdited:function(v){root.setSelectedFrequency(v)}
                    onGainEdited:function(v){root.setSelectedGain(v)}
                    onQEdited:function(v){root.setSelectedQValue(v)}
                    onResetRequested:root.resetSelected()
                }
            }
        }

        RowLayout {
            Layout.fillWidth:true
            Layout.preferredHeight:55
            Layout.leftMargin:12
            Layout.rightMargin:12
            Layout.bottomMargin:10
            spacing:6
            Repeater {
                model:root.bands
                delegate:Rectangle {
                    required property int index
                    required property real freq
                    required property real gain
                    required property real q
                    required property string typeName
                    Layout.fillWidth:true;Layout.preferredHeight:43;radius:9
                    gradient:Gradient {
                        GradientStop{position:0;color:index===root.selectedIndex?"#17363A":"#11171C"}
                        GradientStop{position:1;color:index===root.selectedIndex?"#0B171A":"#090D11"}
                    }
                    border.width:1;border.color:index===root.selectedIndex?Theme.accentSoft:"#242C33"
                    Column {
                        anchors.fill:parent;anchors.margins:6;spacing:0
                        Row { width:parent.width
                            Text{text:"B"+(index+1);color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8}
                            Item{width:Math.max(0,parent.width-34);height:1}
                            Text{text:root.typeShort(typeName);color:index===root.selectedIndex?Theme.accent:Theme.textSoft;font.family:Theme.monoFamily;font.pixelSize:8;font.weight:Font.Bold}
                        }
                        Row { width:parent.width
                            Text{text:root.fmtF(freq);color:Theme.amber;font.family:Theme.monoFamily;font.pixelSize:9;font.weight:Font.Bold}
                            Item{width:Math.max(0,parent.width-54);height:1}
                            Text{text:(gain>0?"+":"")+gain.toFixed(1);color:Theme.textSoft;font.family:Theme.monoFamily;font.pixelSize:8}
                        }
                    }
                    MouseArea{anchors.fill:parent;cursorShape:Qt.PointingHandCursor;onClicked:root.selectBand(index)}
                }
            }
        }
    }
}
