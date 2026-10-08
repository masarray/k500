import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Dialogs

Window {
    id: root

    required property var fileBridge
    required property var presetManager

    width: 960
    height: 590
    minimumWidth: 800
    minimumHeight: 520
    modality: Qt.ApplicationModal
    flags: Qt.Dialog
    color: Theme.background
    title: "Mass Upload Presets"
    visible: false

    // MASS_UPLOAD_FIRST_FRAME_GATE_V1
    // A native Windows dialog can become compositor-visible before Qt Quick has
    // submitted its first dark scene-graph frame. Keep the HWND transparent for
    // a short first-frame warmup, then reveal it atomically. This removes the
    // default white client-area flash without changing transfer/preset behavior.
    opacity: 0.0

    Timer {
        id: firstFrameRevealTimer
        interval: 50
        repeat: false
        onTriggered: {
            if (!root.visible)
                return
            root.opacity = 1.0
            root.requestActivate()
        }
    }

    onVisibleChanged: {
        if (!visible) {
            firstFrameRevealTimer.stop()
            opacity = 0.0
        }
    }

    property int sourceIndex: -1
    property int sourceAnchor: -1
    // MASS_UPLOAD_EXTENDED_SELECTION_V1 — source selection never changes device slots.
    // Copy-on-write preserves QML delegate bindings while Ctrl toggles / Shift extends.
    property var selectedSourceIndexes: []
    property int targetIndex: -1
    readonly property int maxSlots: 10
    // MASS_UPLOAD_ACK_PROGRESS_OVERLAY_V1 — modal visual feedback follows
    // native ACK/verified readback, never a timer-driven fictional percentage.
    property bool transferInFlight: false
    property bool transferSucceeded: false
    property string transferError: ""
    readonly property int verifiedPercent: root.presetManager
                                           ? Number(root.presetManager.massUploadProgressPercent || 0) : 0
    onClosing: function(close) {
        // Abort/close during CMD 0x41/0x42/0x43 may leave device slots partial.
        // There is deliberately no mid-store "Cancel" operation.
        if (root.transferInFlight)
            close.accepted = false
    }
    Connections {
        target: root.presetManager
        enabled: !!root.presetManager
        function onOperationCompleted(kind, slot) {
            if (kind !== "Mass Upload" || !root.transferInFlight)
                return
            root.transferInFlight = false
            root.transferSucceeded = true
        }
        function onOperationFailed(kind, message) {
            if (kind !== "Mass Upload" || !root.transferInFlight)
                return
            root.transferError = String(message)
            root.transferInFlight = false
        }
        function onConnectedChanged() {
            if (root.transferInFlight && !root.presetManager.connected) {
                root.transferError = "Connection lost. Upload was interrupted; reconnect and verify device slots before retrying."
                root.transferInFlight = false
            }
        }
    }
    // Unified source replaces the old folderPresets-only Mass Upload source.
    // folderPresets remains part of combinedPresets together with SONKUPIK official presets.
    readonly property var sourcePresets: root.fileBridge ? root.fileBridge.combinedPresets : []

    ListModel { id: targetModel }

    function clearSourceSelection() {
        sourceIndex = -1
        sourceAnchor = -1
        selectedSourceIndexes = []
    }

    onSourcePresetsChanged: clearSourceSelection()

    function isSourceSelected(index) {
        return selectedSourceIndexes.indexOf(index) >= 0
    }

    function selectSource(index, modifiers) {
        var presets = root.sourcePresets
        if (index < 0 || index >= presets.length || !Boolean(presets[index].valid))
            return
        var ctrl = (modifiers & Qt.ControlModifier) !== 0
        var shift = (modifiers & Qt.ShiftModifier) !== 0
        var next = (ctrl || shift) ? selectedSourceIndexes.slice() : []
        if (shift && sourceAnchor >= 0) {
            if (!ctrl)
                next = []
            for (var i = Math.min(sourceAnchor, index); i <= Math.max(sourceAnchor, index); ++i) {
                if (Boolean(presets[i].valid) && next.indexOf(i) < 0)
                    next.push(i)
            }
        } else if (ctrl) {
            var existing = next.indexOf(index)
            if (existing >= 0)
                next.splice(existing, 1)
            else
                next.push(index)
            sourceAnchor = index
        } else {
            next = [index]
            sourceAnchor = index
        }
        next.sort(function(a, b) { return a - b })
        sourceIndex = index
        selectedSourceIndexes = next
    }

    function openTransfer() {
        transferInFlight = false
        transferSucceeded = false
        transferError = ""
        clearSourceSelection()
        targetIndex = -1
        targetModel.clear()
        if (fileBridge) {
            fileBridge.refreshPresetFolder()
            fileBridge.syncOfficialPresets()
        }
        // Show only to the renderer/compositor while fully transparent. The
        // reveal timer fires after the first dark scene has had time to submit.
        opacity = 0.0
        visible = true
        firstFrameRevealTimer.restart()
    }

    function targetContains(path) {
        for (var i = 0; i < targetModel.count; ++i) {
            if (String(targetModel.get(i).path) === String(path))
                return true
        }
        return false
    }

    function addEntry(entry) {
        if (!entry || !Boolean(entry.valid) || targetModel.count >= maxSlots)
            return
        var path = String(entry.path || "")
        if (path.length === 0 || targetContains(path))
            return
        targetModel.append({
            fileName: String(entry.fileName || "preset.k500"),
            displayName: String(entry.displayName || entry.presetName || entry.fileName || "K500 PRESET"),
            presetName: String(entry.presetName || ""),
            originLabel: String(entry.originLabel || (entry.source === "folder" ? "LOCAL" : "SONKUPIK")),
            path: path
        })
        targetIndex = targetModel.count - 1
    }

    function addSelected() {
        var source = root.sourcePresets
        // Deterministic order, valid-only, no duplicates; never exceed 10 device slots.
        var selected = selectedSourceIndexes.slice().sort(function(a, b) { return a - b })
        for (var i = 0; i < selected.length && targetModel.count < maxSlots; ++i) {
            var index = selected[i]
            if (index >= 0 && index < source.length)
                addEntry(source[index])
        }
    }

    function addAll() {
        var source = root.sourcePresets
        for (var i = 0; i < source.length && targetModel.count < maxSlots; ++i)
            addEntry(source[i])
    }

    function removeSelected() {
        if (targetIndex < 0 || targetIndex >= targetModel.count)
            return
        targetModel.remove(targetIndex)
        targetIndex = targetModel.count === 0 ? -1 : Math.min(targetIndex, targetModel.count - 1)
    }

    function clearTarget() {
        targetModel.clear()
        targetIndex = -1
    }

    function uploadTransfer() {
        if (!fileBridge || !presetManager || targetModel.count === 0)
            return
        var paths = []
        for (var i = 0; i < targetModel.count; ++i)
            paths.push(String(targetModel.get(i).path))
        var entries = fileBridge.buildTransferUploadEntries(paths)
        if (entries && entries.length > 0) {
            // Keep the native modal open; the overlay owns progress from
            // successful Store ACKs, until final 939-byte readback confirms 100%.
            transferSucceeded = false
            transferError = ""
            transferInFlight = true
            presetManager.massUploadSlotImages(entries)
        }
    }

    FolderDialog {
        id: folderDialog
        title: "Select folder containing your K500 presets"
        onAccepted: {
            if (root.fileBridge) {
                root.fileBridge.setPresetFolder(selectedFolder)
                root.clearSourceSelection()
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.background

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 12

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3
                Text {
                    text: "MASS UPLOAD · PRESET TRANSFER"
                    color: Theme.text
                    font.family: Theme.monoFamily
                    font.pixelSize: 12
                    font.weight: Font.Bold
                    font.letterSpacing: 1.0
                }
                Text {
                    Layout.fillWidth: true
                    text: "Choose presets with Ctrl+click or Shift+click, then Add to map up to 10 K500 Device Slots."
                    color: Theme.textDim
                    font.family: Theme.fontFamily
                    font.pixelSize: 10
                    wrapMode: Text.WordWrap
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 12

                StudioPanel {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 410
                    accentTop: false

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        Item {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 36
                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: 12
                                anchors.verticalCenter: parent.verticalCenter
                                text: "PC PRESET COLLECTION"
                                color: Theme.text
                                font.family: Theme.monoFamily
                                font.pixelSize: 9
                                font.weight: Font.Bold
                                font.letterSpacing: 0.9
                            }
                            Text {
                                anchors.right: parent.right
                                anchors.rightMargin: 12
                                anchors.verticalCenter: parent.verticalCenter
                                text: String(root.sourcePresets.length) + " PRESETS" + (root.selectedSourceIndexes.length ? "  ·  " + root.selectedSourceIndexes.length + " SELECTED" : "")
                                color: Theme.accent
                                font.family: Theme.monoFamily
                                font.pixelSize: 9
                                font.weight: Font.Bold
                            }
                            Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: 1; color: Theme.borderSoft }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.margins: 10
                            spacing: 7

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 6
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 1
                                    Text {
                                        Layout.fillWidth: true
                                        text: root.fileBridge ? String(root.fileBridge.officialSyncStatus || "SonKuPik presets ready") : "SonKuPik presets ready"
                                        color: root.fileBridge && String(root.fileBridge.officialSyncError || "").length > 0 ? Theme.amber : Theme.textSoft
                                        font.family: Theme.monoFamily
                                        font.pixelSize: 9
                                        elide: Text.ElideRight
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        text: root.fileBridge && String(root.fileBridge.presetFolder || "").length > 0
                                              ? ("LOCAL · " + String(root.fileBridge.presetFolder))
                                              : "LOCAL · choose a folder for your own presets"
                                        color: Theme.textSoft
                                        font.family: Theme.monoFamily
                                        font.pixelSize: 9
                                        elide: Text.ElideMiddle
                                    }
                                }
                                SoftButton {
                                    Layout.preferredWidth: 64
                                    text: "Folder"
                                    compact: true
                                    enabled: !!root.fileBridge
                                    onClicked: folderDialog.open()
                                }
                                SoftButton {
                                    Layout.preferredWidth: 64
                                    text: root.fileBridge && root.fileBridge.officialSyncBusy ? "Sync…" : "Sync"
                                    compact: true
                                    enabled: !!root.fileBridge && !root.fileBridge.officialSyncBusy
                                    onClicked: root.fileBridge.syncOfficialPresets()
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                radius: 8
                                color: "#090D11"
                                border.width: 1
                                border.color: "#050708"
                                clip: true

                                ListView {
                                    id: sourceList
                                    anchors.fill: parent
                                    anchors.margins: 6
                                    spacing: 3
                                    clip: true
                                    model: root.sourcePresets

                                    delegate: Rectangle {
                                        required property int index
                                        required property var modelData
                                        width: sourceList.width
                                        height: 38
                                        radius: 6
                                        readonly property bool validPreset: Boolean(modelData.valid)
                                        readonly property bool selected: root.isSourceSelected(index)
                                        readonly property string origin: String(modelData.originLabel || (modelData.source === "folder" ? "LOCAL" : "SONKUPIK"))
                                        // MASS_UPLOAD_CANONICAL_BADGE_V1 — stable preset source identity.
                                        // Not a row index and never a destination K500 hardware slot.
                                        readonly property int catalogNumber: Number(modelData.catalogNumber || 0)
                                        readonly property bool numberedOfficial: origin === "SONKUPIK" && catalogNumber >= 1 && catalogNumber <= 20
                                        readonly property string badgeText: numberedOfficial
                                                                             ? String(catalogNumber).padStart(2, "0")
                                                                             : origin === "LOCAL" ? "LOCAL" : "OFFICIAL"
                                        color: selected ? "#15252A" : sourceMouse.containsMouse ? "#12181D" : "#0D1115"
                                        border.width: 1
                                        border.color: selected ? Theme.accentSoft : validPreset ? "#252D34" : "#553A32"

                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.leftMargin: 8
                                            anchors.rightMargin: 8
                                            spacing: 7
                                            Rectangle {
                                                Layout.preferredWidth: numberedOfficial ? 32 : badgeText === "LOCAL" ? 46 : 62
                                                Layout.preferredHeight: 22
                                                radius: 5
                                                color: numberedOfficial ? "#182B30" : "#171B20"
                                                border.width: 1
                                                border.color: numberedOfficial ? Theme.accentSoft : Theme.borderSoft
                                                Text {
                                                    anchors.centerIn: parent
                                                    text: badgeText
                                                    color: numberedOfficial ? Theme.accent : Theme.textSoft
                                                    font.family: Theme.monoFamily
                                                    font.pixelSize: numberedOfficial ? 11 : 9
                                                    font.weight: Font.Bold
                                                }
                                            }
                                            ColumnLayout {
                                                Layout.fillWidth: true
                                                spacing: -1
                                                Text {
                                                    Layout.fillWidth: true
                                                    text: String(modelData.displayName || modelData.presetName || modelData.fileName || "K500 PRESET")
                                                    color: validPreset ? Theme.text : Theme.textDim
                                                    font.family: Theme.monoFamily
                                                    font.pixelSize: 10
                                                    font.weight: Font.Bold
                                                    elide: Text.ElideRight
                                                }
                                                Text {
                                                    Layout.fillWidth: true
                                                    text: String(modelData.description || modelData.fileName || "")
                                                    color: Theme.textSoft
                                                    font.family: Theme.fontFamily
                                                    font.pixelSize: 9
                                                    elide: Text.ElideRight
                                                }
                                            }
                                            Text {
                                                text: validPreset ? "READY" : "INVALID"
                                                color: validPreset ? Theme.accent : Theme.amber
                                                font.family: Theme.monoFamily
                                                font.pixelSize: 9
                                                font.weight: Font.Bold
                                            }
                                        }

                                        MouseArea {
                                            id: sourceMouse
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: validPreset ? Qt.PointingHandCursor : Qt.ArrowCursor
                                            enabled: validPreset
                                            onClicked: function(mouse) { root.selectSource(index, mouse.modifiers) }
                                            onDoubleClicked: { root.selectSource(index, Qt.NoModifier); root.addSelected() }
                                        }
                                    }

                                    Text {
                                        anchors.centerIn: parent
                                        visible: sourceList.count === 0
                                        width: parent.width - 30
                                        horizontalAlignment: Text.AlignHCenter
                                        text: "SonKuPik presets are available automatically.\nChoose a Local Folder to add your own presets."
                                        color: Theme.textDim
                                        font.family: Theme.monoFamily
                                        font.pixelSize: 9
                                        wrapMode: Text.WordWrap
                                    }
                                }
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.preferredWidth: 92
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 8
                    SoftButton { Layout.fillWidth: true; text: root.selectedSourceIndexes.length > 1 ? "Add (" + root.selectedSourceIndexes.length + ")  >" : "Add  >"; compact: true; enabled: root.selectedSourceIndexes.length > 0 && targetModel.count < root.maxSlots; onClicked: root.addSelected() }
                    SoftButton { Layout.fillWidth: true; text: "Add All  >>"; compact: true; enabled: sourceList.count > 0 && targetModel.count < root.maxSlots; onClicked: root.addAll() }
                    Item { Layout.preferredHeight: 12 }
                    SoftButton { Layout.fillWidth: true; text: "<  Remove"; compact: true; enabled: root.targetIndex >= 0; onClicked: root.removeSelected() }
                    SoftButton { Layout.fillWidth: true; text: "<<  Remove All"; compact: true; enabled: targetModel.count > 0; onClicked: root.clearTarget() }
                }

                StudioPanel {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 410
                    accentTop: false

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        Item {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 36
                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: 12
                                anchors.verticalCenter: parent.verticalCenter
                                text: "K500 DEVICE SLOTS"
                                color: Theme.text
                                font.family: Theme.monoFamily
                                font.pixelSize: 9
                                font.weight: Font.Bold
                                font.letterSpacing: 0.9
                            }
                            Text {
                                anchors.right: parent.right
                                anchors.rightMargin: 12
                                anchors.verticalCenter: parent.verticalCenter
                                text: String(targetModel.count) + " / 10"
                                color: targetModel.count >= root.maxSlots ? Theme.amber : Theme.accent
                                font.family: Theme.monoFamily
                                font.pixelSize: 8
                                font.weight: Font.Bold
                            }
                            Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: 1; color: Theme.borderSoft }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.margins: 10
                            radius: 8
                            color: "#090D11"
                            border.width: 1
                            border.color: "#050708"
                            clip: true

                            ListView {
                                id: targetList
                                anchors.fill: parent
                                anchors.margins: 6
                                spacing: 3
                                clip: true
                                model: targetModel

                                delegate: Rectangle {
                                    required property int index
                                    required property string displayName
                                    required property string fileName
                                    required property string path
                                    required property string originLabel
                                    width: targetList.width
                                    height: 38
                                    radius: 6
                                    readonly property bool selected: index === root.targetIndex
                                    color: selected ? "#15252A" : targetMouse.containsMouse ? "#12181D" : "#0D1115"
                                    border.width: 1
                                    border.color: selected ? Theme.accentSoft : "#252D34"

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 8
                                        anchors.rightMargin: 8
                                        spacing: 8
                                        Rectangle {
                                            width: 30
                                            height: 21
                                            radius: 5
                                            color: "#151B20"
                                            border.width: 1
                                            border.color: Theme.borderSoft
                                            Text {
                                                anchors.centerIn: parent
                                                text: String(index + 1).padStart(2, "0")
                                                color: Theme.amber
                                                font.family: Theme.monoFamily
                                                font.pixelSize: 8
                                                font.weight: Font.Bold
                                            }
                                        }
                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: -1
                                            Text {
                                                Layout.fillWidth: true
                                                text: displayName
                                                color: Theme.text
                                                font.family: Theme.monoFamily
                                                font.pixelSize: 9
                                                font.weight: Font.Bold
                                                elide: Text.ElideRight
                                            }
                                            Text {
                                                Layout.fillWidth: true
                                                text: originLabel + " · " + fileName
                                                color: Theme.textDim
                                                font.family: Theme.monoFamily
                                                font.pixelSize: 7
                                                elide: Text.ElideRight
                                            }
                                        }
                                        Text {
                                            text: "SLOT " + String(index + 1).padStart(2, "0")
                                            color: Theme.accent
                                            font.family: Theme.monoFamily
                                            font.pixelSize: 7
                                            font.weight: Font.Bold
                                        }
                                    }

                                    MouseArea {
                                        id: targetMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: root.targetIndex = index
                                        onDoubleClicked: { root.targetIndex = index; root.removeSelected() }
                                    }
                                }

                                Text {
                                    anchors.centerIn: parent
                                    visible: targetModel.count === 0
                                    width: parent.width - 30
                                    horizontalAlignment: Text.AlignHCenter
                                    text: "Add any SonKuPik or Local presets from the left.\nTheir order becomes Device Slot 01…10."
                                    color: Theme.textDim
                                    font.family: Theme.monoFamily
                                    font.pixelSize: 9
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Text {
                    Layout.fillWidth: true
                    text: root.fileBridge && String(root.fileBridge.lastError || "").length > 0
                          ? String(root.fileBridge.lastError)
                          : (root.presetManager && root.presetManager.usbStoreAvailable
                             ? "Ready · upload validates every preset before any hardware write."
                             : "Prepare the list offline; connect K500 via USB to enable final Mass Upload.")
                    color: root.fileBridge && String(root.fileBridge.lastError || "").length > 0 ? Theme.amber : Theme.textDim
                    font.family: Theme.monoFamily
                    font.pixelSize: 8
                    elide: Text.ElideRight
                }
                SoftButton { Layout.preferredWidth: 86; text: "Cancel"; compact: true; onClicked: root.visible = false }
                SoftButton {
                    Layout.preferredWidth: 150
                    text: root.presetManager && root.presetManager.storeBusy ? "Uploading…" : "Upload to K500"
                    compact: true
                    enabled: targetModel.count > 0
                             && !!root.presetManager
                             && root.presetManager.usbStoreAvailable
                             && !root.presetManager.busy
                    onClicked: root.uploadTransfer()
                }
            }
        }

        // Verified-progress transfer overlay; full-screen hit blocker prevents
        // double-submit and edits while non-cancellable native store is active.
        Rectangle {
            id: transferOverlay
            anchors.fill: parent
            z: 100
            visible: root.transferInFlight || root.transferSucceeded || root.transferError.length > 0
            color: "#D9070D12"
            MouseArea { anchors.fill: parent; cursorShape: Qt.ArrowCursor }

            Rectangle {
                width: Math.min(530, parent.width - 52)
                height: 246
                anchors.centerIn: parent
                radius: 14
                color: "#182128"
                border.color: root.transferError.length ? "#D85C69" : root.transferSucceeded ? "#51D9BD" : "#315660"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 11
                    Text {
                        text: root.transferError.length ? "MASS UPLOAD INTERRUPTED"
                              : root.transferSucceeded ? "MASS UPLOAD COMPLETE"
                              : "MASS UPLOAD IN PROGRESS"
                        color: root.transferError.length ? "#FF9AA0" : root.transferSucceeded ? "#69E5C5" : Theme.text
                        font.family: Theme.monoFamily
                        font.pixelSize: 15
                        font.weight: Font.Bold
                        font.letterSpacing: 0.65
                    }
                    Text {
                        Layout.fillWidth: true
                        text: root.transferError.length ? root.transferError
                              : root.transferSucceeded ? "All device writes acknowledged; final Recall and 939-byte verification complete."
                              : root.presetManager ? String(root.presetManager.progress || "Preparing K500 store transaction…") : ""
                        color: Theme.textSoft
                        font.family: Theme.fontFamily
                        font.pixelSize: 11
                        wrapMode: Text.WordWrap
                        Layout.preferredHeight: 39
                        verticalAlignment: Text.AlignVCenter
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: root.transferSucceeded ? "VERIFIED" : root.transferError.length ? "LAST CONFIRMED" : "ACKNOWLEDGED"
                            font.family: Theme.monoFamily
                            font.pixelSize: 10
                            font.weight: Font.Bold
                            color: Theme.textSoft
                        }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: String(root.verifiedPercent) + "%"
                            color: root.transferError.length ? "#FF9AA0" : "#6AE9ED"
                            font.family: Theme.monoFamily
                            font.pixelSize: 24
                            font.weight: Font.Bold
                        }
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 16
                        radius: 8
                        color: "#080D11"
                        border.color: "#35454D"
                        border.width: 1
                        clip: true
                        Rectangle {
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            anchors.left: parent.left
                            anchors.margins: 2
                            radius: 6
                            width: Math.max(0, (parent.width - 4) * Math.max(0, Math.min(100, root.verifiedPercent)) / 100)
                            color: root.transferError.length ? "#D85C69" : root.transferSucceeded ? "#51D9BD" : "#32C9D5"
                            Behavior on width {
                                NumberAnimation { duration: 260; easing.type: Easing.OutCubic }
                            }
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        text: root.transferInFlight
                              ? "Do not unplug K500 or close this window while writing device memory."
                              : root.transferSucceeded ? "Safe to close. Device Slot 01 was recalled and verified."
                              : "Keep LIVE off until K500 is reconnected and its slots are verified."
                        color: root.transferInFlight ? "#E3BF68" : Theme.textDim
                        font.family: Theme.fontFamily
                        font.pixelSize: 10
                        wrapMode: Text.WordWrap
                    }
                    Item { Layout.fillHeight: true }
                    SoftButton {
                        Layout.alignment: Qt.AlignRight
                        Layout.preferredWidth: 120
                        text: root.transferSucceeded ? "Done" : "Close"
                        compact: true
                        visible: !root.transferInFlight
                        onClicked: {
                            root.transferSucceeded = false
                            root.transferError = ""
                            root.visible = false
                        }
                    }
                }
            }
        }
    }
}