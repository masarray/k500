import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Controls

Item {
    id: root
    required property var engine

    // P2_DEVICE_PRESET_UI_V1
    // Main.qml already owns the high-level DeviceManager used by TopBar. Resolve
    // its preset coordinator through the containing window without exposing raw
    // transport/I/O objects to this workspace.
    readonly property var deviceManager: {
        var w = root.Window.window
        return w ? w["deviceManager"] : null
    }
    readonly property var presetManager: root.deviceManager ? root.deviceManager.presetManager : null
    // P3_3_PRESET_FILE_UI_V1
    // Resolve the validated P3.2 file bridge through the same high-level
    // DeviceManager boundary. It never exposes Controller/WinIo to QML.
    readonly property var fileBridge: root.deviceManager ? root.deviceManager.presetFileBridge : null

    // SYSTEM_TOGGLE_DUAL_AUTHORITY_V2
    // OFFLINE and ONLINE are intentionally isolated authorities:
    // - OFFLINE edits only the local editor / working preset.
    // - ONLINE ignores all offline shadow state and uses device readback + live setters.
    // Nothing from offline is queued or auto-applied when a K500 later connects.
    readonly property bool deviceConnected: !!root.presetManager && root.presetManager.connected
    readonly property bool deviceReady: root.deviceConnected
                                        && !!root.deviceManager
                                        && root.deviceManager.status === "connected"
                                        && !root.presetManager.busy

    property bool offlineUseInitVolume: false
    property bool offlineAdjMannerVrOff: false
    property string systemControlHintTarget: ""
    property string systemControlHint: ""
    // SAVE_AS_EXPLICIT_SOURCE_GATE_V1 — live device readback is not a complete
    // 1144-byte .k500 document; never silently export fabricated file data.
    property string pcSaveNotice: ""
    Timer {
        id: pcSaveNoticeTimer
        interval: 6500
        onTriggered: root.pcSaveNotice = ""
    }
    function requestSaveAs() {
        if (!root.fileBridge)
            return
        if (!root.fileBridge.loaded) {
            root.pcSaveNotice = "Select a preset from the PC bank/local folder or Open file before Save As. Device LIVE readback is not a complete .k500 file."
            pcSaveNoticeTimer.restart()
            return
        }
        root.pcSaveNotice = ""
        savePresetDialog.open()
    }

    Timer {
        id: systemControlHintTimer
        interval: 2200
        repeat: false
        onTriggered: {
            root.systemControlHintTarget = ""
            root.systemControlHint = ""
        }
    }

    function showSystemControlHint(target, message) {
        root.systemControlHintTarget = target
        root.systemControlHint = message
        systemControlHintTimer.restart()
    }

    function blockedOnlineToggleHint(target) {
        if (root.deviceManager && root.deviceManager.status === "syncing") {
            root.showSystemControlHint(target, "SYNCING DEVICE")
            return
        }
        if (root.presetManager && root.presetManager.busy) {
            root.showSystemControlHint(target, "DEVICE BUSY")
            return
        }
        root.showSystemControlHint(target, "WAIT FOR DEVICE READY")
    }

    function seedOfflineAdjMannerFromPreview() {
        if (root.deviceConnected)
            return
        var state = root.engine && root.engine.deviceState ? root.engine.deviceState : null
        var system = state ? state.system : null
        if (system && system.adjMannerVrOffKnown)
            root.offlineAdjMannerVrOff = !!system.adjMannerVrOff
    }

    // SYSTEM_OFFLINE_HANDOFF_V1
    // Keep the offline working controls aligned with the latest accepted/read
    // ONLINE state. Unknown/optimistic values are deliberately ignored. On
    // disconnect the bindings fall back to these shadows without changing them.
    // Reconnect never uploads them; authoritative device hydration wins again.
    function mirrorAcceptedDeviceTogglesToOffline() {
        if (!root.deviceConnected || !root.presetManager)
            return
        if (root.presetManager.useInitVolumeKnown)
            root.offlineUseInitVolume = !!root.presetManager.useInitVolume
        if (root.presetManager.adjMannerVrOffKnown)
            root.offlineAdjMannerVrOff = !!root.presetManager.adjMannerVrOff
    }

    // SYSTEM_LATE_LOAD_OFFLINE_SEED_V1
    // SystemWorkspaceImpl is lazy-created. If a complete device session already
    // connected and disconnected before the first System visit, the manager
    // intentionally retains the last accepted boolean payloads while Known=false.
    // Seed presentation only; this never restores hardware authority or queues I/O.
    function seedOfflineTogglesFromRetainedSession() {
        if (root.deviceConnected || !root.presetManager)
            return
        root.offlineUseInitVolume = !!root.presetManager.useInitVolume
        root.offlineAdjMannerVrOff = !!root.presetManager.adjMannerVrOff
    }

    function requestUseInitVolumeToggle() {
        if (!root.deviceConnected) {
            // Use Init is capture-proven as a device-global C0/CMD 0x12 state,
            // not as part of the 0x0290 slot image. Offline is therefore a local
            // editor state only; it is never queued for reconnect.
            root.offlineUseInitVolume = !root.offlineUseInitVolume
            return
        }
        if (!root.deviceReady) {
            root.blockedOnlineToggleHint("init")
            return
        }
        root.presetManager.setUseInitVolume(!root.presetManager.useInitVolume)
    }

    function requestAdjMannerVrToggle() {
        if (!root.deviceConnected) {
            var next = !root.offlineAdjMannerVrOff
            root.offlineAdjMannerVrOff = next

            // File scalar 0x0094 is capture-proven Adj Manner VR OFF.
            // Persist only inside an explicit Preview/edit session. No file
            // edit is ever replayed automatically when the device connects.
            if (root.fileBridge && root.fileBridge.editPersistenceEnabled)
                root.fileBridge.setOfflineAdjMannerVrOff(next)
            return
        }
        if (!root.deviceReady) {
            root.blockedOnlineToggleHint("vr")
            return
        }
        root.presetManager.setAdjMannerVrOff(!root.presetManager.adjMannerVrOff)
    }
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
        // OFFLINE_LAST_READBACK_VALUES_V1 — display confirmed session values
        // after disconnect without treating them as *current* device authority.
        // Offline explicit Preview still owns the editable engine state.
        var state = null
        if (root.engine && root.engine.deviceStateReady)
            state = root.engine.deviceState
        else if (!root.deviceConnected && root.engine)
            state = root.engine.retainedDeviceState
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
    // DEVICE_SLOT_OFFLINE_SHADOW_V1 — a disconnected K500 has no ACTIVE
    // authority, but its last successfully read ten names remain informative.
    // Never infer physical slots from the PC preset collection.
    property var deviceSlots: {
        var liveNames = root.deviceConnected
                        ? root.systemValue("deviceModeNames", root.defaultDeviceSlots) : null
        if (liveNames && liveNames.length === 10)
            return liveNames
        var retainedNames = root.engine ? root.engine.retainedDeviceModeNames : null
        return retainedNames && retainedNames.length === 10 ? retainedNames : root.defaultDeviceSlots
    }
    // MODE_NAME_STORE_CAPTURED_V1 — table readback is hardware truth. Persistent
    // rename is allowed only for the ACTIVE slot because the native transaction
    // stores the current 0x0290 image after patching its 16-byte name field.
    readonly property string selectedDeviceModeName: {
        if (root.selectedDeviceSlot < 0 || root.selectedDeviceSlot >= root.deviceSlots.length)
            return ""
        return String(root.deviceSlots[root.selectedDeviceSlot] || "").slice(0, 16)
    }
    property string modeNameDraft: ""
    function normalizedModeNameDraft() {
        return String(root.modeNameDraft || "").trim()
    }
    function validModeNameDraft() {
        var name = root.normalizedModeNameDraft()
        return name.length >= 1 && name.length <= 16 && /^[ -~]+$/.test(name)
    }
    readonly property bool deviceModeRenameReady: root.deviceConnected
                                                   && root.presetManager
                                                   && root.presetManager.usbStoreAvailable
                                                   && !root.presetManager.busy
                                                   && root.activeDeviceSlot >= 0
                                                   && root.selectedDeviceSlot === root.activeDeviceSlot
                                                   && root.validModeNameDraft()
                                                   && root.normalizedModeNameDraft() !== String(root.selectedDeviceModeName || "").trim()

    readonly property string currentBtName: root.deviceConnected ? String(root.systemValue("btName","")) : ""
    property string btNameDraft: ""
    function normalizedBtNameDraft() { return String(root.btNameDraft || "").trim() }
    function validBtNameDraft() {
        var name = root.normalizedBtNameDraft()
        return name.length >= 1 && name.length <= 8 && /^[ -~]+$/.test(name)
    }
    readonly property bool btNameWriteReady: root.deviceConnected
                                             && root.presetManager
                                             && root.presetManager.usbStoreAvailable
                                             && !root.presetManager.busy
                                             && root.validBtNameDraft()
                                             && root.normalizedBtNameDraft() !== String(root.currentBtName || "").trim()
    readonly property int lowerRackHeight: 304

    // STARTUP_LIMIT_STABLE_MODEL_V1 — metadata identity is constant for the
    // lifetime of this workspace. Live values are owned by StudioEngine and
    // resolved in-place by RackFaderPanel; changing one value must never rebuild
    // all five delegates or cancel an active pointer grab.
    readonly property var startupLimitChannels: [
        {label:"MUSIC INIT",from:0,to:84,step:1,unit:"",decimals:0},
        {label:"MUSIC MAX",from:0,to:84,step:1,unit:"",decimals:0},
        {label:"MIC INIT",from:0,to:84,step:1,unit:"",decimals:0},
        {label:"MIC MAX",from:0,to:84,step:1,unit:"",decimals:0},
        {label:"EFFECT INIT",from:0,to:84,step:1,unit:"",decimals:0}
    ]
    function startupLimitValue(label, fallback) {
        if (!root.engine) return fallback
        var key = String(label || "").toUpperCase()
        if (key === "MUSIC INIT") return Number(root.engine.musicInitVol)
        if (key === "MUSIC MAX") return Number(root.engine.musicMaxVol)
        if (key === "MIC INIT") return Number(root.engine.micInitVol)
        if (key === "MIC MAX") return Number(root.engine.micMaxVol)
        if (key === "EFFECT INIT") return Number(root.engine.effectInitLevel)
        return fallback
    }

    // SYSTEM_LIVE_STABLE_DELEGATE_V1 — Recording and Dance Mic controls use
    // immutable metadata lists. Device readback changes only local values inside
    // existing delegates, so a reconciliation snapshot cannot destroy a drag.
    readonly property var recordingChannels: [
        {label:"UDISK REC",key:"uDiskRecordVol",fallback:4,from:1,to:6,path:"system.uDiskRecordVol",badge:"LIVE"},
        {label:"USB REC",key:"usbRecordVol",fallback:4,from:1,to:6,path:"system.usbRecordVol",badge:"LIVE"}
    ]
    readonly property var micTriggerChannels: [
        {label:"THRESHOLD",key:"danceMicThresholdDb",fallback:-50,from:-60,to:0,unit:"dB",path:"system.danceMicThresholdDb"},
        {label:"HOLD TIME",key:"danceMicHoldSec",fallback:6,from:1,to:30,unit:"s",path:"system.danceMicHoldSec"}
    ]
    function stableSystemValue(spec) {
        return Number(root.systemValue(String(spec.key || ""), Number(spec.fallback || 0)))
    }

    onSelectedDeviceModeNameChanged: root.modeNameDraft = root.selectedDeviceModeName
    onCurrentBtNameChanged: root.btNameDraft = root.currentBtName

    Component.onCompleted: {
        root.bindFileBridgeEngine()
        if (root.deviceConnected)
            root.mirrorAcceptedDeviceTogglesToOffline()
        else
            root.seedOfflineTogglesFromRetainedSession()
        if (root.activeDeviceSlot >= 0)
            root.selectedDeviceSlot = root.activeDeviceSlot
        else if (root.presetManager && Number(root.presetManager.lastKnownSlot) > 0)
            root.selectedDeviceSlot = Number(root.presetManager.lastKnownSlot) - 1
        root.modeNameDraft = root.selectedDeviceModeName
        root.btNameDraft = root.currentBtName
    }
    onFileBridgeChanged: root.bindFileBridgeEngine()

    Connections {
        target: root.fileBridge
        enabled: !!root.fileBridge
        function onEditTrackingChanged() {
            if (!root.deviceConnected && root.fileBridge.editPersistenceEnabled)
                root.seedOfflineAdjMannerFromPreview()
        }
    }

    Connections {
        target: root.presetManager
        enabled: !!root.presetManager
        function onActiveSlotChanged() {
            if (root.activeDeviceSlot >= 0)
                root.selectedDeviceSlot = root.activeDeviceSlot
        }
        function onConnectedChanged() {
            // DEVICE_TRUTH_EDIT_ISOLATION_V1 — a real K500 session always wins.
            // On connect, mirror the authoritative hydrated values into the
            // offline working shadow before LIVE editing starts. On disconnect,
            // do not touch that shadow: it is the intentional visual/editor handoff.
            if (root.deviceConnected) {
                root.mirrorAcceptedDeviceTogglesToOffline()
                if (root.fileBridge)
                    root.fileBridge.editTracking = false
            }
            if (root.activeDeviceSlot >= 0)
                root.selectedDeviceSlot = root.activeDeviceSlot
        }
        function onUseInitVolumeChanged() {
            root.mirrorAcceptedDeviceTogglesToOffline()
        }
        function onAdjMannerVrOffChanged() {
            root.mirrorAcceptedDeviceTogglesToOffline()
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
            if (root.fileBridge) {
                if (root.fileBridge.saveFile(selectedFile)) {
                    root.pcSaveNotice = "Saved: " + String(root.fileBridge.sourceName)
                    pcSaveNoticeTimer.restart()
                } else {
                    root.pcSaveNotice = String(root.fileBridge.lastError || "Save As failed")
                    pcSaveNoticeTimer.restart()
                }
            }
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
                        Text { anchors.left:parent.left;anchors.leftMargin:12;anchors.verticalCenter:parent.verticalCenter;text:"PC PRESET LIBRARY";color:Theme.text;font.family:Theme.monoFamily;font.pixelSize:Theme.rackHeaderSize;font.weight:Font.DemiBold;font.letterSpacing:Theme.rackHeaderTracking }
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
                                        font.pixelSize: Theme.systemListTitleSize
                                        font.weight: Font.DemiBold
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
                                    // SYSTEM_PRESET_NAME_ONLY_V9 — one readable line,
                                    // no generic marketing subtitle per preset.
                                    height: 34
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
                                            font.pixelSize: Theme.systemCaptionSize
                                            font.weight: Font.DemiBold
                                        }
                                        Text {
                                            Layout.fillWidth: true
                                            text: String(modelData.displayName || modelData.fileName || "K500 PRESET")
                                            color: validPreset ? Theme.text : Theme.textDim
                                            font.family: Theme.monoFamily
                                            font.pixelSize: Theme.systemListTitleSize + 1
                                            font.weight: Font.DemiBold
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            text: loadedPreset ? "STAGED" : validPreset ? "" : "INVALID"
                                            color: loadedPreset ? Theme.accent : validPreset ? Theme.textSoft : Theme.amber
                                            font.family: Theme.monoFamily
                                            font.pixelSize: Theme.systemListTitleSize
                                            font.weight: Font.DemiBold
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
                                    font.pixelSize: Theme.systemListTitleSize
                                    font.weight: Font.DemiBold
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
                            // SYSTEM_QUIET_STATUS_V9 — always show an error or
                            // transient action result, never permanent tutorials.
                            text: root.pcSaveNotice.length > 0 ? root.pcSaveNotice
                                  : (root.fileBridge && String(root.fileBridge.lastError || "").length > 0
                                     ? String(root.fileBridge.lastError) : "")
                            color: root.pcSaveNotice.length > 0 || root.fileBridge && String(root.fileBridge.lastError || "").length > 0 ? Theme.amber : Theme.textDim
                            font.family: Theme.monoFamily
                            font.pixelSize: 8
                            elide: Text.ElideRight
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            SoftButton { Layout.fillWidth: true; text: "Open file"; compact: true; enabled: !!root.fileBridge; onClicked: openPresetDialog.open() }
                            SoftButton { Layout.fillWidth: true; text: "Save PC As"; compact: true; enabled: !!root.fileBridge; onClicked: root.requestSaveAs() }
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
                        Text{anchors.left:parent.left;anchors.leftMargin:12;anchors.verticalCenter:parent.verticalCenter;text:"DEVICE PRESET SLOTS";color:Theme.text;font.family:Theme.monoFamily;font.pixelSize:Theme.rackHeaderSize;font.weight:Font.DemiBold;font.letterSpacing:Theme.rackHeaderTracking}
                        // SYSTEM_OFFLINE_TRUTH_V7 — retained slot names are not verified
                        // live data after disconnect; C0 alone owns ACTIVE identity.
                        Text{anchors.right:parent.right;anchors.rightMargin:12;anchors.verticalCenter:parent.verticalCenter;text:root.deviceConnected?"":"OFFLINE";color:root.deviceConnected?Theme.textSoft:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:Theme.systemCaptionSize;font.weight:Font.Medium}
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
                                        // SYSTEM_SLOT_SCOPE_VISUAL_V2B — selected is cyan;
                                        // only confirmed live hardware ACTIVE is amber.
                                        color:selected?Theme.systemSelectedSurface:active?Theme.systemActiveSurface:"#0D1115"
                                        border.width:1
                                        border.color:active?Theme.amber:selected?Theme.accentSoft:"#252D34"
                                        Rectangle {
                                            anchors.left:parent.left
                                            anchors.verticalCenter:parent.verticalCenter
                                            width:3;height:Math.max(14,parent.height-12);radius:1.5
                                            visible:active||selected
                                            color:active?Theme.amber:Theme.accent
                                        }
                                        RowLayout {
                                            anchors.fill:parent;anchors.leftMargin:9;anchors.rightMargin:9;spacing:7
                                            Text{text:String(index+1).padStart(2,"0");color:active?Theme.amber:Theme.textSoft;font.family:Theme.monoFamily;font.pixelSize:Theme.systemListTitleSize;font.weight:Font.DemiBold}
                                            Text{Layout.fillWidth:true;text:modelData;color:Theme.text;font.family:Theme.monoFamily;font.pixelSize:Theme.systemListTitleSize;font.weight:Font.DemiBold;elide:Text.ElideRight}
                                            Text{text:active?"ACTIVE":"";color:Theme.amber;font.family:Theme.monoFamily;font.pixelSize:Theme.systemStatusSize;font.weight:Font.DemiBold}
                                        }
                                        MouseArea{anchors.fill:parent;cursorShape:Qt.PointingHandCursor;enabled:!root.presetManager||!root.presetManager.busy;onClicked:root.selectedDeviceSlot=index}
                                    }
                                }
                            }
                        }

                        // Native KTV parity: selected hardware Mode Name.
                        // Captured transaction = patch active slot image 0x0280..0x028F,
                        // then native Store 0x41/0x42/0x43 and Recall/readback verify.
                        RowLayout {
                            Layout.fillWidth:true
                            spacing:7
                            Text {
                                text:"MODE NAME"
                                color:Theme.textDim
                                font.family:Theme.monoFamily
                                font.pixelSize:Theme.systemListTitleSize
                                font.letterSpacing:.3
                                Layout.preferredWidth:74
                            }
                            Rectangle {
                                Layout.fillWidth:true
                                Layout.preferredHeight:29
                                radius:6
                                color:"#080C10"
                                border.width:1
                                border.color:root.deviceConnected?Theme.borderSoft:"#20272D"
                                // SYSTEM_MODE_NAME_SINGLE_SURFACE_V7 — no retained
                                // mode draft behind the offline caption.
                                TextInput {
                                    id: modeNameInput
                                    visible:root.deviceConnected
                                    anchors.fill:parent
                                    anchors.leftMargin:9
                                    anchors.rightMargin:9
                                    verticalAlignment:TextInput.AlignVCenter
                                    text:root.modeNameDraft
                                    readOnly:!root.deviceConnected
                                             || !root.presetManager
                                             || !root.presetManager.usbStoreAvailable
                                             || root.presetManager.busy
                                             || root.selectedDeviceSlot !== root.activeDeviceSlot
                                    selectByMouse:true
                                    maximumLength:16
                                    color:readOnly?Theme.textDim:Theme.amber
                                    selectionColor:Theme.accentSoft
                                    font.family:Theme.monoFamily
                                    font.pixelSize:Theme.systemListTitleSize
                                    font.weight:Font.DemiBold
                                    clip:true
                                    onTextEdited:root.modeNameDraft=text
                                }
                                Text {
                                    anchors.left:parent.left
                                    anchors.leftMargin:9
                                    anchors.verticalCenter:parent.verticalCenter
                                    visible:!root.deviceConnected
                                    text:"Connect K500 to read"
                                    color:Theme.textDim
                                    font.family:Theme.monoFamily
                                    font.pixelSize:Theme.systemListTitleSize
                                    font.weight:Font.Medium
                                }
                            }
                            SoftButton {
                                Layout.preferredWidth:64
                                text:root.presetManager&&root.presetManager.storeBusy?"Saving…":"Rename"
                                compact:true
                                enabled:root.deviceModeRenameReady
                                onClicked:root.presetManager.renameActiveMode(root.normalizedModeNameDraft())
                            }
                        }

                        // SYSTEM_DEVICE_MODE_INIT_TOGGLE_V4
                        // SYSTEM_CALM_TOGGLE_COPY_V1 — normal state is conveyed
                        // by the switch itself; copy appears only for transient work.
                        SystemToggleRow {
                            Layout.fillWidth:true
                            Layout.preferredHeight:48
                            title:"Use Init Vol"
                            detail:root.deviceConnected
                                   ? "Startup volume"
                                   : "Device-only setting"
                            iconName:"settings-2"
                            checked:root.deviceConnected
                                    ? (!!root.presetManager && root.presetManager.useInitVolume)
                                    : root.offlineUseInitVolume
                            available:!root.deviceConnected
                                      || (!!root.deviceManager && root.deviceManager.status === "connected")
                            pending:root.deviceConnected
                                    && !!root.presetManager
                                    && root.presetManager.useInitBusy
                            statusText:root.systemControlHintTarget === "init" ? root.systemControlHint
                                       : root.deviceConnected
                                         && !root.presetManager.useInitVolumeKnown ? "SYNCING…" : ""
                            statusColor:Theme.amber
                            onToggleRequested:root.requestUseInitVolumeToggle()
                            onBlockedClicked:root.blockedOnlineToggleHint("init")
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
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 355
                spacing: 8

                // P2_SYSTEM_READONLY_AFFORDANCE_V1
                // These native-app surfaces are display-only until donor captures
                // prove the corresponding rename/reset/credential write traffic.
                // Never render enabled controls for an operation we cannot perform.
                StudioPanel {
                    Layout.fillWidth:true
                    Layout.preferredHeight:196
                    accentTop:false
                    ColumnLayout {
                        anchors.fill:parent
                        spacing:0
                        Item {
                            Layout.fillWidth:true
                            Layout.preferredHeight:35
                            Text{anchors.left:parent.left;anchors.leftMargin:12;anchors.verticalCenter:parent.verticalCenter;text:"BT / BLE IDENTITY";color:Theme.text;font.family:Theme.monoFamily;font.pixelSize:Theme.rackHeaderSize;font.weight:Font.DemiBold;font.letterSpacing:Theme.rackHeaderTracking}
                            Rectangle{anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:1;color:Theme.borderSoft}
                        }
                        ColumnLayout {
                            Layout.fillWidth:true
                            Layout.fillHeight:true
                            Layout.margins:12
                            spacing:7
                            RowLayout {
                                Layout.fillWidth:true
                                Text{text:"BT NAME";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:Theme.systemCaptionSize;font.letterSpacing:1.1}
                                Item{Layout.fillWidth:true}
                            }
                            RowLayout {
                                Layout.fillWidth:true
                                spacing:6
                                Rectangle {
                                    Layout.fillWidth:true
                                    Layout.preferredHeight:29
                                    radius:6
                                    color:"#080C10"
                                    border.width:1
                                    border.color:root.btNameWriteReady?Theme.accentSoft:Theme.borderSoft
                                    // SYSTEM_BT_NAME_SINGLE_SURFACE_V7 — connected
                                    // editor and offline help never overlap.
                                    TextInput{
                                        visible:root.deviceConnected
                                        anchors.fill:parent
                                        anchors.leftMargin:9
                                        anchors.rightMargin:9
                                        verticalAlignment:TextInput.AlignVCenter
                                        // BT_IDENTITY_DISPLAY_FULL_V1 — hardware readback
                                        // is a 0x13-byte identity field. Do not truncate
                                        // KTV_BT_00AB12-style device names just because
                                        // CMD 0x4E SET itself accepts only 1..8 ASCII.
                                        text:root.btNameDraft
                                        maximumLength:19
                                        readOnly:!root.deviceConnected||!root.presetManager||root.presetManager.busy
                                        selectByMouse:true
                                        color:readOnly?Theme.textDim:Theme.amber
                                        font.family:Theme.monoFamily
                                        font.pixelSize:10
                                        font.weight:Font.Bold
                                        onTextEdited:root.btNameDraft=text
                                    }
                                    Text {
                                        visible:!root.deviceConnected
                                        anchors.left:parent.left
                                        anchors.leftMargin:9
                                        anchors.verticalCenter:parent.verticalCenter
                                        text:"Connect K500 to read"
                                        color:Theme.textDim
                                        font.family:Theme.monoFamily
                                        font.pixelSize:Theme.systemListTitleSize
                                        font.weight:Font.Medium
                                    }
                                }
                                SoftButton{
                                    Layout.preferredWidth:58
                                    text:"Rename"
                                    compact:true
                                    enabled:root.btNameWriteReady
                                    onClicked:root.presetManager.setBtName(root.normalizedBtNameDraft())
                                    // BT write limit differs from full hardware
                                    // readback name length; never change validation.
                                    HoverHandler { id: btRenameHelp }
                                    ToolTip.visible: btRenameHelp.hovered
                                    ToolTip.delay: 500
                                    ToolTip.text: "Rename: 1–8 ASCII characters. BLE name is read-only."
                                }
                                SoftButton{
                                    Layout.preferredWidth:48
                                    text:"Reset"
                                    compact:true
                                    enabled:root.deviceConnected&&root.presetManager&&root.presetManager.usbStoreAvailable&&!root.presetManager.busy
                                    onClicked:root.presetManager.resetBtName()
                                }
                            }
                            Text{text:"BLE NAME";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:Theme.systemCaptionSize;font.letterSpacing:1.1}
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
                            Item{Layout.fillHeight:true}
                        }
                    }
                }

                StudioPanel {
                    Layout.fillWidth:true
                    Layout.preferredHeight:176
                    accentTop:false
                    ColumnLayout {
                        anchors.fill:parent
                        spacing:0
                        Item {
                            Layout.fillWidth:true
                            Layout.preferredHeight:35
                            Text{anchors.left:parent.left;anchors.leftMargin:12;anchors.verticalCenter:parent.verticalCenter;text:"LOCK / ADMIN";color:Theme.text;font.family:Theme.monoFamily;font.pixelSize:Theme.rackHeaderSize;font.weight:Font.DemiBold;font.letterSpacing:Theme.rackHeaderTracking}
                            Text{anchors.right:parent.right;anchors.rightMargin:12;anchors.verticalCenter:parent.verticalCenter;text:"READ ONLY";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:Theme.systemCaptionSize;font.weight:Font.Medium}
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
                                    Text{text:"LOCK KEY";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:Theme.systemCaptionSize;font.letterSpacing:1.0}
                                    Rectangle {
                                        Layout.fillWidth:true
                                        Layout.preferredHeight:29
                                        radius:6
                                        color:"#080C10"
                                        border.width:1
                                        border.color:Theme.borderSoft
                                        Text{anchors.left:parent.left;anchors.leftMargin:9;anchors.verticalCenter:parent.verticalCenter;text:root.deviceConnected ? "NOT EXPOSED" : "OFFLINE";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:Theme.systemCaptionSize;font.weight:Font.DemiBold}
                                    }
                                }
                                ColumnLayout {
                                    Layout.fillWidth:true
                                    spacing:4
                                    Text{text:"ADMIN";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:Theme.systemCaptionSize;font.letterSpacing:1.0}
                                    Rectangle {
                                        Layout.fillWidth:true
                                        Layout.preferredHeight:29
                                        radius:6
                                        color:"#080C10"
                                        border.width:1
                                        border.color:Theme.borderSoft
                                        Text{anchors.left:parent.left;anchors.leftMargin:9;anchors.verticalCenter:parent.verticalCenter;text:root.deviceConnected ? "NOT EXPOSED" : "OFFLINE";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:Theme.systemCaptionSize;font.weight:Font.DemiBold}
                                    }
                                }
                            }
                            Item{Layout.fillHeight:true}
                        }
                    }
                }

                // SYSTEM_MANUAL_ADJUSTMENT_CARD_V2
                StudioPanel {
                    Layout.fillWidth:true
                    Layout.fillHeight:true
                    Layout.minimumHeight:96
                    accentTop:false

                    ColumnLayout {
                        anchors.fill:parent
                        spacing:0

                        Item {
                            Layout.fillWidth:true
                            Layout.preferredHeight:33
                            Text {
                                anchors.left:parent.left
                                anchors.leftMargin:12
                                anchors.verticalCenter:parent.verticalCenter
                                text:"MANUAL ADJUSTMENT"
                                color:Theme.text
                                font.family:Theme.monoFamily
                                font.pixelSize:Theme.rackHeaderSize
                                font.weight:Font.DemiBold
                                font.letterSpacing:Theme.rackHeaderTracking
                            }
                            Rectangle{anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:1;color:Theme.borderSoft}
                        }

                        Item {
                            Layout.fillWidth:true
                            Layout.fillHeight:true

                            SystemToggleRow {
                                anchors.left:parent.left
                                anchors.right:parent.right
                                anchors.leftMargin:10
                                anchors.rightMargin:10
                                anchors.verticalCenter:parent.verticalCenter
                                height:50
                                title:"VR / Trim Pot Off"
                                detail:!root.deviceConnected && root.offlineEditMode
                                       ? "Saved with preset"
                                       : "Disable front-panel adjustment"
                                iconName:"sliders-horizontal"
                                checked:root.deviceConnected
                                        ? (!!root.presetManager && root.presetManager.adjMannerVrOff)
                                        : root.offlineAdjMannerVrOff
                                available:!root.deviceConnected
                                          || (!!root.deviceManager && root.deviceManager.status === "connected")
                                pending:root.deviceConnected
                                        && !!root.presetManager
                                        && root.presetManager.adjMannerBusy
                                statusText:root.systemControlHintTarget === "vr" ? root.systemControlHint
                                           : root.deviceConnected
                                             && !root.presetManager.adjMannerVrOffKnown ? "SYNCING…" : ""
                                statusColor:Theme.amber
                                onToggleRequested:root.requestAdjMannerVrToggle()
                                onBlockedClicked:root.blockedOnlineToggleHint("vr")
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
                channels:root.startupLimitChannels
                valueResolver:root.startupLimitValue
                interactionEnabled:root.deviceConnected && root.engine.deviceStateReady
            }

            StudioPanel {
                Layout.fillWidth:true
                Layout.fillHeight:true
                Layout.preferredWidth:432
                accentTop:false
                ColumnLayout {
                    anchors.fill:parent
                    spacing:0
                    Item{Layout.fillWidth:true;Layout.preferredHeight:35;Text{anchors.left:parent.left;anchors.leftMargin:12;anchors.verticalCenter:parent.verticalCenter;text:"RECORDING / MIC TRIGGER";color:Theme.text;font.family:Theme.monoFamily;font.pixelSize:Theme.rackHeaderSize;font.weight:Font.DemiBold;font.letterSpacing:Theme.rackHeaderTracking}Rectangle{anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:1;color:Theme.borderSoft}}
                    RowLayout {
                        Layout.fillWidth:true;Layout.fillHeight:true;Layout.margins:10;spacing:10
                        Rectangle {
                            Layout.fillWidth:true;Layout.fillHeight:true;radius:10;color:"#151B21";border.width:1;border.color:Theme.borderSoft
                            ColumnLayout {
                                anchors.fill:parent;anchors.margins:8;spacing:5
                                Text{text:"RECORDING";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:Theme.systemCaptionSize;font.letterSpacing:1.1}
                                RowLayout {
                                    Layout.fillWidth:true;Layout.fillHeight:true;spacing:7
                                    Repeater {
                                        model: root.recordingChannels.length
                                        delegate:ColumnLayout {
                                            id: recChannel
                                            required property int index
                                            readonly property var modelData: root.recordingChannels[index]
                                            property real localValue: root.stableSystemValue(modelData)
                                            // SYSTEM_DEFERRED_AUTHORITATIVE_SYNC_V1 — preserve the
                                            // active gesture, but never lose a newer device snapshot.
                                            property bool deferredModelSync: false
                                            readonly property bool channelEditable: root.deviceConnected && root.engine.deviceStateReady
                                            onModelDataChanged: {
                                                if (recFader && recFader.dragging) {
                                                    deferredModelSync = true
                                                } else {
                                                    localValue = root.stableSystemValue(modelData)
                                                    deferredModelSync = false
                                                }
                                            }
                                            Connections {
                                                target: root.engine
                                                function onDeviceStateChanged() {
                                                    if (recFader.dragging) {
                                                        recChannel.deferredModelSync = true
                                                    } else {
                                                        recChannel.localValue = root.stableSystemValue(recChannel.modelData)
                                                        recChannel.deferredModelSync = false
                                                    }
                                                }
                                            }
                                            Layout.fillWidth:true;Layout.fillHeight:true;spacing:3
                                            Text{Layout.alignment:Qt.AlignHCenter;text:modelData.label+" · "+modelData.badge;color:recFader.highlighted?recFader.accentColor:Theme.textSoft;font.family:Theme.monoFamily;font.pixelSize:Theme.systemCaptionSize;font.weight:recFader.highlighted?Font.DemiBold:Font.Medium;Behavior on color{ColorAnimation{duration:75}}}
                                            StudioFader{
                                                id:recFader
                                                Layout.fillHeight:true;Layout.preferredWidth:48;Layout.alignment:Qt.AlignHCenter
                                                enabled:recChannel.channelEditable
                                                opacity:enabled?1.0:0.52
                                                value:recChannel.localValue;from:modelData.from;to:modelData.to;step:1;defaultValue:root.stableSystemValue(modelData)
                                                onDraggingChanged:{
                                                    if(!dragging && recChannel.deferredModelSync){
                                                        recChannel.localValue=root.stableSystemValue(recChannel.modelData)
                                                        recChannel.deferredModelSync=false
                                                    }
                                                }
                                                onValueEdited:function(v){
                                                    recChannel.localValue=v
                                                    if(recChannel.channelEditable&&String(modelData.path||"").length>0)
                                                        root.engine.editDevicePath(String(modelData.path),v)
                                                }
                                            }
                                            Rectangle{Layout.alignment:Qt.AlignHCenter;Layout.preferredWidth:48;Layout.preferredHeight:23;radius:8;color:"#080C10";border.width:1;border.color:recFader.highlighted?recFader.accentColor:"#050708";Behavior on border.color{ColorAnimation{duration:75}}Text{anchors.centerIn:parent;text:recChannel.localValue;color:recChannel.channelEditable?Theme.amber:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:9;font.weight:Font.Bold}}
                                        }
                                    }
                                }
                            }
                        }
                        Rectangle {
                            Layout.fillWidth:true;Layout.fillHeight:true;radius:10;color:"#151B21";border.width:1;border.color:Theme.borderSoft
                            ColumnLayout {
                                anchors.fill:parent;anchors.margins:8;spacing:5
                                Text{text:"MIC TRIGGER";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:Theme.systemCaptionSize;font.letterSpacing:1.1}
                                RowLayout {
                                    Layout.fillWidth:true;Layout.fillHeight:true;spacing:7
                                    Repeater {
                                        model: root.micTriggerChannels.length
                                        delegate:ColumnLayout {
                                            id: triggerChannel
                                            required property int index
                                            readonly property var modelData: root.micTriggerChannels[index]
                                            property real localValue: root.stableSystemValue(modelData)
                                            property bool deferredModelSync: false
                                            readonly property bool channelEditable:root.deviceConnected
                                                                 && root.engine.deviceStateReady
                                                                 && Boolean(root.systemValue("danceMicTriggerKnown",false))
                                            onModelDataChanged: {
                                                if (triggerFader && triggerFader.dragging) {
                                                    deferredModelSync = true
                                                } else {
                                                    localValue = root.stableSystemValue(modelData)
                                                    deferredModelSync = false
                                                }
                                            }
                                            Connections {
                                                target: root.engine
                                                function onDeviceStateChanged() {
                                                    if (triggerFader.dragging) {
                                                        triggerChannel.deferredModelSync = true
                                                    } else {
                                                        triggerChannel.localValue = root.stableSystemValue(triggerChannel.modelData)
                                                        triggerChannel.deferredModelSync = false
                                                    }
                                                }
                                            }
                                            Layout.fillWidth:true;Layout.fillHeight:true;spacing:3
                                            Text{Layout.alignment:Qt.AlignHCenter;text:modelData.label;color:triggerFader.highlighted?triggerFader.accentColor:Theme.textSoft;font.family:Theme.monoFamily;font.pixelSize:Theme.systemCaptionSize;font.weight:triggerFader.highlighted?Font.DemiBold:Font.Medium;Behavior on color{ColorAnimation{duration:75}}}
                                            StudioFader{
                                                id:triggerFader
                                                Layout.fillHeight:true;Layout.preferredWidth:48;Layout.alignment:Qt.AlignHCenter
                                                enabled:triggerChannel.channelEditable
                                                opacity:enabled?1.0:0.52
                                                value:triggerChannel.localValue;from:modelData.from;to:modelData.to;step:1;defaultValue:root.stableSystemValue(modelData)
                                                onDraggingChanged:{
                                                    if(!dragging && triggerChannel.deferredModelSync){
                                                        triggerChannel.localValue=root.stableSystemValue(triggerChannel.modelData)
                                                        triggerChannel.deferredModelSync=false
                                                    }
                                                }
                                                onValueEdited:function(v){
                                                    triggerChannel.localValue=v
                                                    if(triggerChannel.channelEditable)
                                                        root.engine.editDevicePath(String(modelData.path),v)
                                                }
                                            }
                                            Rectangle{Layout.alignment:Qt.AlignHCenter;Layout.preferredWidth:52;Layout.preferredHeight:23;radius:8;color:"#080C10";border.width:1;border.color:triggerFader.highlighted?triggerFader.accentColor:"#050708";Behavior on border.color{ColorAnimation{duration:75}}Row{anchors.centerIn:parent;spacing:3;Text{text:triggerChannel.localValue;color:triggerChannel.channelEditable?Theme.amber:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:9;font.weight:Font.Bold}Text{text:modelData.unit;color:triggerFader.highlighted?Theme.textSoft:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:7;anchors.baseline:parent.children[0].baseline;Behavior on color{ColorAnimation{duration:75}}}}}
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
