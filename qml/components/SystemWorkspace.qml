import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Dialogs

Item {
    id: root
    required property var engine

    // P2_DEVICE_PRESET_UI_V1
    // Main.qml already owns the high-level DeviceManager used by TopBar. Resolve
    // its preset coordinator through the containing window without exposing raw
    // transport/I/O objects to this workspace.
    readonly property var presetManager: {
        var w = root.Window.window
        var dm = w ? w["deviceManager"] : null
        return dm ? dm.presetManager : null
    }
    // P3_3_PRESET_FILE_UI_V1
    // Resolve the validated P3.2 file bridge through the same high-level
    // DeviceManager boundary. It never exposes Controller/WinIo to QML.
    readonly property var fileBridge: {
        var w = root.Window.window
        var dm = w ? w["deviceManager"] : null
        return dm ? dm.presetFileBridge : null
    }
    readonly property bool offlineFileMode: !root.presetManager || !root.presetManager.connected
    // P4_PC_PRESET_UPLOAD_UI_V1 — permanent write remains fail-closed unless
    // the P3 working document is valid and P2 confirms USB store availability.
    readonly property bool pcUploadReady: !!root.fileBridge
                                          && root.fileBridge.loaded
                                          && root.fileBridge.checksumOk
                                          && !!root.presetManager
                                          && root.presetManager.usbStoreAvailable
                                          && !root.presetManager.busy
    // P4_2_PRESET_BATCH_UI_V1 — batch files are validated only after the user
    // selects them; before that we require only the safe USB transaction gate.
    readonly property bool massUploadReady: !!root.fileBridge
                                            && !!root.presetManager
                                            && root.presetManager.usbStoreAvailable
                                            && !root.presetManager.busy

    // P6_PC_PRESET_LIBRARY_UI_V1 — device slots are never populated from the
    // PC library. When the K500 has not supplied names yet, show neutral slot
    // labels instead of pretending that application presets already live there.
    readonly property var defaultDeviceSlots: [
        "SLOT 01", "SLOT 02", "SLOT 03", "SLOT 04", "SLOT 05",
        "SLOT 06", "SLOT 07", "SLOT 08", "SLOT 09", "SLOT 10"
    ]
    property int pcLibraryTab: 0 // 0 = built-in, 1 = local folder

    function systemValue(key, fallback) {
        var state = engine && engine.deviceState ? engine.deviceState : null
        var system = state ? state.system : null
        var value = system ? system[key] : undefined
        return value === undefined || value === null || value === "" ? fallback : value
    }
    function bindFileBridgeEngine() {
        if (root.fileBridge)
            root.fileBridge.engine = root.engine
    }
    function uploadLoadedPreset() {
        if (!root.pcUploadReady)
            return
        // Backend performs exact 0x0290 validation again before any Store frame.
        root.presetManager.uploadSlotImage(root.selectedDeviceSlot + 1,
                                           root.fileBridge.deviceSlotImage())
    }
    function massUploadFiles(files) {
        if (!root.massUploadReady)
            return
        var entries = root.fileBridge.buildMassUploadEntries(files,
                                                              root.selectedDeviceSlot + 1)
        if (entries && entries.length > 0)
            root.presetManager.massUploadSlotImages(entries)
    }
    function loadPcLibraryEntry(index) {
        if (!root.fileBridge)
            return
        root.bindFileBridgeEngine()
        if (root.pcLibraryTab === 0)
            root.fileBridge.loadBuiltInPreset(index)
        else
            root.fileBridge.loadFolderPreset(index)
    }

    readonly property int activeDeviceSlot: {
        if (root.presetManager && Number(root.presetManager.activeSlot) > 0)
            return Math.max(0, Math.min(9, Number(root.presetManager.activeSlot) - 1))
        return Math.max(0, Math.min(9, Number(systemValue("deviceModeIndex", 4)) - 1))
    }
    property int selectedDeviceSlot: 3
    property var deviceSlots: {
        var names = systemValue("deviceModeNames", root.defaultDeviceSlots)
        return names && names.length === 10 ? names : root.defaultDeviceSlots
    }
    readonly property int lowerRackHeight: 304

    Component.onCompleted: root.bindFileBridgeEngine()
    onFileBridgeChanged: root.bindFileBridgeEngine()

    Connections {
        target: root.engine
        function onDeviceStateChanged() { root.selectedDeviceSlot = root.activeDeviceSlot }
    }
    Connections {
        target: root.presetManager
        enabled: !!root.presetManager
        function onActiveSlotChanged() { root.selectedDeviceSlot = root.activeDeviceSlot }
    }

    FileDialog {
        id: openPresetDialog
        title: "Open K500 preset"
        fileMode: FileDialog.OpenFile
        nameFilters: ["K500 preset (*.k500)"]
        onAccepted: {
            root.bindFileBridgeEngine()
            if (root.fileBridge)
                root.fileBridge.loadFile(selectedFile)
        }
    }

    FolderDialog {
        id: presetFolderDialog
        title: "Select folder containing K500 presets"
        onAccepted: {
            root.bindFileBridgeEngine()
            if (root.fileBridge)
                root.fileBridge.setPresetFolder(selectedFolder)
        }
    }

    FileDialog {
        id: savePresetDialog
        title: "Save K500 preset copy"
        fileMode: FileDialog.SaveFile
        nameFilters: ["K500 preset (*.k500)"]
        onAccepted: {
            if (root.fileBridge)
                root.fileBridge.saveFile(selectedFile)
        }
    }

    FileDialog {
        id: massPresetDialog
        title: "Mass Upload K500 presets — start slot " + String(root.selectedDeviceSlot + 1)
        fileMode: FileDialog.OpenFiles
        nameFilters: ["K500 preset (*.k500)"]
        onAccepted: root.massUploadFiles(selectedFiles)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            StudioPanel {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 466
                accentTop: false

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 35
                        Text { anchors.left:parent.left;anchors.leftMargin:12;anchors.verticalCenter:parent.verticalCenter;text:"PC PRESET LIBRARY";color:Theme.text;font.family:Theme.monoFamily;font.pixelSize:10;font.weight:Font.Bold;font.letterSpacing:1.05 }
                        Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:1;color:Theme.borderSoft }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.margins: 10
                        spacing: 7

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Repeater {
                                model: ["SONKUPIK BANK", "LOCAL FOLDER"]
                                delegate: Rectangle {
                                    required property int index
                                    required property string modelData
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 28
                                    radius: 6
                                    color: root.pcLibraryTab === index ? "#15252A" : "#0B0F13"
                                    border.width: 1
                                    border.color: root.pcLibraryTab === index ? Theme.accentSoft : Theme.borderSoft
                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData
                                        color: root.pcLibraryTab === index ? Theme.accent : Theme.textDim
                                        font.family: Theme.monoFamily
                                        font.pixelSize: 8
                                        font.weight: Font.Bold
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: root.pcLibraryTab = index
                                    }
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            visible: root.pcLibraryTab === 1
                            spacing: 6
                            Text {
                                Layout.fillWidth: true
                                text: root.fileBridge && String(root.fileBridge.presetFolder || "").length > 0
                                      ? String(root.fileBridge.presetFolder)
                                      : "Choose a folder containing .k500 files"
                                color: root.fileBridge && String(root.fileBridge.presetFolder || "").length > 0 ? Theme.textSoft : Theme.textDim
                                font.family: Theme.monoFamily
                                font.pixelSize: 8
                                elide: Text.ElideMiddle
                            }
                            SoftButton { Layout.preferredWidth: 62; text: "Folder"; compact: true; enabled: !!root.fileBridge; onClicked: presetFolderDialog.open() }
                            SoftButton { Layout.preferredWidth: 58; text: "Refresh"; compact: true; enabled: !!root.fileBridge; onClicked: root.fileBridge.refreshPresetFolder() }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            radius: 9
                            color: "#090D11"
                            border.width: 1
                            border.color: "#050708"
                            clip: true

                            ListView {
                                id: pcPresetList
                                anchors.fill: parent
                                anchors.margins: 6
                                spacing: 3
                                clip: true
                                model: root.fileBridge
                                       ? (root.pcLibraryTab === 0 ? root.fileBridge.builtInPresets : root.fileBridge.folderPresets)
                                       : []

                                delegate: Rectangle {
                                    required property int index
                                    required property var modelData
                                    width: pcPresetList.width
                                    height: root.pcLibraryTab === 0 ? 40 : 34
                                    radius: 6
                                    readonly property bool validPreset: Boolean(modelData.valid)
                                    readonly property bool loadedPreset: root.fileBridge
                                                                        && root.fileBridge.loaded
                                                                        && (String(root.fileBridge.sourceName) === String(modelData.fileName))
                                    color: loadedPreset ? "#142328" : presetMouse.containsMouse ? "#12181D" : "#0D1115"
                                    border.width: 1
                                    border.color: loadedPreset ? Theme.accentSoft : validPreset ? "#252D34" : "#553A32"

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 8
                                        anchors.rightMargin: 8
                                        spacing: 8

                                        Text {
                                            text: String(index + 1).padStart(2, "0")
                                            color: validPreset ? Theme.amber : Theme.textDim
                                            font.family: Theme.monoFamily
                                            font.pixelSize: 8
                                            font.weight: Font.Bold
                                        }
                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: -1
                                            Text {
                                                Layout.fillWidth: true
                                                text: String(modelData.displayName || modelData.fileName || "K500 PRESET")
                                                color: validPreset ? Theme.text : Theme.textDim
                                                font.family: Theme.monoFamily
                                                font.pixelSize: 9
                                                font.weight: Font.Bold
                                                elide: Text.ElideRight
                                            }
                                            Text {
                                                visible: root.pcLibraryTab === 0
                                                Layout.fillWidth: true
                                                text: String(modelData.description || modelData.presetName || "")
                                                color: Theme.textDim
                                                font.family: Theme.fontFamily
                                                font.pixelSize: 8
                                                elide: Text.ElideRight
                                            }
                                        }
                                        Text {
                                            text: loadedPreset ? "LOADED" : validPreset ? "LOAD" : "INVALID"
                                            color: loadedPreset ? Theme.accent : validPreset ? Theme.textSoft : Theme.amber
                                            font.family: Theme.monoFamily
                                            font.pixelSize: 7
                                            font.weight: Font.Bold
                                        }
                                    }
                                    MouseArea {
                                        id: presetMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: validPreset ? Qt.PointingHandCursor : Qt.ArrowCursor
                                        enabled: validPreset
                                        onClicked: root.loadPcLibraryEntry(index)
                                    }
                                }

                                Text {
                                    anchors.centerIn: parent
                                    visible: pcPresetList.count === 0
                                    text: root.pcLibraryTab === 0 ? "Built-in preset bank unavailable" : "No .k500 files in selected folder"
                                    color: Theme.textDim
                                    font.family: Theme.monoFamily
                                    font.pixelSize: 9
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 34
                            radius: 7
                            color: "#0B0F13"
                            border.width: 1
                            border.color: root.fileBridge && root.fileBridge.loaded ? Theme.accentSoft : Theme.borderSoft
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                anchors.rightMargin: 8
                                spacing: 7
                                Text { text: "PC"; color: Theme.amber; font.family: Theme.monoFamily; font.pixelSize: 8; font.weight: Font.Bold }
                                Text {
                                    Layout.fillWidth: true
                                    text: root.fileBridge && root.fileBridge.loaded ? String(root.fileBridge.presetName) : "NO PRESET LOADED"
                                    color: root.fileBridge && root.fileBridge.loaded ? Theme.text : Theme.textDim
                                    font.family: Theme.monoFamily
                                    font.pixelSize: 9
                                    font.weight: Font.Bold
                                    elide: Text.ElideRight
                                }
                                Text {
                                    text: root.fileBridge && root.fileBridge.checksumOk ? (root.fileBridge.dirty ? "EDITED" : "VALID") : ""
                                    color: Theme.accent
                                    font.family: Theme.monoFamily
                                    font.pixelSize: 7
                                    font.weight: Font.Bold
                                }
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: root.fileBridge && String(root.fileBridge.lastError || "").length > 0
                                  ? String(root.fileBridge.lastError)
                                  : (root.fileBridge && root.fileBridge.loaded && root.fileBridge.dirty
                                     ? ("Verified edit · " + String(root.fileBridge.changedByteCount) + " changed byte(s) incl. checksum")
                                     : (root.pcUploadReady
                                        ? ("PC preset ready · explicit upload to hardware slot " + String(root.selectedDeviceSlot + 1))
                                        : "PC library is separate from the 10 hardware slots"))
                            color: root.fileBridge && String(root.fileBridge.lastError || "").length > 0 ? Theme.amber : Theme.textDim
                            font.family: Theme.monoFamily
                            font.pixelSize: 8
                            elide: Text.ElideRight
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            SoftButton { Layout.fillWidth: true; text: "Open file"; compact: true; enabled: root.offlineFileMode && !!root.fileBridge; onClicked: openPresetDialog.open() }
                            SoftButton { Layout.fillWidth: true; text: "Save as"; compact: true; enabled: root.fileBridge && root.fileBridge.loaded; onClicked: savePresetDialog.open() }
                            SoftButton {
                                Layout.fillWidth: true
                                text: root.presetManager && root.presetManager.storeBusy ? "Uploading…" : "Upload"
                                compact: true
                                enabled: root.pcUploadReady
                                onClicked: root.uploadLoadedPreset()
                            }
                            SoftButton {
                                Layout.fillWidth: true
                                text: root.presetManager && root.presetManager.storeBusy ? "Uploading…" : "Mass"
                                compact: true
                                enabled: root.massUploadReady
                                onClicked: massPresetDialog.open()
                            }
                        }
                    }
                }
            }

            StudioPanel {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 432
                accentTop: false

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Item {
                        Layout.fillWidth:true
                        Layout.preferredHeight:35
                        Text{anchors.left:parent.left;anchors.leftMargin:12;anchors.verticalCenter:parent.verticalCenter;text:"DEVICE PRESET SLOTS";color:Theme.text;font.family:Theme.monoFamily;font.pixelSize:10;font.weight:Font.Bold;font.letterSpacing:1.05}
                        Rectangle{anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:1;color:Theme.borderSoft}
                    }

                    ColumnLayout {
                        Layout.fillWidth:true
                        Layout.fillHeight:true
                        Layout.margins:12
                        spacing:8

                        RowLayout {
                            Layout.fillWidth:true
                            Text{Layout.fillWidth:true;text:(root.activeDeviceSlot+1)+" · "+String(root.deviceSlots[root.activeDeviceSlot] || "K500 DEVICE");color:Theme.accent;font.family:Theme.monoFamily;font.pixelSize:12;font.weight:Font.Bold}
                            Rectangle{width:52;height:24;radius:12;color:"#0B1715";border.width:1;border.color:"#31584E";Text{anchors.centerIn:parent;text:"ACTIVE";color:Theme.accent;font.family:Theme.monoFamily;font.pixelSize:8;font.weight:Font.Bold}}
                        }

                        Rectangle {
                            Layout.fillWidth:true
                            Layout.fillHeight:true
                            radius:10
                            color:"#090D11"
                            border.width:1
                            border.color:"#050708"
                            clip:true

                            Column {
                                anchors.fill:parent
                                anchors.margins:7
                                spacing:3
                                Repeater {
                                    model:root.deviceSlots
                                    delegate:Rectangle {
                                        required property int index
                                        required property string modelData
                                        width:parent.width
                                        height:27
                                        radius:6
                                        readonly property bool active:index===root.activeDeviceSlot
                                        readonly property bool selected:index===root.selectedDeviceSlot
                                        color:selected?"#151B20":"#0D1115"
                                        border.width:1
                                        border.color:active?"#8B6A08":selected?Theme.accentSoft:"#252D34"
                                        RowLayout {
                                            anchors.fill:parent;anchors.leftMargin:9;anchors.rightMargin:9;spacing:7
                                            Text{text:index+1;color:Theme.amber;font.family:Theme.monoFamily;font.pixelSize:9;font.weight:Font.Bold}
                                            Text{Layout.fillWidth:true;text:modelData;color:Theme.text;font.family:Theme.monoFamily;font.pixelSize:9;font.weight:Font.Bold;elide:Text.ElideRight}
                                            Text{text:active?"ACTIVE":"";color:Theme.accent;font.family:Theme.monoFamily;font.pixelSize:8;font.weight:Font.Bold}
                                        }
                                        MouseArea{anchors.fill:parent;cursorShape:Qt.PointingHandCursor;enabled:!root.presetManager||!root.presetManager.busy;onClicked:root.selectedDeviceSlot=index}
                                    }
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth:true
                            spacing:5
                            Rectangle{
                                id:initVolumeCheck
                                width:13;height:13;radius:2
                                color:root.presetManager&&root.presetManager.useInitVolume?Theme.accent:"#4A5055"
                                border.width:1
                                border.color:root.presetManager&&root.presetManager.useInitVolume?Theme.accentSoft:"#60676C"
                                Text{anchors.centerIn:parent;visible:root.presetManager&&root.presetManager.useInitVolume;text:"✓";color:"#071012";font.pixelSize:10;font.weight:Font.Bold}
                                MouseArea{
                                    anchors.fill:parent
                                    cursorShape:Qt.PointingHandCursor
                                    enabled:root.presetManager&&root.presetManager.connected&&!root.presetManager.busy
                                    onClicked:root.presetManager.setUseInitVolume(!root.presetManager.useInitVolume)
                                }
                            }
                            Text{text:"Use init volume";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:9}
                            Item{Layout.fillWidth:true}
                            Text{
                                visible:root.presetManager&&String(root.presetManager.progress||"").length>0
                                text:String(root.presetManager?root.presetManager.progress:"")
                                color:root.presetManager&&root.presetManager.busy?Theme.amber:Theme.textDim
                                font.family:Theme.monoFamily;font.pixelSize:8
                                elide:Text.ElideRight
                                Layout.maximumWidth:180
                            }
                        }

                        RowLayout {
                            Layout.fillWidth:true
                            spacing:8
                            SoftButton{
                                Layout.fillWidth:true;text:root.presetManager&&root.presetManager.recallBusy?"Recalling…":"Recall";compact:true
                                enabled:root.presetManager&&root.presetManager.connected&&!root.presetManager.busy
                                onClicked:root.presetManager.recallMode(root.selectedDeviceSlot+1)
                            }
                            SoftButton{
                                Layout.fillWidth:true;text:root.presetManager&&root.presetManager.storeBusy?"Saving…":"Save";compact:true
                                enabled:root.presetManager&&root.presetManager.usbStoreAvailable&&!root.presetManager.busy
                                onClicked:root.presetManager.saveCurrentToSlot(root.selectedDeviceSlot+1)
                            }
                            SoftButton{Layout.fillWidth:true;text:"Reset all";compact:true;enabled:false}
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 355
                spacing: 12

                StudioPanel {
                    Layout.fillWidth:true
                    Layout.preferredHeight:212
                    accentTop:false
                    ColumnLayout {
                        anchors.fill:parent
                        spacing:0
                        Item{Layout.fillWidth:true;Layout.preferredHeight:35;Text{anchors.left:parent.left;anchors.leftMargin:12;anchors.verticalCenter:parent.verticalCenter;text:"BT NAME";color:Theme.text;font.family:Theme.monoFamily;font.pixelSize:10;font.weight:Font.Bold;font.letterSpacing:1.05}Rectangle{anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:1;color:Theme.borderSoft}}
                        ColumnLayout {
                            Layout.fillWidth:true;Layout.fillHeight:true;Layout.margins:12;spacing:8
                            Text{text:"BT NAME";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.letterSpacing:1.1}
                            Rectangle{Layout.fillWidth:true;Layout.preferredHeight:29;radius:6;color:"#080C10";border.width:1;border.color:"#050708";Text{anchors.left:parent.left;anchors.leftMargin:9;anchors.verticalCenter:parent.verticalCenter;text:String(root.systemValue("btName","KTV_BT_00AB12"));color:Theme.amber;font.family:Theme.monoFamily;font.pixelSize:10;font.weight:Font.Bold}}
                            Text{text:"BLE NAME";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.letterSpacing:1.1}
                            Rectangle{Layout.fillWidth:true;Layout.preferredHeight:29;radius:6;color:"#080C10";border.width:1;border.color:"#050708";Text{anchors.left:parent.left;anchors.leftMargin:9;anchors.verticalCenter:parent.verticalCenter;text:String(root.systemValue("bleName","KTV_BLE_00AB12"));color:Theme.amber;font.family:Theme.monoFamily;font.pixelSize:10;font.weight:Font.Bold}}
                            Item{Layout.fillHeight:true}
                            RowLayout{Layout.fillWidth:true;spacing:8;SoftButton{Layout.fillWidth:true;text:"Modify";compact:true}SoftButton{Layout.fillWidth:true;text:"Reset";compact:true}}
                        }
                    }
                }

                StudioPanel {
                    Layout.fillWidth:true
                    Layout.fillHeight:true
                    accentTop:false
                    ColumnLayout {
                        anchors.fill:parent
                        spacing:0
                        Item{Layout.fillWidth:true;Layout.preferredHeight:35;Text{anchors.left:parent.left;anchors.leftMargin:12;anchors.verticalCenter:parent.verticalCenter;text:"LOCK / ADMIN";color:Theme.text;font.family:Theme.monoFamily;font.pixelSize:10;font.weight:Font.Bold;font.letterSpacing:1.05}Rectangle{anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:1;color:Theme.borderSoft}}
                        ColumnLayout {
                            Layout.fillWidth:true;Layout.fillHeight:true;Layout.margins:12;spacing:8
                            RowLayout {
                                Layout.fillWidth:true;spacing:8
                                ColumnLayout{Layout.fillWidth:true;spacing:4;Text{text:"LOCK KEY";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.letterSpacing:1.0}Rectangle{Layout.fillWidth:true;Layout.preferredHeight:29;radius:6;color:"#080C10";border.width:1;border.color:"#050708";Text{anchors.left:parent.left;anchors.leftMargin:9;anchors.verticalCenter:parent.verticalCenter;text:"••••";color:Theme.amber;font.family:Theme.monoFamily;font.pixelSize:10}}}
                                ColumnLayout{Layout.fillWidth:true;spacing:4;Text{text:"ADMIN";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.letterSpacing:1.0}Rectangle{Layout.fillWidth:true;Layout.preferredHeight:29;radius:6;color:"#080C10";border.width:1;border.color:"#050708";Text{anchors.left:parent.left;anchors.leftMargin:9;anchors.verticalCenter:parent.verticalCenter;text:"••••";color:Theme.amber;font.family:Theme.monoFamily;font.pixelSize:10}}}
                            }
                            RowLayout {
                                Layout.fillWidth:true;spacing:9
                                Repeater {
                                    model:[{label:"Unlock",on:true},{label:"Lock",on:false},{label:"Admin",on:true}]
                                    delegate:RowLayout {
                                        required property var modelData
                                        spacing:4
                                        Rectangle{width:13;height:13;radius:2;color:modelData.on?Theme.accent:"#E6E9EB";border.width:1;border.color:modelData.on?Theme.accentSoft:"#B8C0C6";Text{anchors.centerIn:parent;visible:modelData.on;text:"✓";color:"#071012";font.pixelSize:10;font.weight:Font.Bold}}
                                        Text{text:modelData.label;color:Theme.text;font.family:Theme.fontFamily;font.pixelSize:9}
                                    }
                                }
                                Item{Layout.fillWidth:true}
                            }
                            Item{Layout.fillHeight:true}
                            Text{text:"NEW PASSWORD";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.letterSpacing:1.1}
                            RowLayout {
                                Layout.fillWidth:true;spacing:8
                                Rectangle{Layout.fillWidth:true;Layout.preferredHeight:29;radius:6;color:"#080C10";border.width:1;border.color:"#050708"}
                                SoftButton{Layout.preferredWidth:58;text:"Modify";compact:true}
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: root.lowerRackHeight
            Layout.minimumHeight: root.lowerRackHeight
            Layout.maximumHeight: root.lowerRackHeight
            spacing: 12

            RackFaderPanel {
                Layout.fillWidth:true
                Layout.fillHeight:true
                Layout.preferredWidth:466
                title:"Startup Limits"
                channels:[
                    {label:"MUSIC INIT",value:Number(root.systemValue("musicInitVol",25)),from:0,to:84,step:1,unit:"",decimals:0},
                    {label:"MUSIC MAX",value:Number(root.systemValue("musicMaxVol",84)),from:0,to:84,step:1,unit:"",decimals:0},
                    {label:"MIC INIT",value:Number(root.systemValue("micInitVol",25)),from:0,to:84,step:1,unit:"",decimals:0},
                    {label:"MIC MAX",value:Number(root.systemValue("micMaxVol",84)),from:0,to:84,step:1,unit:"",decimals:0},
                    {label:"EFFECT INIT",value:Number(root.systemValue("effectInitLevel",25)),from:0,to:84,step:1,unit:"",decimals:0}
                ]
            }

            StudioPanel {
                Layout.fillWidth:true
                Layout.fillHeight:true
                Layout.preferredWidth:432
                accentTop:false
                ColumnLayout {
                    anchors.fill:parent
                    spacing:0
                    Item{Layout.fillWidth:true;Layout.preferredHeight:35;Text{anchors.left:parent.left;anchors.leftMargin:12;anchors.verticalCenter:parent.verticalCenter;text:"RECORDING / MIC TRIGGER";color:Theme.text;font.family:Theme.monoFamily;font.pixelSize:10;font.weight:Font.Bold;font.letterSpacing:1.05}Rectangle{anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:1;color:Theme.borderSoft}}
                    RowLayout {
                        Layout.fillWidth:true;Layout.fillHeight:true;Layout.margins:10;spacing:10
                        Rectangle {
                            Layout.fillWidth:true;Layout.fillHeight:true;radius:10;color:"#151B21";border.width:1;border.color:Theme.borderSoft
                            ColumnLayout {
                                anchors.fill:parent;anchors.margins:8;spacing:5
                                Text{text:"RECORDING";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.letterSpacing:1.1}
                                RowLayout {
                                    Layout.fillWidth:true;Layout.fillHeight:true;spacing:7
                                    Repeater {
                                        model:[
                                            {label:"UDISK REC",value:Number(root.systemValue("uDiskRecordVol",4)),from:1,to:6},
                                            {label:"USB REC",value:Number(root.systemValue("usbRecordVol",4)),from:1,to:6}
                                        ]
                                        delegate:ColumnLayout {
                                            id: recChannel
                                            required property var modelData
                                            property real localValue: Number(modelData.value)
                                            Layout.fillWidth:true;Layout.fillHeight:true;spacing:3
                                            Text{Layout.alignment:Qt.AlignHCenter;text:modelData.label;color:recFader.highlighted?recFader.accentColor:Theme.textDim;style:recFader.highlighted?Text.Outline:Text.Normal;styleColor:recFader.highlighted?Qt.rgba(recFader.accentColor.r,recFader.accentColor.g,recFader.accentColor.b,.34):"transparent";font.family:Theme.monoFamily;font.pixelSize:8;font.weight:recFader.highlighted?Font.DemiBold:Font.Normal;Behavior on color{ColorAnimation{duration:75}}Behavior on styleColor{ColorAnimation{duration:75}}}
                                            StudioFader{id:recFader;Layout.fillHeight:true;Layout.preferredWidth:48;Layout.alignment:Qt.AlignHCenter;value:recChannel.localValue;from:modelData.from;to:modelData.to;step:1;defaultValue:modelData.value;onValueEdited:function(v){recChannel.localValue=v}}
                                            Rectangle{Layout.alignment:Qt.AlignHCenter;Layout.preferredWidth:48;Layout.preferredHeight:23;radius:8;color:"#080C10";border.width:1;border.color:recFader.highlighted?recFader.accentColor:"#050708";Behavior on border.color{ColorAnimation{duration:75}}Text{anchors.centerIn:parent;text:recChannel.localValue;color:Theme.amber;font.family:Theme.monoFamily;font.pixelSize:9;font.weight:Font.Bold}}
                                        }
                                    }
                                }
                            }
                        }
                        Rectangle {
                            Layout.fillWidth:true;Layout.fillHeight:true;radius:10;color:"#151B21";border.width:1;border.color:Theme.borderSoft
                            ColumnLayout {
                                anchors.fill:parent;anchors.margins:8;spacing:5
                                Text{text:"MIC TRIGGER";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.letterSpacing:1.1}
                                RowLayout {
                                    Layout.fillWidth:true;Layout.fillHeight:true;spacing:7
                                    Repeater {
                                        model:[{label:"THRESHOLD",value:-50,from:-80,to:0,unit:"dB"},{label:"HOLD TIME",value:6,from:0,to:30,unit:"s"}]
                                        delegate:ColumnLayout {
                                            id: triggerChannel
                                            required property var modelData
                                            property real localValue: Number(modelData.value)
                                            Layout.fillWidth:true;Layout.fillHeight:true;spacing:3
                                            Text{Layout.alignment:Qt.AlignHCenter;text:modelData.label;color:triggerFader.highlighted?triggerFader.accentColor:Theme.textDim;style:triggerFader.highlighted?Text.Outline:Text.Normal;styleColor:triggerFader.highlighted?Qt.rgba(triggerFader.accentColor.r,triggerFader.accentColor.g,triggerFader.accentColor.b,.34):"transparent";font.family:Theme.monoFamily;font.pixelSize:8;font.weight:triggerFader.highlighted?Font.DemiBold:Font.Normal;Behavior on color{ColorAnimation{duration:75}}Behavior on styleColor{ColorAnimation{duration:75}}}
                                            StudioFader{id:triggerFader;Layout.fillHeight:true;Layout.preferredWidth:48;Layout.alignment:Qt.AlignHCenter;value:triggerChannel.localValue;from:modelData.from;to:modelData.to;step:1;defaultValue:modelData.value;onValueEdited:function(v){triggerChannel.localValue=v}}
                                            Rectangle{Layout.alignment:Qt.AlignHCenter;Layout.preferredWidth:52;Layout.preferredHeight:23;radius:8;color:"#080C10";border.width:1;border.color:triggerFader.highlighted?triggerFader.accentColor:"#050708";Behavior on border.color{ColorAnimation{duration:75}}Row{anchors.centerIn:parent;spacing:3;Text{text:triggerChannel.localValue;color:Theme.amber;font.family:Theme.monoFamily;font.pixelSize:9;font.weight:Font.Bold}Text{text:modelData.unit;color:triggerFader.highlighted?Theme.textSoft:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:7;anchors.baseline:parent.children[0].baseline;Behavior on color{ColorAnimation{duration:75}}}}}
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            MasterStripPanel {
                engine: root.engine
                Layout.fillWidth:true
                Layout.fillHeight:true
                Layout.preferredWidth:355
            }
        }
    }
}
