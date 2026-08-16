import QtQuick

Item {
    id: root

    property string label: "PARAMETER"
    property real value: 0
    property real from: -12
    property real to: 12
    property real step: 0.1
    property real defaultValue: 0
    property int decimals: 1
    property string unit: "dB"
    property color accentColor: Theme.accent
    property bool logarithmic: false
    property bool dragging: false
    readonly property bool hovered: pointer.containsMouse
    readonly property bool highlighted: hovered || dragging || activeFocus
    signal valueEdited(real newValue)

    implicitHeight: 32
    implicitWidth: 250
    activeFocusOnTab: true

    function clamp(v){return Math.max(from,Math.min(to,v))}
    function valueToNorm(v){if(logarithmic)return Math.log(Math.max(from,v)/from)/Math.log(to/from);return(v-from)/(to-from)}
    function normToValue(n){n=Math.max(0,Math.min(1,n));return logarithmic?from*Math.pow(to/from,n):from+n*(to-from)}
    function quantize(v,fine){var s=fine?step/10:step;return Number(clamp(Math.round(v/s)*s).toFixed(Math.max(decimals+1,3)))}
    function nudge(direction,fine){valueEdited(quantize(value+direction*(fine?step/10:step),fine))}
    function display(v){if(unit==="Hz"&&v>=1000)return(v/1000).toFixed(v>=10000?1:2)+"k";return Number(v).toFixed(decimals)}

    Keys.onPressed:function(event){
        if(event.key===Qt.Key_Up||event.key===Qt.Key_Right){nudge(1,(event.modifiers&Qt.ShiftModifier)!==0);event.accepted=true}
        else if(event.key===Qt.Key_Down||event.key===Qt.Key_Left){nudge(-1,(event.modifiers&Qt.ShiftModifier)!==0);event.accepted=true}
        else if(event.key===Qt.Key_Home){valueEdited(defaultValue);event.accepted=true}
    }

    // CONTROL_CAPTION_AWARENESS_V1
    Text {
        id:caption
        anchors.left:parent.left
        anchors.verticalCenter:parent.verticalCenter
        width:76
        text:root.label
        color:root.highlighted ? root.accentColor : Theme.textDim
        style:root.highlighted ? Text.Outline : Text.Normal
        styleColor:root.highlighted ? Qt.rgba(root.accentColor.r,root.accentColor.g,root.accentColor.b,.34) : "transparent"
        font.family:Theme.monoFamily
        font.pixelSize:8
        font.weight:root.highlighted ? Font.DemiBold : Font.Medium
        font.letterSpacing:.8
        elide:Text.ElideRight
        Behavior on color { ColorAnimation { duration:75 } }
        Behavior on styleColor { ColorAnimation { duration:75 } }
    }

    Rectangle {
        id:track
        anchors.left:parent.left
        anchors.leftMargin:82
        anchors.right:readout.left
        anchors.rightMargin:8
        anchors.verticalCenter:parent.verticalCenter
        height:root.highlighted ? 8 : 7
        radius:4
        color:"#040608"
        border.width:1
        border.color:root.highlighted ? root.accentColor : "#11191F"
        Behavior on height { NumberAnimation { duration:70 } }
        Behavior on border.color { ColorAnimation { duration:75 } }

        Rectangle { anchors.left:parent.left;anchors.verticalCenter:parent.verticalCenter;anchors.leftMargin:2;width:Math.max(2,(parent.width-4)*root.valueToNorm(root.value));height:3;radius:2;color:root.accentColor;opacity:.88 }
        Rectangle { x:2+(parent.width-12-4)*root.valueToNorm(root.value)-5;anchors.verticalCenter:parent.verticalCenter;width:22;height:18;radius:9;color:root.accentColor;opacity:root.highlighted?.16:.06;z:-1;Behavior on opacity{NumberAnimation{duration:70}} }
        Rectangle {
            x:2+(parent.width-width-4)*root.valueToNorm(root.value)
            anchors.verticalCenter:parent.verticalCenter
            width:11;height:10;radius:5
            color:root.accentColor
            border.width:1
            border.color:root.highlighted?"#C7FFFB":"#6DE8DF"
            Rectangle{anchors.left:parent.left;anchors.right:parent.right;anchors.top:parent.top;anchors.leftMargin:2;anchors.rightMargin:2;height:1;color:"#FFFFFF";opacity:.45}
        }
    }

    Rectangle {
        id:readout
        anchors.right:parent.right
        anchors.verticalCenter:parent.verticalCenter
        width:72;height:25;radius:7
        color:"#06090C";border.width:1;border.color:root.highlighted?root.accentColor:"#1B242B"
        Behavior on border.color { ColorAnimation { duration:75 } }
        Row {
            anchors.centerIn:parent
            spacing:3
            Text{text:root.display(root.value);color:Theme.amber;font.family:Theme.monoFamily;font.pixelSize:9;font.weight:Font.Bold}
            Text{text:root.unit;color:root.highlighted?Theme.textSoft:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:7;anchors.baseline:parent.children[0].baseline;Behavior on color{ColorAnimation{duration:75}}}
        }
    }

    MouseArea {
        id:pointer
        anchors.left:track.left;anchors.right:readout.right;anchors.top:parent.top;anchors.bottom:parent.bottom
        hoverEnabled:true
        preventStealing:true
        cursorShape:Qt.PointingHandCursor
        function setFromX(sceneX){var p=mapToItem(track,sceneX,0);root.valueEdited(root.quantize(root.normToValue(p.x/track.width),false))}
        onPressed:function(event){root.forceActiveFocus();root.dragging=true;setFromX(event.x);event.accepted=true}
        onPositionChanged:function(event){if(pressed)setFromX(event.x)}
        onReleased:function(event){root.dragging=false;event.accepted=true}
        onCanceled:root.dragging=false
        onDoubleClicked:function(event){root.forceActiveFocus();root.valueEdited(root.defaultValue);event.accepted=true}
        onWheel:function(event){root.forceActiveFocus();root.nudge(event.angleDelta.y>0?1:-1,(event.modifiers&Qt.ShiftModifier)!==0);event.accepted=true}
    }
}
