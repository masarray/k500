import QtQuick
import QtQuick.Layouts

StudioPanel {
    id: root
    accentTop: false
    implicitHeight: 52

    property bool transportPlaying: false
    property bool transportMuted: false

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        spacing: 8

        RowLayout {
            Layout.preferredWidth: 288
            Layout.minimumWidth: 252
            spacing: 10

            Rectangle {
                Layout.preferredWidth: 38
                Layout.preferredHeight: 38
                radius: 7
                border.width: 1
                border.color: "#0A0E12"
                gradient: Gradient {
                    GradientStop { position:0;color:"#2B343B" }
                    GradientStop { position:.34;color:"#171F25" }
                    GradientStop { position:1;color:"#080C10" }
                }
                Rectangle { anchors.fill:parent;anchors.margins:1;radius:6;color:"transparent";border.width:1;border.color:"#12FFFFFF" }
                Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.top:parent.top;anchors.leftMargin:5;anchors.rightMargin:5;anchors.topMargin:1;height:1;color:"#FFFFFF";opacity:.10 }
                Image { anchors.fill:parent;anchors.margins:4;source:"qrc:/assets/sonkupik-logo.png";fillMode:Image.PreserveAspectFit;smooth:true }
            }

            ColumnLayout {
                spacing: -1
                RowLayout {
                    spacing: 3
                    Text { text:"SONKUPIK";color:Theme.text;font.family:Theme.displayFamily;font.pixelSize:14;font.weight:Font.Bold }
                    Text { text:"STUDIO";color:Theme.amber;font.family:Theme.displayFamily;font.pixelSize:14;font.weight:Font.Bold }
                }
                Text { text:"KARAOKE PROCESSOR";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.letterSpacing:1.3 }
            }
            Item { Layout.fillWidth:true }
        }

        Item { Layout.fillWidth:true }

        Rectangle {
            Layout.preferredWidth: 142
            Layout.preferredHeight: 34
            radius: 9
            color: "#080D11"
            border.width: 1
            border.color: "#26323A"
            Rectangle { anchors.fill:parent;anchors.margins:1;radius:8;color:"transparent";border.width:1;border.color:"#0AFFFFFF" }
            Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.top:parent.top;anchors.leftMargin:6;anchors.rightMargin:6;anchors.topMargin:1;height:1;color:"#FFFFFF";opacity:.07 }
            Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;anchors.leftMargin:6;anchors.rightMargin:6;anchors.bottomMargin:1;height:1;color:"#000000";opacity:.52 }

            RowLayout {
                anchors.fill: parent
                anchors.margins: 4
                spacing: 3
                SoftButton { Layout.preferredWidth:27;Layout.fillHeight:true;transport:true;toolbar:true;iconName:"skip-back";iconOnly:true }
                SoftButton {
                    Layout.preferredWidth:31;Layout.fillHeight:true;transport:true;toolbar:true
                    iconName:root.transportPlaying?"pause":"play";iconOnly:true
                    checked:root.transportPlaying;neonAccent:root.transportPlaying;accentIcon:true
                    onClicked:root.transportPlaying=!root.transportPlaying
                }
                SoftButton { Layout.preferredWidth:27;Layout.fillHeight:true;transport:true;toolbar:true;iconName:"skip-forward";iconOnly:true }
                Rectangle { Layout.preferredWidth:1;Layout.preferredHeight:16;color:"#344049";opacity:.58 }
                SoftButton {
                    Layout.preferredWidth:27;Layout.fillHeight:true;transport:true;toolbar:true;iconName:"volume-x";iconOnly:true
                    checked:root.transportMuted;danger:root.transportMuted
                    onClicked:root.transportMuted=!root.transportMuted
                }
            }
        }

        SoftButton {
            Layout.preferredWidth: 124
            Layout.preferredHeight: 32
            text: "DEFAULT FLAT"
            compact: true
            toolbar: true
            amber: true
            checked: true
        }

        RowLayout {
            spacing: 5
            Rectangle { width:6;height:6;radius:3;color:Theme.textFaint }
            Text { text:"LIVE";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.weight:Font.Bold;font.letterSpacing:.7 }
            SoftButton { Layout.preferredWidth:50;Layout.preferredHeight:29;text:"BT";iconName:"bluetooth";compact:true;toolbar:true;checked:true }
            SoftButton { Layout.preferredWidth:50;Layout.preferredHeight:29;text:"USB";iconName:"usb";compact:true;toolbar:true }
            SoftButton { Layout.preferredWidth:76;Layout.preferredHeight:29;text:"Connect";iconName:"cable";compact:true;toolbar:true }
        }

        SoftButton {
            Layout.preferredWidth: 72
            Layout.preferredHeight: 29
            text: "OFFLINE"
            compact: true
            toolbar: true
            amber: true
            checked: true
        }

        Item { Layout.fillWidth:true }

        RowLayout {
            spacing: 7
            SoftButton { Layout.preferredWidth:80;Layout.preferredHeight:30;text:"Import";iconName:"upload";compact:true;toolbar:true }
            SoftButton { Layout.preferredWidth:80;Layout.preferredHeight:30;text:"Export";iconName:"download";compact:true;toolbar:true;checked:true }
        }
    }
}
