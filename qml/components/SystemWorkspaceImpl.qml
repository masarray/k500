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
    readonly property bool deviceConnected: !!root.presetManager && root.presetManager.connected
    readonly property bool offlineFileMode: !root.deviceConnected
    readonly property bool stagedPresetReady: !!root.fileBridge
                                              && root.fileBridge.loaded
                                              && root.fileBridge.checksumOk
    readonly property bool offlineEditMode: root.offlineFileMode
                                             && !!root.fileBridge
                                             && root.fileBridge.editPersistenceEnabled
    // P4_PC_PRESET_UPLOAD_UI_V1 — permanent write remains fail-closed unless
    // the staged PC document is valid and P2 confirms USB store availability.
    readonly property bool pcUploadReady: root.stagedPresetReady
                                          && !!root.presetManager
                                          && root.presetManager.usbStoreAvailable
                                          && !root.presetManager.busy
    // P4_2_PRESET_BATCH_UI_V1 — only final hardware execution is USB-gated.
    // The transfer window itself is deliberately available offline.
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
        if (root.fileBridge) {
            root.fileBridge.engine = root.engine
            // DEVICE_TRUTH_STAGING_V1: every new selection begins staged-only.
            // Explicit offline Preview is the only action that opts into the
            // controlled P3.4 edit-persistence session.
            root.fileBridge.editTracking = false
        }
    }
    function previewOrUploadLoadedPreset() {
        if (!root.stagedPresetReady)
            return
        if (root.offlineFileMode) {
            // OFFLINE_PREVIEW_V1 — explicit Preview hydrates visual/editor state
            // and the bridge then enables its whitelisted offline edit session.
            root.fileBridge.previewLoadedPreset()
            return
        }
        if (!root.pcUploadReady)
            return
        // Backend performs exact 0x0290 validation again before Store, then
        // recalls the same slot and full-readbacks the K500 before UI changes.
        root.presetManager.uploadSlotImage(root.selectedDeviceSlot + 1,
                                           root.fileBridge.deviceSlotImage())
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

    // OFFLINE_DEVICE_SLOT_V1 — ACTIVE is a hardware fact, never a fallback.
    // Without a connected K500/C0 handshake there is no active device slot.
    readonly property int activeDeviceSlot: {
        if (root.deviceConnected && Number(root.presetManager.activeSlot) > 0)
            return Math.max(0, Math.min(9, Number(root.presetManager.activeSlot) - 1))
        return -1
    }
    property int selectedDeviceSlot: 0
    property var deviceSlots: {
        if (!root.deviceConnected)
            return root.defaultDeviceSlots
        var names = root.systemValue("deviceModeNames", root.defaultDeviceSlots)
        return names && names.length === 10 ? names : root.defaultDeviceSlots
    }
    // MODE_NAME_UI_V1 — mirrors the selected hardware slot name only.
    // Persistent rename stays disabled until the donor write packet is captured.
    readonly property string selectedDeviceModeName: {
        if (!root.deviceConnected || root.selectedDeviceSlot < 0 || root.selectedDeviceSlot >= root.deviceSlots.length)
            return ""
        return String(root.deviceSlots[root.selectedDeviceSlot] || "").slice(0, 16)
    }
    readonly property int lowerRackHeight: 304

    Component.onCompleted: {
        root.bindFileBridgeEngine()
        if (root.activeDeviceSlot >= 0)
            root.selectedDeviceSlot = root.activeDeviceSlot
    }
    onFileBridgeChanged: root.bindFileBridgeEngine()

    Connections {
        target: root.presetManager
        enabled: !!root.presetManager
        function onActiveSlotChanged() {
            if (root.activeDeviceSlot >= 0)
                root.selectedDeviceSlot = root.activeDeviceSlot
        }
        function onConnectedChanged() {
            // DEVICE_TRUTH_EDIT_ISOLATION_V1 — a real K500 session always wins.
            // Disable PC edit persistence before any subsequent LIVE user edits.
            if (root.deviceConnected && root.fileBridge)
                root.fileBridge.editTracking = false
            if (root.activeDeviceSlot >= 0)
                root.selectedDeviceSlot = root.activeDeviceSlot
        }
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

    MassUploadTransferWindow {
        id: massPresetDialog
        fileBridge: root.fileBridge
        presetManager: root.presetManager
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
                                            text: loadedPreset ? "STAGED" : validPreset ? "SELECT" : "INVALID"
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
                                    text: root.fileBridge && root.fileBridge.loaded ? String(root.fileBridge.presetName) : "NO PRESET STAGED"
                                    color: root.fileBridge && root.fileBridge.loaded ? Theme.text : Theme.textDim
                                    font.family: Theme.monoFamily
                                    font.pixelSize: 9
                                    font.weight: Font.Bold
                                    elide: Text.ElideRight
                                }
                                Text {
                                    text: root.fileBridge && root.fileBridge.checksumOk
                                          ? (root.fileBridge.dirty ? "EDITED" : (root.offlineEditMode ? "PREVIEW" : "STAGED"))
                                          : ""
                                    color: root.fileBridge && root.fileBridge.dirty ? Theme.amber : Theme.accent
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
                                  : (root.fileBridge && root.fileBridge.dirty
                                     ? ("Verified edit · " + String(root.fileBridge.changedByteCount) + " changed byte(s) incl. checksum")
                                     : (root.offlineEditMode
                                        ? "Offline preview/edit · verified PEQ/fader edits persist to the staged preset"
                                        : (root.offlineFileMode && root.stagedPresetReady
                                           ? "Offline · press Preview to inspect and edit this preset"
                                           : (root.pcUploadReady
                                              ? ("Staged only · editor remains K500 truth · Upload to hardware slot " + String(root.selectedDeviceSlot + 1))
                                              : "PC library is separate from the 10 hardware slots"))))
                            color: root.fileBridge && String(root.fileBridge.lastError || "").length > 0 ? Theme.amber : Theme.textDim
                            font.family: Theme.monoFamily
                            font.pixelSize: 8
                            elide: Text.ElideRight
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            SoftButton { Layout.fillWidth: true; text: "Open file"; compact: true; enabled: !!root.fileBridge; onClicked: openPresetDialog.open() }
                            SoftButton { Layout.fillWidth: true; text: "Save as"; compact: true; enabled: root.fileBridge && root.fileBridge.loaded; onClicked: savePresetDialog.open() }
                            SoftButton {
                                Layout.fillWidth: true
                                text: root.offlineFileMode ? (root.offlineEditMode ? "Preview again" : "Preview") : (root.presetManager && root.presetManager.storeBusy ? "Uploading…" : "Upload")
                                compact: true
                                enabled: root.offlineFileMode ? root.stagedPresetReady : root.pcUploadReady
                                onClicked: root.previewOrUploadLoadedPreset()
                            }
                            SoftButton {
                                Layout.fillWidth: true
                                text: root.presetManager && root.presetManager.storeBusy ? "Uploading…" : "Mass"
                                compact: true
                                // OFFLINE_MASS_PREP_V1 — preparation is local-only and
                                // remains available without hardware. Final send is gated
                                // inside MassUploadTransferWindow.
                                enabled: !!root.fileBridge && (!root.presetManager || !root.presetManager.busy)
                                onClicked: massPresetDialog.openTransfer()
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
                        Layout.margins:10
                        spacing:7

                        Rectangle {
                            Layout.fillWidth:true
                            Layout.fillHeight:true
                            Layout.minimumHeight:311
                            radius:10
                            color:"#090D11"
                            border.width:1
                            border.color:"#050708"
                            clip:true

                            Column {
                                id: deviceSlotColumn
                                anchors.fill:parent
                                anchors.margins:7
                                spacing:3
                                Repeater {
                                    model:root.deviceSlots
                                    delegate:Rectangle {
                                        required property int index
                                        required property string modelData
                                        width:parent.width
                                        height:Math.max(27, (deviceSlotColumn.height - (9 * deviceSlotColumn.spacing)) / 10)
                                        radius:6
                                        readonly property bool active:root.deviceConnected && index===root.activeDeviceSlot
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

                        // Native KTV parity: selected hardware Mode Name.
                        // Read-only until persistent rename traffic is donor-verified.
                        RowLayout {
                            Layout.fillWidth:true
                            spacing:7
                            Text {
                                text:"MODE NAME"
                                color:Theme.textDim
                                font.family:Theme.monoFamily
                                font.pixelSize:8
                                font.letterSpacing:1.0
                                Layout.preferredWidth:68
                            }
                            Rectangle {
                                Layout.fillWidth:true
                                Layout.preferredHeight:29
                                radius:6
                                color:"#080C10"
                                border.width:1
                                border.color:root.deviceConnected?Theme.borderSoft:"#20272D"
                                TextInput {
                                    anchors.fill:parent
                                    anchors.leftMargin:9
                                    anchors.rightMargin:9
                                    verticalAlignment:TextInput.AlignVCenter
                                    text:root.selectedDeviceModeName
                                    readOnly:true
                                    selectByMouse:true
                                    maximumLength:16
                                    color:root.deviceConnected?Theme.amber:Theme.textDim
                                    font.family:Theme.monoFamily
                                    font.pixelSize:9
                                    font.weight:Font.Bold
                                    clip:true
                                }
                                Text {
                                    anchors.left:parent.left
                                    anchors.leftMargin:9
                                    anchors.verticalCenter:parent.verticalCenter
                                    visible:!root.deviceConnected
                                    text:"Connect K500 to read"
                                    color:Theme.textDim
                                    font.family:Theme.monoFamily
                                    font.pixelSize:8
                                }
                            }
                            Text {
                                text:"READ ONLY"
                                color:Theme.textDim
                                font.family:Theme.monoFamily
                                font.pixelSize:7
                                font.weight:Font.Bold
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
                                    enabled:root.presetManager&&!root.presetManager.busy
                                    onClicked:root.presetManager.setUseInitVolume(!root.presetManager.useInitVolume)
                                }
                            }
                            Text{text:"Use init volume";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:9}
                            Text{visible:root.offlineFileMode;text:"OFFLINE PREF";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:7}
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
                                enabled:root.deviceConnected&&root.presetManager&&!root.presetManager.busy
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

                // P2_SYSTEM_READONLY_AFFORDANCE_V1
                // These native-app surfaces are display-only until donor captures
                // prove the corresponding rename/reset/credential write traffic.
                // Never render enabled controls for an operation we cannot perform.
                StudioPanel {
                    Layout.fillWidth:true
                    Layout.preferredHeight:212
                    accentTop:false
                    ColumnLayout {
                        anchors.fill:parent
                        spacing:0
                        Item {
                            Layout.fillWidth:true
                            Layout.preferredHeight:35
                            Text{anchors.left:parent.left;anchors.leftMargin:12;anchors.verticalCenter:parent.verticalCenter;text:"BT / BLE IDENTITY";color:Theme.text;font.family:Theme.monoFamily;font.pixelSize:10;font.weight:Font.Bold;font.letterSpacing:1.05}
                            Rectangle{anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:1;color:Theme.borderSoft}
                        }
                        ColumnLayout {
                            Layout.fillWidth:true
                            Layout.fillHeight:true
                            Layout.margins:12
                            spacing:7
                            RowLayout {
                                Layout.fillWidth:true
                                Text{text:"BT NAME";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.letterSpacing:1.1}
                                Item{Layout.fillWidth:true}
                                Text{text:"READ ONLY";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:7;font.weight:Font.Bold}
                            }
                            Rectangle {
                                Layout.fillWidth:true
                                Layout.preferredHeight:29
                                radius:6
                                color:"#080C10"
                                border.width:1
                                border.color:Theme.borderSoft
                                Text{
                                    anchors.left:parent.left;anchors.leftMargin:9;anchors.verticalCenter:parent.verticalCenter
                                    text:root.deviceConnected ? String(root.systemValue("btName","NOT READ")) : "CONNECT K500 TO READ"
                                    color:root.deviceConnected ? Theme.amber : Theme.textDim
                                    font.family:Theme.monoFamily;font.pixelSize:10;font.weight:Font.Bold
                                }
                            }
                            Text{text:"BLE NAME";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.letterSpacing:1.1}
                            Rectangle {
                                Layout.fillWidth:true
                                Layout.preferredHeight:29
                                radius:6
                                color:"#080C10"
                                border.width:1
                                border.color:Theme.borderSoft
                                Text{
                                    anchors.left:parent.left;anchors.leftMargin:9;anchors.verticalCenter:parent.verticalCenter
                                    text:root.deviceConnected ? String(root.systemValue("bleName","NOT READ")) : "CONNECT K500 TO READ"
                                    color:root.deviceConnected ? Theme.amber : Theme.textDim
                                    font.family:Theme.monoFamily;font.pixelSize:10;font.weight:Font.Bold
                                }
                            }
                            Rectangle {
                                Layout.fillWidth:true
                                Layout.preferredHeight:31
                                radius:7
                                color:"#10161B"
                                border.width:1
                                border.color:Theme.borderSoft
                                RowLayout {
                                    anchors.fill:parent
                                    anchors.leftMargin:9
                                    anchors.rightMargin:9
                                    spacing:7
                                    Text{text:"READ ONLY";color:Theme.accent;font.family:Theme.monoFamily;font.pixelSize:7;font.weight:Font.Bold}
                                    Text{Layout.fillWidth:true;text:"Rename / reset mapping not verified";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:7;elide:Text.ElideRight}
                                }
                            }
                            Item{Layout.fillHeight:true}
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
                        Item {
                            Layout.fillWidth:true
                            Layout.preferredHeight:35
                            Text{anchors.left:parent.left;anchors.leftMargin:12;anchors.verticalCenter:parent.verticalCenter;text:"LOCK / ADMIN";color:Theme.text;font.family:Theme.monoFamily;font.pixelSize:10;font.weight:Font.Bold;font.letterSpacing:1.05}
                            Rectangle{anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:1;color:Theme.borderSoft}
                        }
                        ColumnLayout {
                            Layout.fillWidth:true
                            Layout.fillHeight:true
                            Layout.margins:12
                            spacing:8
                            RowLayout {
                                Layout.fillWidth:true
                                spacing:8
                                ColumnLayout {
                                    Layout.fillWidth:true
                                    spacing:4
                                    Text{text:"LOCK KEY";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.letterSpacing:1.0}
                                    Rectangle {
                                        Layout.fillWidth:true
                                        Layout.preferredHeight:29
                                        radius:6
                                        color:"#080C10"
                                        border.width:1
                                        border.color:Theme.borderSoft
                                        Text{anchors.left:parent.left;anchors.leftMargin:9;anchors.verticalCenter:parent.verticalCenter;text:root.deviceConnected ? "NOT EXPOSED" : "OFFLINE";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.weight:Font.Bold}
                                    }
                                }
                                ColumnLayout {
                                    Layout.fillWidth:true
                                    spacing:4
                                    Text{text:"ADMIN";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.letterSpacing:1.0}
                                    Rectangle {
                                        Layout.fillWidth:true
                                        Layout.preferredHeight:29
                                        radius:6
                                        color:"#080C10"
                                        border.width:1
                                        border.color:Theme.borderSoft
                                        Text{anchors.left:parent.left;anchors.leftMargin:9;anchors.verticalCenter:parent.verticalCenter;text:root.deviceConnected ? "NOT EXPOSED" : "OFFLINE";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.weight:Font.Bold}
                                    }
                                }
                            }
                            Rectangle {
                                Layout.fillWidth:true
                                Layout.preferredHeight:62
                                radius:8
                                color:"#10161B"
                                border.width:1
                                border.color:Theme.borderSoft
                                ColumnLayout {
                                    anchors.fill:parent
                                    anchors.margins:9
                                    spacing:3
                                    Text{text:"READ ONLY · DEVICE MANAGED";color:Theme.accent;font.family:Theme.monoFamily;font.pixelSize:8;font.weight:Font.Bold}
                                    Text{Layout.fillWidth:true;text:"Lock state, credentials and password writes are not yet donor-verified.";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:7;wrapMode:Text.WordWrap}
                                    Text{Layout.fillWidth:true;text:"Controls stay unavailable until the native write mapping is proven.";color:Theme.textFaint;font.family:Theme.monoFamily;font.pixelSize:7;elide:Text.ElideRight}
                                }
                            }
                            Item{Layout.fillHeight:true}
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
