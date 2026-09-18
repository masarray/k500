import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

StudioPanel {
    id: root
    accentTop: false
    implicitHeight: 52

    required property var deviceManager
    required property var engine
    property bool transportPlaying: false
    signal aboutRequested()

    readonly property bool deviceBusy: deviceManager.status === "connecting"
                                       || deviceManager.status === "syncing"
                                       || deviceManager.reconciliationInProgress
    readonly property string deviceStatusText: deviceManager.status === "connected" ? "ONLINE"
                                                : deviceManager.status === "connecting" ? "CONNECT"
                                                : deviceManager.status === "syncing" ? "SYNC"
                                                : deviceManager.status === "error" ? "ERROR"
                                                : "OFFLINE"
    readonly property color deviceStatusAccent: deviceManager.status === "error" ? "#FF6B75"
                                                 : deviceManager.connected ? Theme.accent
                                                 : deviceBusy ? Theme.amber
                                                 : Theme.textDim

    // P0_TOPBAR_SEMANTIC_TRUTH_V1
    readonly property var presetManager: root.deviceManager ? root.deviceManager.presetManager : null
    readonly property var presetFileBridge: root.deviceManager ? root.deviceManager.presetFileBridge : null

    function slotLabel(slotOneBased) {
        var slot = Math.max(0, Math.round(Number(slotOneBased) || 0))
        return slot > 0 ? (slot < 10 ? "0" + slot : String(slot)) : ""
    }
    function deviceModeName(slotOneBased) {
        var slot = Math.round(Number(slotOneBased) || 0)
        if (slot < 1) return ""
        var state = root.engine && root.engine.deviceState ? root.engine.deviceState : null
        var system = state ? state.system : null
        var names = system ? system.deviceModeNames : null
        if (!names || slot > names.length) return ""
        return String(names[slot - 1] || "").trim()
    }
    function pcPresetName() {
        var bridge = root.presetFileBridge
        if (!bridge || !bridge.loaded) return ""
        var preset = String(bridge.presetName || "").trim()
        if (preset.length) return preset
        var source = String(bridge.sourceName || "").trim()
        if (source.toLowerCase().endsWith(".k500")) source = source.slice(0, -5)
        return source
    }

    readonly property string presetContextKind: {
        var manager = root.presetManager
        if (root.deviceManager.connected && manager && Number(manager.activeSlot) > 0)
            return "DEVICE SLOT " + root.slotLabel(manager.activeSlot)
        var bridge = root.presetFileBridge
        if (bridge && bridge.loaded)
            return bridge.editPersistenceEnabled ? (bridge.dirty ? "PC PREVIEW · EDITED" : "PC PREVIEW") : "PC STAGED"
        return root.deviceManager.connected ? "DEVICE STATE" : "NO PRESET"
    }
    readonly property string presetContextName: {
        var manager = root.presetManager
        if (root.deviceManager.connected && manager && Number(manager.activeSlot) > 0) {
            var mode = root.deviceModeName(manager.activeSlot)
            return mode.length ? mode : "SLOT " + root.slotLabel(manager.activeSlot)
        }
        var pcName = root.pcPresetName()
        if (pcName.length) return pcName
        return root.deviceManager.connected ? "CURRENT DEVICE" : "OFFLINE"
    }
    readonly property color presetContextAccent: {
        var bridge = root.presetFileBridge
        if (root.deviceManager.connected) return Theme.accent
        if (bridge && bridge.loaded && bridge.dirty) return Theme.amber
        if (bridge && bridge.loaded) return Theme.accent
        return Theme.textDim
    }

    FileDialog {
        id: supportReportDialog
        title: "Save K500 support report"
        fileMode: FileDialog.SaveFile
        nameFilters: ["JSON support report (*.json)"]
        currentFile: "SonKuPik-K500-support-report.json"
        onAccepted: root.deviceManager.saveSupportReport(selectedFile)
    }

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
                Image { anchors.fill:parent;anchors.margins:3;source:"qrc:/assets/SonKuPik-k500-logo.png";fillMode:Image.PreserveAspectFit;smooth:true }
            }

            ColumnLayout {
                spacing: -1
                RowLayout {
                    spacing: 3
                    Text { text:"SonKuPik";color:Theme.text;font.family:Theme.displayFamily;font.pixelSize:14;font.weight:Font.Bold }
                    Text { text:"K500";color:Theme.amber;font.family:Theme.displayFamily;font.pixelSize:14;font.weight:Font.Bold }
                }
                Text {
                    text:"KARAOKE PROCESSOR"
                    color:Theme.textDim
                    renderType:Text.NativeRendering
                    font.family:Theme.monoFamily
                    font.pixelSize:9
                    font.weight:Font.Medium
                    font.hintingPreference:Font.PreferFullHinting
                    font.letterSpacing:1.15
                }
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
                SoftButton { Layout.preferredWidth:27;Layout.fillHeight:true;transport:true;toolbar:true;iconName:"skip-back";iconOnly:true;enabled:root.deviceManager.connected&&!root.deviceManager.reconciliationInProgress;onClicked:root.deviceManager.sendPlayerCommand("rewind") }
                SoftButton {
                    Layout.preferredWidth:31;Layout.fillHeight:true;transport:true;toolbar:true
                    iconName:root.transportPlaying?"pause":"play";iconOnly:true
                    checked:root.transportPlaying;neonAccent:root.transportPlaying;accentIcon:true
                    enabled:root.deviceManager.connected&&!root.deviceManager.reconciliationInProgress
                    onClicked:{root.deviceManager.sendPlayerCommand("playPause");root.transportPlaying=!root.transportPlaying}
                }
                SoftButton { Layout.preferredWidth:27;Layout.fillHeight:true;transport:true;toolbar:true;iconName:"skip-forward";iconOnly:true;enabled:root.deviceManager.connected&&!root.deviceManager.reconciliationInProgress;onClicked:root.deviceManager.sendPlayerCommand("forward") }
                Rectangle { Layout.preferredWidth:1;Layout.preferredHeight:16;color:"#344049";opacity:.58 }
                SoftButton { Layout.preferredWidth:27;Layout.fillHeight:true;transport:true;toolbar:true;iconName:"volume-x";iconOnly:true;checked:root.deviceManager.muted;danger:root.deviceManager.muted;enabled:root.deviceManager.connected&&!root.deviceManager.reconciliationInProgress;onClicked:root.deviceManager.toggleMute() }
            }
        }

        Rectangle {
            id: presetContextChip
            Layout.preferredWidth: 144
            Layout.preferredHeight: 34
            radius: 9
            color: "#080D11"
            border.width: 1
            border.color: Qt.rgba(root.presetContextAccent.r, root.presetContextAccent.g, root.presetContextAccent.b,
                                  root.presetContextKind === "NO PRESET" ? .18 : .42)
            Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.top:parent.top;anchors.leftMargin:8;anchors.rightMargin:8;anchors.topMargin:1;height:1;color:root.presetContextAccent;opacity:root.presetContextKind==="NO PRESET"?.07:.18 }

            Column {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: -1
                Text {
                    width: parent.width
                    text: root.presetContextKind
                    color: root.presetContextAccent
                    renderType: Text.NativeRendering
                    font.family: Theme.monoFamily
                    font.pixelSize: 9
                    font.weight: Font.Bold
                    font.hintingPreference: Font.PreferFullHinting
                    font.letterSpacing: .60
                    elide: Text.ElideRight
                }
                Text {
                    width: parent.width
                    text: root.presetContextName
                    color: root.presetContextKind === "NO PRESET" ? Theme.textDim : Theme.text
                    renderType: Text.NativeRendering
                    font.family: Theme.fontFamily
                    font.pixelSize: 10
                    font.weight: Font.DemiBold
                    font.hintingPreference: Font.PreferFullHinting
                    elide: Text.ElideRight
                }
            }

            ToolTip.visible: presetContextHover.containsMouse
            ToolTip.text: root.presetContextKind + " — " + root.presetContextName
            ToolTip.delay: 350
            MouseArea { id:presetContextHover;anchors.fill:parent;hoverEnabled:true;acceptedButtons:Qt.NoButton;cursorShape:Qt.ArrowCursor }
        }

        RowLayout {
            spacing: 6
            RowLayout {
                visible: root.deviceManager.liveEnabled
                spacing: 5
                Rectangle { width:6;height:6;radius:3;color:Theme.accent }
                Text {
                    text:"LIVE"
                    color:Theme.accent
                    renderType:Text.NativeRendering
                    font.family:Theme.monoFamily
                    font.pixelSize:9
                    font.weight:Font.Bold
                    font.hintingPreference:Font.PreferFullHinting
                    font.letterSpacing:.65
                }
            }

            Rectangle {
                id: transportModeSegment
                Layout.preferredWidth: 104
                Layout.preferredHeight: 29
                radius: 7
                color: "#070B0E"
                border.width: 1
                border.color: "#26343D"
                Row {
                    anchors.fill: parent
                    anchors.margins: 1
                    spacing: 1
                    SoftButton { width:(parent.width-1)/2;height:parent.height;radius:5;text:"BT";iconName:"bluetooth";compact:true;toolbar:true;mixerSelect:true;checked:root.deviceManager.transportMode==="bt";enabled:!root.deviceBusy;onClicked:root.deviceManager.setTransportMode("bt") }
                    SoftButton { width:(parent.width-1)/2;height:parent.height;radius:5;text:"USB";iconName:"usb";compact:true;toolbar:true;mixerSelect:true;checked:root.deviceManager.transportMode==="usb";enabled:!root.deviceBusy;onClicked:root.deviceManager.setTransportMode("usb") }
                }
            }

            SoftButton {
                Layout.preferredWidth:94;Layout.preferredHeight:30
                text:root.deviceManager.connected||root.deviceBusy?"Disconnect":"Connect"
                iconName:root.deviceManager.connected?"unplug":"cable"
                compact:true;toolbar:true;mixerSelect:true;primaryAction:true
                checked:root.deviceManager.connected
                enabled:!root.deviceBusy||root.deviceManager.connected
                onClicked:root.deviceManager.toggleConnection()
            }
        }

        Rectangle {
            id: statusDisplay
            Layout.preferredWidth: 76
            Layout.preferredHeight: 29
            radius: 8
            color: "#080D11"
            border.width: 1
            border.color: Qt.rgba(root.deviceStatusAccent.r,root.deviceStatusAccent.g,root.deviceStatusAccent.b,
                                  root.deviceManager.connected||root.deviceBusy||root.deviceManager.status==="error"?.48:.22)
            Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.top:parent.top;anchors.leftMargin:7;anchors.rightMargin:7;anchors.topMargin:1;height:1;color:root.deviceStatusAccent;opacity:root.deviceManager.connected||root.deviceBusy?.22:.08 }
            Row {
                anchors.centerIn: parent
                spacing: 6
                Rectangle { anchors.verticalCenter:parent.verticalCenter;width:5;height:5;radius:3;color:root.deviceStatusAccent;opacity:root.deviceManager.status==="disconnected"?.55:1 }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.deviceStatusText
                    color: root.deviceStatusAccent
                    renderType: Text.NativeRendering
                    font.family: Theme.monoFamily
                    font.pixelSize: 9
                    font.weight: Font.Bold
                    font.hintingPreference: Font.PreferFullHinting
                    font.letterSpacing: .55
                }
            }
            ToolTip.visible: statusHover.containsMouse && (root.deviceManager.lastError.length > 0 || root.deviceManager.portLabel.length > 0)
            ToolTip.text: root.deviceManager.lastError.length > 0 ? root.deviceManager.lastError : root.deviceManager.portLabel
            ToolTip.delay: 350
            MouseArea { id:statusHover;anchors.fill:parent;hoverEnabled:true;acceptedButtons:Qt.NoButton;cursorShape:Qt.ArrowCursor }
        }

        Item { Layout.fillWidth:true }

        // ABOUT_ENTRY_V1 — compact information key; the floating About card
        // itself is owned by the ApplicationWindow so it stays truly centered.
        SoftButton {
            Layout.preferredWidth:32
            Layout.preferredHeight:30
            text:"i"
            compact:true
            toolbar:true
            onClicked:root.aboutRequested()
            ToolTip.visible:aboutHover.containsMouse
            ToolTip.text:"About SonKuPik K500"
            ToolTip.delay:300
            MouseArea { id:aboutHover;anchors.fill:parent;hoverEnabled:true;acceptedButtons:Qt.NoButton }
        }

        SoftButton {
            Layout.preferredWidth:94
            Layout.preferredHeight:30
            text:"Support"
            iconName:"file-down"
            compact:true
            toolbar:true
            onClicked:supportReportDialog.open()
            ToolTip.visible:supportHover.containsMouse
            ToolTip.text:"Export bounded K500 diagnostics"
            ToolTip.delay:350
            MouseArea { id:supportHover;anchors.fill:parent;hoverEnabled:true;acceptedButtons:Qt.NoButton }
        }
    }
}
