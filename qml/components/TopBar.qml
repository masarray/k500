import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

StudioPanel {
    id: root
    accentTop: false
    implicitHeight: 52

    required property var deviceManager
    property bool transportPlaying: false
    readonly property bool deviceBusy: deviceManager.status === "connecting" || deviceManager.status === "syncing"
    readonly property string deviceStatusText: deviceManager.status === "connected" ? "ONLINE"
                                                : deviceManager.status === "connecting" ? "CONNECT"
                                                : deviceManager.status === "syncing" ? "SYNC"
                                                : deviceManager.status === "error" ? "ERROR"
                                                : "OFFLINE"

    Connections {
        target: root.deviceManager
        function onStatusChanged() {
            if (!root.deviceManager.connected)
                root.transportPlaying = false
        }
    }

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
                SoftButton {
                    Layout.preferredWidth:27;Layout.fillHeight:true;transport:true;toolbar:true;iconName:"skip-back";iconOnly:true
                    enabled: root.deviceManager.connected
                    onClicked: root.deviceManager.sendPlayerCommand("rewind")
                }
                SoftButton {
                    Layout.preferredWidth:31;Layout.fillHeight:true;transport:true;toolbar:true
                    iconName:root.transportPlaying?"pause":"play";iconOnly:true
                    checked:root.transportPlaying;neonAccent:root.transportPlaying;accentIcon:true
                    enabled: root.deviceManager.connected
                    onClicked: {
                        root.deviceManager.sendPlayerCommand("playPause")
                        root.transportPlaying = !root.transportPlaying
                    }
                }
                SoftButton {
                    Layout.preferredWidth:27;Layout.fillHeight:true;transport:true;toolbar:true;iconName:"skip-forward";iconOnly:true
                    enabled: root.deviceManager.connected
                    onClicked: root.deviceManager.sendPlayerCommand("forward")
                }
                Rectangle { Layout.preferredWidth:1;Layout.preferredHeight:16;color:"#344049";opacity:.58 }
                SoftButton {
                    Layout.preferredWidth:27;Layout.fillHeight:true;transport:true;toolbar:true;iconName:"volume-x";iconOnly:true
                    checked:root.deviceManager.muted;danger:root.deviceManager.muted
                    enabled: root.deviceManager.connected
                    onClicked: root.deviceManager.toggleMute()
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
            Rectangle {
                width:6;height:6;radius:3
                color: root.deviceManager.liveEnabled ? Theme.accent
                     : root.deviceBusy ? Theme.amber
                     : root.deviceManager.status === "error" ? "#FF6868"
                     : Theme.textFaint
            }
            Text {
                text:"LIVE"
                color:root.deviceManager.liveEnabled ? Theme.accent : Theme.textDim
                font.family:Theme.monoFamily;font.pixelSize:8;font.weight:Font.Bold;font.letterSpacing:.7
            }
            SoftButton {
                Layout.preferredWidth:50;Layout.preferredHeight:29;text:"BT";iconName:"bluetooth";compact:true;toolbar:true
                checked:root.deviceManager.transportMode === "bt"
                enabled:!root.deviceBusy
                onClicked:root.deviceManager.setTransportMode("bt")
            }
            SoftButton {
                Layout.preferredWidth:50;Layout.preferredHeight:29;text:"USB";iconName:"usb";compact:true;toolbar:true
                checked:root.deviceManager.transportMode === "usb"
                enabled:!root.deviceBusy
                onClicked:root.deviceManager.setTransportMode("usb")
            }
            SoftButton {
                Layout.preferredWidth:86;Layout.preferredHeight:29
                text:root.deviceManager.connected || root.deviceBusy ? "Disconnect" : "Connect"
                iconName:root.deviceManager.connected ? "unplug" : "cable"
                compact:true;toolbar:true
                checked:root.deviceManager.connected
                neonAccent:root.deviceManager.connected
                onClicked:root.deviceManager.toggleConnection()
            }
        }

        SoftButton {
            id: statusButton
            Layout.preferredWidth: 72
            Layout.preferredHeight: 29
            text: root.deviceStatusText
            compact: true
            toolbar: true
            amber: !root.deviceManager.connected && root.deviceManager.status !== "error"
            danger: root.deviceManager.status === "error"
            checked: root.deviceManager.connected
            neonAccent: root.deviceManager.connected
            ToolTip.visible: statusHover.containsMouse && (root.deviceManager.lastError.length > 0 || root.deviceManager.portLabel.length > 0)
            ToolTip.text: root.deviceManager.lastError.length > 0 ? root.deviceManager.lastError : root.deviceManager.portLabel
            ToolTip.delay: 350
            MouseArea { id: statusHover; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
        }

        Item { Layout.fillWidth:true }

        RowLayout {
            spacing: 7
            SoftButton { Layout.preferredWidth:80;Layout.preferredHeight:30;text:"Import";iconName:"upload";compact:true;toolbar:true }
            SoftButton { Layout.preferredWidth:80;Layout.preferredHeight:30;text:"Export";iconName:"download";compact:true;toolbar:true;checked:true }
        }
    }
}
