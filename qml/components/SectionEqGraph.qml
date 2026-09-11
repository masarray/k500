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
    property string selectedTarget: "band" // band | hpf | lpf
    property real selectedFreq: 80
    property real selectedGain: 0
    property real selectedQ: 1
    readonly property var bands: bandModel

    readonly property real virtualScaleX: graph.width > 0 ? graph.width / 1040.0 : 1.0
    readonly property real virtualScaleY: graph.height > 0 ? graph.height / 354.0 : 1.0
    readonly property real nodeScale: Math.min(virtualScaleX, virtualScaleY)
    readonly property real leftPad: 56 * virtualScaleX
    readonly property real rightPad: 22 * virtualScaleX
    readonly property real topPad: 24 * virtualScaleY
    readonly property real bottomPad: 38 * virtualScaleY
    readonly property real plotBottom: Math.max(topPad + 120 * virtualScaleY, graph.height - bottomPad)

    // PEQ_PREMIUM_JEWEL_NODES_V1
    // Professional EQs make control points read as small floating instruments,
    // not flat buttons. Each band keeps a restrained jewel identity while the
    // global cyan focus ring remains the single selection language of SonKuPik.
    readonly property var colors: [
        "#A977FF", "#E86A92", "#FF7659", "#F5B94C", "#9AD654",
        "#55D6A0", "#42C7D9", "#5B9CFF", "#7C78FF", "#F58A45"
    ]
    readonly property real inspectorFrequency: selectedTarget === "hpf" ? Number(bands.hpfHz) : selectedTarget === "lpf" ? Number(bands.lpfHz) : selectedFreq

    signal micChannelRequested(int channel)
    signal eqLinkRequested(bool linked)
    signal crossoverTypeRequested(string which, string value)

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
    function rawGainForY(y){
        var g=24-clamp((y-topPad)/Math.max(1,plotBottom-topPad),0,1)*48
        return Math.round(g*10)/10
    }
    function gainForY(y){
        var g=rawGainForY(y)
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
    function inspectorShouldTop(f){ return totalDb(f) < -1.5 }

    function selectBand(i){
        if(!bands||bands.count<1)return
        selectedTarget="band"
        selectedIndex=clamp(i,0,bands.count-1)
        var b=bands.get(selectedIndex)
        selectedFreq=b.freq;selectedGain=b.gain;selectedQ=b.q
        curve.requestPaint()
    }
    function selectCrossover(which){ selectedTarget=which==="lpf"?"lpf":"hpf";curve.requestPaint() }
    function updateSelected(){ bands.setBand(selectedIndex,selectedFreq,selectedGain,selectedQ);curve.requestPaint() }
    function setSelectedFrequency(v){selectedFreq=clamp(v,20,20000);updateSelected()}
    function setSelectedGain(v){selectedGain=clamp(v,-24,24);updateSelected()}
    function setSelectedQValue(v){selectedQ=clamp(v,0.1,30);updateSelected()}
    function resetSelected(){bands.resetBand(selectedIndex);selectBand(selectedIndex)}
    function setCrossoverType(which,value){
        if(which==="hpf"){
            if(typeof bands.setHpType === "function")bands.setHpType(value)
            else root.crossoverTypeRequested("hpf",value)
        }else{
            if(typeof bands.setLpType === "function")bands.setLpType(value)
            else root.crossoverTypeRequested("lpf",value)
        }
        curve.requestPaint()
    }
    function resetCrossover(which){
        if(which==="hpf"){
            bands.setHpfHz(Number(bands.defaultHpfHz)||20)
            setCrossoverType("hpf",String(bands.defaultHpType||"HP Butter 12"))
        } else {
            bands.setLpfHz(Number(bands.defaultLpfHz)||20000)
            setCrossoverType("lpf",String(bands.defaultLpType||"LP Butter 12"))
        }
        curve.requestPaint()
    }
    function resetAll(){bands.resetAll();selectBand(0)}

    Connections {
        target: bands
        function onBandChanged(){
            if(root.selectedTarget==="band")root.selectBand(Math.min(root.selectedIndex,root.bands.count-1))
            else curve.requestPaint()
        }
        function onCrossoverChanged(){curve.requestPaint()}
    }
    onBandModelChanged: Qt.callLater(function(){root.selectBand(0);curve.requestPaint()})
    Component.onCompleted: selectBand(0)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            spacing: 8

            ColumnLayout {
                spacing: -1
                Text { text:"PARAMETRIC EQ   ·   "+root.bands.count+" BANDS";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:10;font.weight:Font.Medium;font.letterSpacing:1.35 }
                Text { text:root.sectionLabel;color:Theme.text;font.family:Theme.displayFamily;font.pixelSize:14;font.weight:Font.DemiBold }
            }
            Item { Layout.fillWidth: true }
            RowLayout {
                visible: root.showMicSelector
                spacing: 7
                SoftButton { Layout.preferredWidth:58;Layout.preferredHeight:27;text:"Mic A";compact:true;checked:root.micChannel===0;onClicked:root.micChannelRequested(0) }
                SoftButton { Layout.preferredWidth:58;Layout.preferredHeight:27;text:"Mic B";compact:true;checked:root.micChannel===1;onClicked:root.micChannelRequested(1) }
                SoftButton { Layout.preferredWidth:78;Layout.preferredHeight:27;text:"EQ LINK";compact:true;checked:root.eqLinked;onClicked:root.eqLinkRequested(!root.eqLinked) }
            }
            RowLayout {
                visible: !root.showMicSelector
                spacing: 7
                SoftButton { Layout.preferredWidth:50;text:"FLAT";compact:true;onClicked:root.resetAll() }
                SoftButton { Layout.preferredWidth:50;text:"A/B";compact:true }
            }
        }

        Rectangle { Layout.fillWidth:true;Layout.preferredHeight:1;color:Theme.borderSoft;opacity:.72 }

        Item {
            Layout.fillWidth:true
            Layout.fillHeight:true
            Layout.margins:12

            Rectangle {
                id:graph
                anchors.fill:parent
                radius:10
                color:"#040507"
                border.width:1
                border.color:"#010203"
                clip:true

                // PEQ_INTERACTION_PARITY_V1
                // Keep the Qt graph aligned with the web/FabFilter-style workflow:
                // wheel = selected-band Q, Shift = fine, Ctrl/Cmd + vertical drag = Q,
                // normal drag = frequency/gain with magnetic 0 dB snap.
                WheelHandler {
                    id:peqWheelHandler
                    target:null
                    onWheel:function(event){
                        if(root.selectedTarget!=="band"){
                            event.accepted=false
                            return
                        }
                        var delta=event.angleDelta.y
                        if(delta===0)delta=event.pixelDelta.y
                        if(delta===0){event.accepted=false;return}
                        var direction=delta>0?1:-1
                        var fine=(event.modifiers & Qt.ShiftModifier)!==0
                        var step=fine?0.02:0.1
                        var nextQ=Math.round(root.clamp(root.safeQ(root.selectedQ)+direction*step,0.1,30)*100)/100
                        root.setSelectedQValue(nextQ)
                        event.accepted=true
                    }
                }

                Repeater {
                    model:[20,30,50,70,100,200,500,1000,2000,5000,10000,20000]
                    delegate:Item {
                        required property var modelData
                        x:root.xFor(modelData)-0.5;y:0;width:1;height:graph.height
                        Rectangle{x:0;y:root.topPad;width:1;height:root.plotBottom-root.topPad;color:"#FFFFFF";opacity:.06}
                        Text{anchors.horizontalCenter:parent.horizontalCenter;y:root.plotBottom+10*root.virtualScaleY;text:root.fmtF(modelData);color:"#A6B1BA";font.family:Theme.monoFamily;font.pixelSize:10;font.weight:Font.Medium}
                    }
                }
                Repeater {
                    model:[-24,-18,-12,-6,0,6,12,18,24]
                    delegate:Item {
                        required property var modelData
                        x:0;y:root.yFor(modelData)-0.5;width:graph.width;height:1
                        Rectangle{id:hGridLine;x:root.leftPad;y:0;width:graph.width-root.leftPad-root.rightPad;height:modelData===0?1.1:1;color:modelData===0?Theme.accent:"#FFFFFF";opacity:modelData===0?.18:.06}
                        Text{anchors.right:hGridLine.left;anchors.rightMargin:10*root.virtualScaleX;anchors.verticalCenter:hGridLine.verticalCenter;text:modelData>0?"+"+modelData:modelData;color:"#A6B1BA";font.family:Theme.monoFamily;font.pixelSize:10;font.weight:Font.Medium}
                    }
                }

                // P1_NATIVE_PEQ_SCENEGRAPH_V1
                // Response math is cached in C++ and the curve is retained as
                // scene-graph geometry. QML still owns every interaction/handle.
                EqCurveItem {
                    id:curve
                    anchors.fill:parent
                    bandModel:root.bands
                    selectedIndex:root.selectedIndex
                    selectedTarget:root.selectedTarget
                    leftPad:root.leftPad
                    rightPad:root.rightPad
                    topPad:root.topPad
                    plotBottom:root.plotBottom
                    accentColor:Theme.accent
                    amberColor:Theme.amber
                }

                Item {
                    id:hpfGuide
                    readonly property bool selected:root.selectedTarget==="hpf"
                    width:28*root.virtualScaleX;height:root.plotBottom-root.topPad
                    x:root.xFor(root.bands.hpfHz)-width/2;y:root.topPad
                    Repeater{model:Math.max(1,Math.floor(parent.height/9));delegate:Rectangle{required property int index;width:1;height:4;x:parent.width/2;y:index*9;color:Theme.amber;opacity:.28}}
                    Text{x:parent.width/2+12*root.virtualScaleX;y:6*root.virtualScaleY;text:Math.round(root.bands.hpfHz)+" Hz";color:Theme.amber;font.family:Theme.monoFamily;font.pixelSize:10;font.weight:Font.Bold}
                    Item {
                        id:hpfNode
                        anchors.horizontalCenter:parent.horizontalCenter
                        y:root.yFor(0)-root.topPad-height/2
                        width:30*root.nodeScale;height:width
                        Rectangle {
                            anchors.centerIn:parent
                            width:(hpfGuide.selected?30:23)*root.nodeScale;height:width;radius:width/2
                            color:Theme.amber;opacity:hpfGuide.selected?.15:.055;antialiasing:true
                            Behavior on width{NumberAnimation{duration:75}}
                        }
                        Rectangle {
                            anchors.centerIn:parent
                            width:(hpfGuide.selected?24:20)*root.nodeScale;height:width;radius:width/2
                            color:"transparent";border.width:hpfGuide.selected?1.8:1.2
                            border.color:hpfGuide.selected?Theme.accent:Theme.amber;antialiasing:true
                            Behavior on width{NumberAnimation{duration:75}}
                            Behavior on border.color{ColorAnimation{duration:75}}
                        }
                        Rectangle {
                            id:hpfCore
                            anchors.centerIn:parent
                            width:(hpfGuide.selected?18:16)*root.nodeScale;height:width;radius:width/2
                            color:"#070B0E";border.width:1;border.color:"#5FFFBE00";antialiasing:true
                            Rectangle{width:4*root.nodeScale;height:2*root.nodeScale;radius:height/2;x:3*root.nodeScale;y:2.5*root.nodeScale;color:"#FFFFFF";opacity:hpfGuide.selected?.38:.20;antialiasing:true}
                            Text{anchors.centerIn:parent;text:"HP";color:hpfGuide.selected?Theme.text:Theme.amber;font.family:Theme.monoFamily;font.pixelSize:7;font.weight:Font.Bold;font.letterSpacing:-.2}
                        }
                    }
                    MouseArea{anchors.fill:parent;cursorShape:Qt.SizeHorCursor;onPressed:root.selectCrossover("hpf");onPositionChanged:function(e){if(pressed){var p=mapToItem(graph,e.x,e.y);root.bands.setHpfHz(root.freqForX(p.x))}}}
                }

                Item {
                    id:lpfGuide
                    readonly property bool selected:root.selectedTarget==="lpf"
                    width:28*root.virtualScaleX;height:root.plotBottom-root.topPad
                    x:root.xFor(root.bands.lpfHz)-width/2;y:root.topPad
                    Repeater{model:Math.max(1,Math.floor(parent.height/9));delegate:Rectangle{required property int index;width:1;height:4;x:parent.width/2;y:index*9;color:Theme.amber;opacity:.28}}
                    Text{anchors.right:parent.horizontalCenter;anchors.rightMargin:12*root.virtualScaleX;y:6*root.virtualScaleY;text:root.fmtF(root.bands.lpfHz)+" Hz";color:Theme.amber;font.family:Theme.monoFamily;font.pixelSize:10;font.weight:Font.Bold}
                    Item {
                        id:lpfNode
                        anchors.horizontalCenter:parent.horizontalCenter
                        y:root.yFor(0)-root.topPad-height/2
                        width:30*root.nodeScale;height:width
                        Rectangle {
                            anchors.centerIn:parent
                            width:(lpfGuide.selected?30:23)*root.nodeScale;height:width;radius:width/2
                            color:Theme.amber;opacity:lpfGuide.selected?.15:.055;antialiasing:true
                            Behavior on width{NumberAnimation{duration:75}}
                        }
                        Rectangle {
                            anchors.centerIn:parent
                            width:(lpfGuide.selected?24:20)*root.nodeScale;height:width;radius:width/2
                            color:"transparent";border.width:lpfGuide.selected?1.8:1.2
                            border.color:lpfGuide.selected?Theme.accent:Theme.amber;antialiasing:true
                            Behavior on width{NumberAnimation{duration:75}}
                            Behavior on border.color{ColorAnimation{duration:75}}
                        }
                        Rectangle {
                            anchors.centerIn:parent
                            width:(lpfGuide.selected?18:16)*root.nodeScale;height:width;radius:width/2
                            color:"#070B0E";border.width:1;border.color:"#5FFFBE00";antialiasing:true
                            Rectangle{width:4*root.nodeScale;height:2*root.nodeScale;radius:height/2;x:3*root.nodeScale;y:2.5*root.nodeScale;color:"#FFFFFF";opacity:lpfGuide.selected?.38:.20;antialiasing:true}
                            Text{anchors.centerIn:parent;text:"LP";color:lpfGuide.selected?Theme.text:Theme.amber;font.family:Theme.monoFamily;font.pixelSize:7;font.weight:Font.Bold;font.letterSpacing:-.2}
                        }
                    }
                    MouseArea{anchors.fill:parent;cursorShape:Qt.SizeHorCursor;onPressed:root.selectCrossover("lpf");onPositionChanged:function(e){if(pressed){var p=mapToItem(graph,e.x,e.y);root.bands.setLpfHz(root.freqForX(p.x))}}}
                }

                Repeater {
                    model:root.bands
                    delegate:Item {
                        id:bandNode
                        required property int index
                        required property real freq
                        required property real gain
                        required property real q
                        required property string typeName
                        property real dragLastX:0
                        property real dragLastY:0
                        readonly property bool selected:root.selectedTarget==="band"&&index===root.selectedIndex
                        readonly property bool hovered:bandMouse.containsMouse
                        readonly property color bandColor:root.colorFor(index)
                        readonly property real haloVirtual:selected?33:hovered?28:24
                        readonly property real ringVirtual:selected?27:hovered?23:20
                        readonly property real coreVirtual:selected?21:hovered?19:17
                        width:36*root.nodeScale;height:width
                        x:root.xFor(freq)-width/2;y:root.yFor(gain)-height/2

                        Rectangle {
                            anchors.centerIn:parent
                            width:parent.haloVirtual*root.nodeScale;height:width;radius:width/2
                            color:parent.bandColor
                            opacity:parent.selected?.16:parent.hovered?.10:.045
                            antialiasing:true
                            Behavior on width{NumberAnimation{duration:70;easing.type:Easing.OutQuad}}
                            Behavior on opacity{NumberAnimation{duration:70}}
                        }
                        Rectangle {
                            anchors.centerIn:parent
                            width:parent.ringVirtual*root.nodeScale;height:width;radius:width/2
                            color:"transparent"
                            border.width:parent.selected?1.9:parent.hovered?1.5:1.15
                            border.color:parent.selected?Theme.accent:Qt.rgba(parent.bandColor.r,parent.bandColor.g,parent.bandColor.b,parent.hovered?.95:.78)
                            antialiasing:true
                            Behavior on width{NumberAnimation{duration:70;easing.type:Easing.OutQuad}}
                            Behavior on border.color{ColorAnimation{duration:70}}
                        }
                        Rectangle {
                            id:bandCore
                            anchors.centerIn:parent
                            width:parent.coreVirtual*root.nodeScale;height:width;radius:width/2
                            color:"#070B0E"
                            border.width:1
                            border.color:Qt.rgba(parent.bandColor.r,parent.bandColor.g,parent.bandColor.b,parent.selected?.78:.48)
                            antialiasing:true
                            Behavior on width{NumberAnimation{duration:70;easing.type:Easing.OutQuad}}

                            Rectangle {
                                anchors.centerIn:parent
                                width:Math.max(5,parent.width-5*root.nodeScale)
                                height:width;radius:width/2
                                color:bandNode.bandColor
                                opacity:bandNode.selected?.14:bandNode.hovered?.10:.07
                                antialiasing:true
                            }
                            Rectangle {
                                width:4.4*root.nodeScale;height:2.1*root.nodeScale;radius:height/2
                                x:3.1*root.nodeScale;y:2.7*root.nodeScale
                                color:"#FFFFFF";opacity:bandNode.selected?.42:bandNode.hovered?.30:.20
                                antialiasing:true
                            }
                            Text {
                                anchors.centerIn:parent
                                text:index+1
                                color:bandNode.selected?Theme.text:bandNode.bandColor
                                style:Text.Outline;styleColor:"#B0000000"
                                font.family:Theme.monoFamily
                                font.pixelSize:bandNode.selected?9:8
                                font.weight:Font.Bold
                            }
                        }
                        MouseArea {
                            id:bandMouse
                            anchors.fill:parent
                            hoverEnabled:true
                            cursorShape:Qt.SizeAllCursor
                            preventStealing:true
                            onPressed:function(e){
                                root.selectBand(index)
                                var p=mapToItem(graph,e.x,e.y)
                                bandNode.dragLastX=p.x
                                bandNode.dragLastY=p.y
                            }
                            onPositionChanged:function(e){
                                if(!pressed)return
                                var p=mapToItem(graph,e.x,e.y)
                                var fine=(e.modifiers & Qt.ShiftModifier)!==0
                                var qDrag=(e.modifiers & Qt.ControlModifier)!==0 || (e.modifiers & Qt.MetaModifier)!==0
                                if(qDrag){
                                    var qDy=p.y-bandNode.dragLastY
                                    bandNode.dragLastX=p.x
                                    bandNode.dragLastY=p.y
                                    var factor=Math.exp(-qDy*(fine?0.003:0.012))
                                    root.selectedQ=Math.round(root.clamp(root.safeQ(root.selectedQ)*factor,0.1,30)*100)/100
                                    root.updateSelected()
                                    return
                                }
                                if(fine){
                                    var dx=(p.x-bandNode.dragLastX)*0.25
                                    var dy=(p.y-bandNode.dragLastY)*0.25
                                    root.selectedFreq=root.freqForX(root.xFor(root.selectedFreq)+dx)
                                    root.selectedGain=root.rawGainForY(root.yFor(root.selectedGain)+dy)
                                }else{
                                    root.selectedFreq=root.freqForX(p.x)
                                    root.selectedGain=root.gainForY(p.y)
                                }
                                bandNode.dragLastX=p.x
                                bandNode.dragLastY=p.y
                                root.updateSelected()
                            }
                            onDoubleClicked:{root.selectBand(index);root.setSelectedGain(0)}
                        }
                    }
                }

                BandInspector {
                    visible:root.selectedTarget==="band"
                    bandModel:root.bands;bandIndex:root.selectedIndex;frequency:root.selectedFreq;gain:root.selectedGain;q:root.selectedQ
                    accentColor:root.colorFor(root.selectedIndex)
                    width:Math.min(320,graph.width-30);height:86
                    x:root.clamp(root.xFor(root.selectedFreq)-width/2,15,graph.width-width-15)
                    readonly property bool stickyTop:root.inspectorShouldTop(root.selectedFreq)
                    readonly property real edgeGap:12*root.nodeScale
                    y:stickyTop?root.topPad+edgeGap:root.plotBottom-height-edgeGap
                    Behavior on x{SmoothedAnimation{velocity:1800}}
                    Behavior on y{SmoothedAnimation{velocity:1400}}
                    onFrequencyEdited:function(v){root.setSelectedFrequency(v)}
                    onGainEdited:function(v){root.setSelectedGain(v)}
                    onQEdited:function(v){root.setSelectedQValue(v)}
                    onResetRequested:root.resetSelected()
                }

                CrossoverInspector {
                    visible:root.selectedTarget!=="band"
                    mode:root.selectedTarget
                    frequency:root.inspectorFrequency
                    filterType:root.selectedTarget==="hpf"?String(root.bands.hpType):String(root.bands.lpType)
                    accentColor:Theme.amber
                    width:Math.min(320,graph.width-30);height:86
                    x:root.clamp(root.xFor(root.inspectorFrequency)-width/2,15,graph.width-width-15)
                    readonly property bool stickyTop:root.inspectorShouldTop(root.inspectorFrequency)
                    readonly property real edgeGap:12*root.nodeScale
                    y:stickyTop?root.topPad+edgeGap:root.plotBottom-height-edgeGap
                    Behavior on x{SmoothedAnimation{velocity:1800}}
                    Behavior on y{SmoothedAnimation{velocity:1400}}
                    onFrequencyEdited:function(v){if(root.selectedTarget==="hpf")root.bands.setHpfHz(v);else root.bands.setLpfHz(v)}
                    onTypeEdited:function(v){root.setCrossoverType(root.selectedTarget,v)}
                    onResetRequested:root.resetCrossover(root.selectedTarget)
                }
            }
        }

        RowLayout {
            Layout.fillWidth:true;Layout.preferredHeight:55;Layout.leftMargin:12;Layout.rightMargin:12;Layout.bottomMargin:12;spacing:6
            Repeater {
                model:root.bands
                delegate:Rectangle {
                    id:bandPill
                    required property int index
                    required property real freq
                    required property real gain
                    required property real q
                    required property string typeName
                    readonly property bool selected:root.selectedTarget==="band"&&index===root.selectedIndex
                    readonly property color bandColor:root.colorFor(index)
                    Layout.fillWidth:true;Layout.preferredHeight:43;radius:10
                    gradient:Gradient{GradientStop{position:0;color:bandPill.selected?"#103136":"#11171C"}GradientStop{position:1;color:bandPill.selected?"#081719":"#090D11"}}
                    border.width:1;border.color:bandPill.selected?Theme.accentSoft:"#242C33"
                    Column {
                        anchors.fill:parent;anchors.margins:6;spacing:0
                        Row{width:parent.width;Text{text:"B"+(index+1);color:bandPill.bandColor;font.family:Theme.monoFamily;font.pixelSize:9;font.weight:Font.DemiBold}Item{width:Math.max(0,parent.width-34);height:1}Text{text:root.typeShort(typeName);color:bandPill.selected?Theme.accent:Theme.textSoft;font.family:Theme.monoFamily;font.pixelSize:9;font.weight:Font.Bold}}
                        Row{width:parent.width;Text{text:root.fmtF(freq);color:bandPill.bandColor;font.family:Theme.monoFamily;font.pixelSize:9;font.weight:Font.Bold}Item{width:Math.max(0,parent.width-54);height:1}Text{text:(gain>0?"+":"")+gain.toFixed(1);color:Theme.textSoft;font.family:Theme.monoFamily;font.pixelSize:9}}
                    }
                    MouseArea{anchors.fill:parent;cursorShape:Qt.PointingHandCursor;onClicked:root.selectBand(index)}
                }
            }
        }
    }
}
