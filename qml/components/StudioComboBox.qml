import QtQuick
import QtQuick.Controls

ComboBox {
    id: control

    property string value: ""
    property color accentColor: Theme.amber
    signal valueEdited(string newValue)

    implicitHeight: 34
    leftPadding: 8
    rightPadding: 22
    topPadding: 0
    bottomPadding: 0
    currentIndex: Math.max(0, control.model.indexOf(control.value))
    hoverEnabled: true

    onActivated: function(index) {
        control.value = control.textAt(index)
        control.valueEdited(control.value)
    }

    contentItem: Text {
        text: control.displayText
        color: control.enabled ? control.accentColor : Theme.textFaint
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        font.family: Theme.monoFamily
        font.pixelSize: 8
        font.weight: Font.Bold
        font.letterSpacing: 0
    }

    indicator: Item {
        x: control.width - width - 6
        y: (control.height - height) / 2
        width: 12
        height: 12
        LucideIcon {
            anchors.fill: parent
            name: "chevron-down"
            color: control.popup.visible || control.hovered ? control.accentColor : Theme.textDim
            strokeWidth: 1.8
            rotation: control.popup.visible ? 180 : 0
            Behavior on rotation { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
            Behavior on color { ColorAnimation { duration: 100 } }
        }
    }

    background: Rectangle {
        radius: 6
        border.width: 1
        border.color: control.activeFocus || control.popup.visible ? control.accentColor : control.hovered ? "#6B5B24" : Theme.borderSoft
        gradient: Gradient {
            GradientStop { position: 0.0; color: control.down ? "#0E100C" : control.hovered ? "#1D1D15" : "#111518" }
            GradientStop { position: 0.48; color: control.down ? "#080A0B" : "#0B0F12" }
            GradientStop { position: 1.0; color: "#050709" }
        }
        Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.top:parent.top;anchors.leftMargin:5;anchors.rightMargin:5;height:1;color:control.accentColor;opacity:control.activeFocus||control.popup.visible?.40:.07 }
        Rectangle { anchors.top:parent.top;anchors.bottom:parent.bottom;anchors.right:parent.right;anchors.rightMargin:19;width:1;color:Theme.borderSoft }
    }

    delegate: ItemDelegate {
        id: option
        required property var modelData
        required property int index
        width: control.width - 8
        height: 30
        leftPadding: 7
        rightPadding: 6
        highlighted: control.highlightedIndex === index
        contentItem: Text {
            text: option.modelData
            color: option.highlighted ? control.accentColor : Theme.textSoft
            verticalAlignment: Text.AlignVCenter
            font.family: Theme.monoFamily
            font.pixelSize: 8
            font.weight: option.highlighted ? Font.Bold : Font.Medium
        }
        background: Rectangle { radius:5;color:option.highlighted?Theme.amberFaint:"transparent";border.width:option.highlighted?1:0;border.color:Theme.amberSoft }
    }

    popup: Popup {
        y: control.height + 4
        width: Math.max(control.width, 112)
        implicitHeight: Math.min(contentItem.implicitHeight + 8, 224)
        padding: 4
        z: 50
        contentItem: ListView { clip:true;implicitHeight:contentHeight;model:control.popup.visible?control.delegateModel:null;currentIndex:control.highlightedIndex;spacing:2 }
        background: Rectangle { radius:8;color:"#0D1318";border.width:1;border.color:"#36424B" }
    }
}
