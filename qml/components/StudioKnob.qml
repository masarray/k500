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
    property bool premium: false
    // FX_DETAIL_READ_ONLY_TRUTH_V1
    // Premium knobs are currently used by Reverb/Echo detail fields. The K500
    // exposes their readback bytes, but the repository explicitly has no
    // donor-verified live write command for those details. Default them to
    // read-only rather than allowing a control to move while hardware does not.
    // Ordinary/compressor knobs remain editable; a future verified effect path
    // can opt in explicitly with editable:true.
    property bool editable: !premium
    property color accentColor: Theme.accent
    signal valueEdited(real newValue)

    // PREMIUM_FX_KNOB_V1
    // FX racks can opt into a slightly larger, instrument-like dial with a
    // luminous value arc and restrained tick halo. It remains the same control
    // rendering as the canonical StudioKnob while unsupported writes fail closed.
    implicitWidth: premium ? 92 : (compact ? 72 : 80)
    implicitHeight: premium ? 118 : (compact ? 102 : 110)

    property real previewValue: value
    property bool dragging: false
    property bool hovered: root.editable && pointer.containsMouse
    property real pressY: 0
    property real pressNorm: 0
    readonly property bool highlighted: root.editable && (hovered || dragging || activeFocus)

    function clamp(v,a,b){ return Math.max(a,Math.min(b,v)) }
    function valueToNorm(v){
        if(logarithmic){var safeFrom=Math.max(.0001,from),safeValue=Math.max(safeFrom,v);return clamp(Math.log(safeValue/safeFrom)/Math.log(to/safeFrom),0,1)}
        return clamp((v-from)/(to-from),0,1)
    }
    function normToValue(n){n=clamp(n,0,1);if(logarithmic){var safeFrom=Math.max(.0001,from);return safeFrom*Math.pow(to/safeFrom,n)}return from+n*(to-from)}
    function formatValue(v){if(logarithmic&&unit==="Hz"&&v>=1000)return(v/1000).toFixed(v>=10000?1:2)+"k";return Number(v).toFixed(decimals)}
    function effectiveStep(fine){var base=step>0?step:Math.max((to-from)/100,Math.pow(10,-decimals));return fine?base/10:base}
    function quantize(v,fine){var s=effectiveStep(fine),next=clamp(Math.round(v/s)*s,from,to);return Number(next.toFixed(Math.max(decimals+1,3)))}
    function nudge(direction,fine){
        if(!root.editable)return
        var next=quantize(value+direction*effectiveStep(fine),fine)
        previewValue=next
        valueEdited(next)
    }

    activeFocusOnTab: root.editable
    Keys.onPressed:function(event){
        if(!root.editable)return
        if(event.key===Qt.Key_Up||event.key===Qt.Key_Right){nudge(1,(event.modifiers&Qt.ShiftModifier)!==0);event.accepted=true}
        else if(event.key===Qt.Key_Down||event.key===Qt.Key_Left){nudge(-1,(event.modifiers&Qt.ShiftModifier)!==0);event.accepted=true}
        else if(event.key===Qt.Key_Home){previewValue=defaultValue;valueEdited(defaultValue);event.accepted=true}
    }

    onValueChanged: if(!dragging)previewValue=value
    onPreviewValueChanged: dial.requestPaint()
    onAccentColorChanged: dial.requestPaint()
    onPremiumChanged: dial.requestPaint()
    onEditableChanged: { if(!editable) dragging=false; dial.requestPaint() }

    // CONTROL_CAPTION_AWARENESS_V1: captions follow the control under the pointer.
    Text {
        id:titleLabel
        anchors.top:parent.top
        anchors.horizontalCenter:parent.horizontalCenter
        text:root.title
        color:root.highlighted ? root.accentColor : Theme.textDim
        style:root.highlighted ? Text.Outline : Text.Normal
        styleColor:root.highlighted ? Qt.rgba(root.accentColor.r,root.accentColor.g,root.accentColor.b,.34) : "transparent"
        font.family:Theme.monoFamily
        font.pixelSize:9
        font.weight:root.highlighted ? Font.DemiBold : Font.Medium
        font.letterSpacing:root.premium ? .95 : .75
        Behavior on color { ColorAnimation { duration:75 } }
        Behavior on styleColor { ColorAnimation { duration:75 } }
    }

    Item {
        id:knobBox
        width:root.premium ? 74 : 64
        height:width
        anchors.top:titleLabel.bottom
        anchors.topMargin:root.premium ? 4 : 3
        anchors.horizontalCenter:parent.horizontalCenter

        Canvas {
            id:dial
            anchors.fill:parent
            antialiasing:true
            onPaint:{
                var ctx=getContext("2d");ctx.reset()
                var cx=width/2,cy=height*.50,norm=root.valueToNorm(root.previewValue)
                var start=Math.PI*.75,sweep=Math.PI*1.5,end=start+sweep,activeEnd=start+sweep*norm
                var arcR=width*(root.premium?.35:.36)

                if(root.premium){
                    ctx.lineCap="round"
                    ctx.strokeStyle="#85939C"
                    ctx.lineWidth=1
                    for(var ti=0;ti<=16;++ti){
                        var ta=start+sweep*ti/16
                        var tr0=arcR+7,tr1=arcR+(ti%4===0?11:9)
                        ctx.globalAlpha=ti%4===0?.28:.15
                        ctx.beginPath()
                        ctx.moveTo(cx+Math.cos(ta)*tr0,cy+Math.sin(ta)*tr0)
                        ctx.lineTo(cx+Math.cos(ta)*tr1,cy+Math.sin(ta)*tr1)
                        ctx.stroke()
                    }
                }

                ctx.lineCap="round"
                ctx.globalAlpha=1
                ctx.lineWidth=root.premium?3.2:2.6
                ctx.strokeStyle="#020304"
                ctx.beginPath();ctx.arc(cx,cy,arcR,start,end,false);ctx.stroke()

                ctx.globalAlpha=root.premium?.18:.16
                ctx.lineWidth=root.premium?7.2:5.1
                ctx.strokeStyle=root.accentColor.toString()
                ctx.beginPath();ctx.arc(cx,cy,arcR,start,activeEnd,false);ctx.stroke()

                ctx.globalAlpha=root.editable ? 1 : .72
                ctx.lineWidth=root.premium?3.1:2.6
                ctx.strokeStyle=root.accentColor.toString()
                ctx.beginPath();ctx.arc(cx,cy,arcR,start,activeEnd,false);ctx.stroke()

                var capR=width*(root.premium?.255:.245)
                var g=ctx.createRadialGradient(cx-capR*.30,cy-capR*.42,1,cx,cy,capR)
                g.addColorStop(0,root.premium?"#68737B":"#555E66")
                g.addColorStop(.25,root.premium?"#39434B":"#30383F")
                g.addColorStop(.72,"#181E23")
                g.addColorStop(1,"#0A0E12")
                ctx.globalAlpha=1
                ctx.fillStyle=g;ctx.beginPath();ctx.arc(cx,cy,capR,0,Math.PI*2);ctx.fill()
                ctx.strokeStyle=root.highlighted?root.accentColor.toString():"#020304"
                ctx.globalAlpha=root.highlighted?.72:1
                ctx.lineWidth=root.premium?1.3:1
                ctx.stroke()

                if(root.premium){
                    ctx.globalAlpha=.20
                    ctx.strokeStyle="#FFFFFF"
                    ctx.lineWidth=1
                    ctx.beginPath();ctx.arc(cx-capR*.08,cy-capR*.10,capR*.72,Math.PI*1.05,Math.PI*1.63,false);ctx.stroke()
                }

                var a2=activeEnd
                ctx.globalAlpha=root.editable ? 1 : .78
                ctx.strokeStyle=Theme.amber.toString()
                ctx.lineWidth=root.premium?2.25:1.9
                ctx.beginPath()
                ctx.moveTo(cx+Math.cos(a2)*capR*.38,cy+Math.sin(a2)*capR*.38)
                ctx.lineTo(cx+Math.cos(a2)*capR*.90,cy+Math.sin(a2)*capR*.90)
                ctx.stroke()
            }
        }

        MouseArea {
            id:pointer
            anchors.fill:parent
            enabled:root.editable
            hoverEnabled:root.editable
            cursorShape:root.editable ? Qt.SizeVerCursor : Qt.ArrowCursor
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
        anchors.topMargin:root.premium ? 2 : 1
        anchors.horizontalCenter:parent.horizontalCenter
        width:root.premium ? 72 : 64
        height:root.premium ? 22 : 20
        radius:7
        color:root.premium ? "#04080B" : "#05080A"
        border.width:1
        border.color:root.editable && (root.dragging||root.hovered) ? root.accentColor
                     : root.editable && root.activeFocus ? Theme.focus : "#020304"
        Behavior on border.color { ColorAnimation { duration:75 } }

        Text {
            anchors.centerIn:parent
            text:root.valuePrefix+root.formatValue(root.previewValue)+(root.unit.length?" "+root.unit:"")
            color:Theme.amber
            opacity:root.editable ? 1 : .90
            font.family:Theme.monoFamily
            font.pixelSize:9
            font.weight:Font.Bold
        }
    }
}
