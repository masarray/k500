import QtQuick
import QtQuick.Layouts

Item {
    id: root

    property real value: -6.0
    property real from: -60.0
    property real to: 10.0
    property real defaultValue: 0.0
    property color accentColor: Theme.accent
    // Kept for source compatibility with older panel bindings. Visual ownership
    // is intentionally focus-based so two independent panels can never leave
    // two faders persistently highlighted at the same time.
    property bool selected: false
    property real step: 0.5
    signal valueEdited(real newValue)
    signal activated()

    implicitWidth: 48
    implicitHeight: 160
    // One physical fader metric for the entire product. These attached
    // layout constraints also cover direct StudioFader users.
    Layout.preferredHeight: 160
    Layout.minimumHeight: 160
    Layout.maximumHeight: 160

    // FADER_INTERACTION_PARITY_V1
    // FADER_SINGLE_ACTIVE_FOCUS_V1
    // Strong highlight is owned by Qt's one active focus item, not by panel-
    // local selected flags. Hover remains a quiet preview only. This gives the
    // whole section/window one unambiguous active control like a professional
    // console/VST surface while preserving keyboard and drag interaction.
    property real previewValue: value
    property real dragRawValue: value
    property real dragLastY: 0
    property bool dragging: false
    property bool hovered: pointer.containsMouse
    readonly property bool interactionActive: dragging || activeFocus
    readonly property bool highlighted: interactionActive
    readonly property bool hoverPreview: hovered && !interactionActive

    function clamp(v,a,b){ return Math.max(a,Math.min(b,v)) }
    function valueToNorm(v){ return clamp((v-from)/(to-from),0,1) }
    function normToValue(n){ return from+clamp(n,0,1)*(to-from) }
    function quantize(v,fine){
        var s=fine?step/5:step
        if(s<=0)return clamp(v,from,to)
        return clamp(Math.round(v/s)*s,from,to)
    }
    function activate(){
        forceActiveFocus()
        activated()
    }
    function commitPreview(next){
        next=clamp(next,from,to)
        if(Math.abs(next-previewValue)<0.000001)return
        previewValue=next
        valueEdited(next)
    }
    function nudge(direction,fine){
        var amount=fine?step/5:step
        commitPreview(quantize(previewValue+direction*amount,fine))
        dragRawValue=previewValue
    }
    function dragTo(yPos,fine){
        var travel=Math.max(32,root.height-16-cap.height)
        var dy=yPos-dragLastY
        dragLastY=yPos
        // Relative drag avoids the abrupt jump of absolute track mapping and
        // stays controllable even when the pointer starts away from the cap.
        var sensitivity=fine?0.20:1.0
        dragRawValue=clamp(dragRawValue-(dy/travel)*(to-from)*sensitivity,from,to)
        commitPreview(quantize(dragRawValue,fine))
    }

    activeFocusOnTab: true
    Keys.onPressed: function(event){
        if(event.key===Qt.Key_Up||event.key===Qt.Key_Right){activate();nudge(1,(event.modifiers&Qt.ShiftModifier)!==0);event.accepted=true}
        else if(event.key===Qt.Key_Down||event.key===Qt.Key_Left){activate();nudge(-1,(event.modifiers&Qt.ShiftModifier)!==0);event.accepted=true}
        else if(event.key===Qt.Key_Home){activate();previewValue=defaultValue;dragRawValue=defaultValue;valueEdited(defaultValue);event.accepted=true}
    }
    onValueChanged: if(!dragging){previewValue=value;dragRawValue=value}

    // The active control gets the premium cyan focus treatment. Hover only
    // lifts the surface slightly, so it can never be confused with selection.
    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: 10
        color: root.interactionActive ? root.accentColor : "#91A3AB"
        opacity: root.dragging ? 0.13 : root.interactionActive ? 0.082 : root.hoverPreview ? 0.018 : 0
        border.width: root.interactionActive ? 1 : 0
        border.color: root.accentColor
        Behavior on opacity { NumberAnimation { duration: 75 } }
    }

    Rectangle {
        id: track
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: root.interactionActive ? 9 : 8
        anchors.topMargin: 7
        anchors.bottomMargin: 7
        radius: width/2
        gradient: Gradient {
            GradientStop { position:0;color:"#050607" }
            GradientStop { position:.55;color:"#010203" }
            GradientStop { position:1;color:"#080B0E" }
        }
        border.width: 1
        border.color: root.interactionActive ? root.accentColor : root.hoverPreview ? "#294047" : "#010203"
        opacity: root.hoverPreview ? 0.96 : 1
        Behavior on width { NumberAnimation { duration: 70 } }
        Behavior on border.color { ColorAnimation { duration: 75 } }
        Rectangle { anchors.horizontalCenter:parent.horizontalCenter;anchors.top:parent.top;anchors.bottom:parent.bottom;anchors.margins:2;width:2;radius:1;color:"#000000" }
    }

    // Seven quiet ticks per side keeps every fader family on the same ruler.
    Repeater {
        model: 7
        delegate: Item {
            required property int index
            width: root.width
            height: 1
            y: 5 + index * (root.height - 10) / 6
            Rectangle { width:8;height:1;x:1;color:root.interactionActive?root.accentColor:"#89949C";opacity:root.interactionActive?.36:.28 }
            Rectangle { width:8;height:1;anchors.right:parent.right;anchors.rightMargin:1;color:root.interactionActive?root.accentColor:"#89949C";opacity:root.interactionActive?.36:.28 }
        }
    }

    Rectangle {
        id: capShadow
        width: 29
        height: 21
        radius: 10
        anchors.horizontalCenter: parent.horizontalCenter
        y: cap.y+2
        color: "#000000"
        opacity: root.dragging ? .66 : .52
        Behavior on opacity { NumberAnimation { duration: 75 } }
    }

    Rectangle {
        id: cap
        width: root.dragging ? 27 : root.interactionActive ? 26 : root.hoverPreview ? 25 : 24
        height: root.dragging ? 18 : root.interactionActive ? 16 : 16
        radius: height/2
        anchors.horizontalCenter: parent.horizontalCenter
        y: 8+(1-root.valueToNorm(root.previewValue))*(root.height-height-16)
        border.width: root.interactionActive ? 1.5 : 1
        border.color: root.interactionActive ? root.accentColor : root.hoverPreview ? "#42555D" : "#090C0F"
        gradient: Gradient {
            GradientStop { position:0;color:"#6C767E" }
            GradientStop { position:.22;color:"#48525A" }
            GradientStop { position:.60;color:"#293138" }
            GradientStop { position:1;color:"#151B20" }
        }
        // Never animate behind the pointer while dragging. External/device
        // updates may still glide smoothly when the user is not touching it.
        Behavior on y { enabled: !root.dragging; SmoothedAnimation { velocity:1700 } }
        Behavior on width { NumberAnimation { duration:65 } }
        Behavior on height { NumberAnimation { duration:65 } }
        Behavior on border.color { ColorAnimation { duration:70 } }

        Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.leftMargin:4;anchors.rightMargin:4;anchors.top:parent.top;anchors.topMargin:2;height:1;color:"#FFFFFF";opacity:.20 }
        Rectangle {
            anchors.left:parent.left;anchors.right:parent.right;anchors.leftMargin:4;anchors.rightMargin:4
            anchors.verticalCenter:parent.verticalCenter;height:1;color:root.interactionActive?root.accentColor:root.hoverPreview?"#C5CDD1":"#AAB4BA";opacity:root.interactionActive?.96:root.hoverPreview?.58:.45
        }
    }

    MouseArea {
        id: pointer
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        preventStealing: true
        cursorShape: Qt.SizeVerCursor

        onPressed:function(e){
            root.activate()
            root.dragging=true
            root.dragLastY=e.y
            root.dragRawValue=root.previewValue
            e.accepted=true
        }
        onPositionChanged:function(e){
            if(!pressed)return
            root.dragTo(e.y,(e.modifiers&Qt.ShiftModifier)!==0)
        }
        onReleased:function(e){
            root.dragging=false
            root.dragRawValue=root.previewValue
            e.accepted=true
        }
        onCanceled:{root.dragging=false;root.dragRawValue=root.previewValue}
        onDoubleClicked:function(e){
            root.activate()
            root.previewValue=root.defaultValue
            root.dragRawValue=root.defaultValue
            root.valueEdited(root.defaultValue)
            e.accepted=true
        }
        onWheel:function(e){
            root.activate()
            var delta=e.angleDelta.y
            if(delta===0)delta=e.pixelDelta.y
            if(delta===0){e.accepted=false;return}
            root.nudge(delta>0?1:-1,(e.modifiers&Qt.ShiftModifier)!==0)
            e.accepted=true
        }
    }
}
