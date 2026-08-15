import QtQuick

Item {
    id: root

    property string title: "GAIN"
    property real value: 0.0
    property real from: -12.0
    property real to: 12.0
    property real defaultValue: 0.0
    property int decimals: 1
    property real step: 0
    property string unit: "dB"
    property string valuePrefix: ""
    property bool logarithmic: false
    property bool compact: false
    property color accentColor: Theme.accent
    signal valueEdited(real newValue)

    implicitWidth: compact ? 70 : 84
    implicitHeight: compact ? 76 : 94

    property real previewValue: value
    property bool dragging: false
    property bool hovered: pointer.containsMouse
    property real pressY: 0
    property real pressNorm: 0

    function clamp(v,a,b){ return Math.max(a,Math.min(b,v)) }
    function valueToNorm(v){
        if(logarithmic){var safeFrom=Math.max(.0001,from),safeValue=Math.max(safeFrom,v);return clamp(Math.log(safeValue/safeFrom)/Math.log(to/safeFrom),0,1)}
        return clamp((v-from)/(to-from),0,1)
    }
    function normToValue(n){n=clamp(n,0,1);if(logarithmic){var safeFrom=Math.max(.0001,from);return safeFrom*Math.pow(to/safeFrom,n)}return from+n*(to-from)}
    function formatValue(v){if(logarithmic&&unit==="Hz"&&v>=1000)return(v/1000).toFixed(v>=10000?1:2)+"k";return Number(v).toFixed(decimals)}
    function effectiveStep(fine){var base=step>0?step:Math.max((to-from)/100,Math.pow(10,-decimals));return fine?base/10:base}
    function quantize(v,fine){var s=effectiveStep(fine),next=clamp(Math.round(v/s)*s,from,to);return Number(next.toFixed(Math.max(decimals+1,3)))}
    function nudge(direction,fine){var next=quantize(value+direction*effectiveStep(fine),fine);previewValue=next;valueEdited(next)}

    activeFocusOnTab: true
    Keys.onPressed:function(event){
        if(event.key===Qt.Key_Up||event.key===Qt.Key_Right){nudge(1,(event.modifiers&Qt.ShiftModifier)!==0);event.accepted=true}
        else if(event.key===Qt.Key_Down||event.key===Qt.Key_Left){nudge(-1,(event.modifiers&Qt.ShiftModifier)!==0);event.accepted=true}
        else if(event.key===Qt.Key_Home){previewValue=defaultValue;valueEdited(defaultValue);event.accepted=true}
    }

    onValueChanged: if(!dragging)previewValue=value
    onPreviewValueChanged: dial.requestPaint()
    onAccentColorChanged: dial.requestPaint()

    Text {
        id:titleLabel
        anchors.top:parent.top
        anchors.horizontalCenter:parent.horizontalCenter
        text:root.title
        color:Theme.textDim
        font.family:Theme.monoFamily
        font.pixelSize:9
        font.weight:Font.Medium
        font.letterSpacing:.75
    }

    Item {
        id:knobBox
        width:root.compact?50:60
        height:width
        anchors.top:titleLabel.bottom
        anchors.topMargin:root.compact?3:5
        anchors.horizontalCenter:parent.horizontalCenter

        Canvas {
            id:dial
            anchors.fill:parent
            antialiasing:true
            onPaint:{
                var ctx=getContext("2d");ctx.reset()
                var cx=width/2,cy=height*.62,norm=root.valueToNorm(root.previewValue)
                var start=Math.PI*.75,sweep=Math.PI*1.5,end=start+sweep,activeEnd=start+sweep*norm
                var arcR=width*.36

                // Web knob = quiet dark arc, cyan active arc, metal face and one amber pointer.
                ctx.lineCap="round"
                ctx.lineWidth=root.compact?2.0:2.4
                ctx.strokeStyle="#050607"
                ctx.beginPath();ctx.arc(cx,cy,arcR,start,end,false);ctx.stroke()

                ctx.strokeStyle=root.accentColor.toString()
                ctx.beginPath();ctx.arc(cx,cy,arcR,start,activeEnd,false);ctx.stroke()

                var capR=width*.245
                var g=ctx.createRadialGradient(cx-capR*.30,cy-capR*.42,1,cx,cy,capR)
                g.addColorStop(0,"#555E66");g.addColorStop(.28,"#30383F");g.addColorStop(.72,"#181E23");g.addColorStop(1,"#0C1014")
                ctx.fillStyle=g;ctx.beginPath();ctx.arc(cx,cy,capR,0,Math.PI*2);ctx.fill()
                ctx.strokeStyle="#050607";ctx.lineWidth=1;ctx.stroke()

                var a2=activeEnd
                ctx.strokeStyle=Theme.amber.toString()
                ctx.lineWidth=root.compact?1.8:2.0
                ctx.beginPath()
                ctx.moveTo(cx+Math.cos(a2)*capR*.42,cy+Math.sin(a2)*capR*.42)
                ctx.lineTo(cx+Math.cos(a2)*capR*.88,cy+Math.sin(a2)*capR*.88)
                ctx.stroke()
            }
        }

        MouseArea {
            id:pointer
            anchors.fill:parent
            hoverEnabled:true
            cursorShape:Qt.SizeVerCursor
            onPressed:function(e){root.forceActiveFocus();root.dragging=true;root.pressY=e.y;root.pressNorm=root.valueToNorm(root.value);root.previewValue=root.value}
            onPositionChanged:function(e){if(!pressed)return;var fine=(e.modifiers&Qt.ShiftModifier)!==0,sensitivity=fine?420:145,nextNorm=root.clamp(root.pressNorm+(root.pressY-e.y)/sensitivity,0,1);root.previewValue=root.quantize(root.normToValue(nextNorm),fine);root.valueEdited(root.previewValue)}
            onReleased:root.dragging=false
            onCanceled:root.dragging=false
            onDoubleClicked:{root.previewValue=root.defaultValue;root.valueEdited(root.defaultValue)}
            onWheel:function(e){root.forceActiveFocus();root.nudge(e.angleDelta.y>0?1:-1,(e.modifiers&Qt.ShiftModifier)!==0);e.accepted=true}
        }
    }

    Rectangle {
        anchors.top:knobBox.bottom
        anchors.topMargin:root.compact?-2:0
        anchors.horizontalCenter:parent.horizontalCenter
        width:root.compact?64:76
        height:root.compact?18:20
        radius:7
        color:"#05080A"
        border.width:1
        border.color:root.dragging?root.accentColor:root.activeFocus?Theme.focus:"#020304"

        Text {
            anchors.centerIn:parent
            text:root.valuePrefix+root.formatValue(root.previewValue)+(root.unit.length?" "+root.unit:"")
            color:Theme.amber
            font.family:Theme.monoFamily
            font.pixelSize:root.compact?9:10
            font.weight:Font.Bold
        }
    }
}
