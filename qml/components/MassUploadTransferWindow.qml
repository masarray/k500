import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Controls

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
    // MASS_UPLOAD_EXTENDED_SELECTION_V1 — supported Ctrl/Shift input contract.
    // MASS_UPLOAD_SELECTION_ORDER_V2 — chronological selection is the source
    // of truth for Add; catalogue row position is not the hardware slot order.
    // Ctrl appends/removes; Shift walks from anchor toward clicked row, not
    // sorted row-index order. Copy-on-write keeps QML delegate bindings alive.
    property var selectedSourceIndexes: []
    property int targetIndex: -1
    readonly property int maxSlots: 10
    // MASS_TRANSFER_TRUE_MOVE_DND_V1 — targetModel is the ONLY editable plan.
    // Source is a stable-identity catalogue projection, excluding staged paths.
    // Drag gestures never touch files/USB; only Upload freezes ordered paths.
    property int stagedRevision: 0
    property string dragKind: ""          // "source" or "target"
    property int dragIndex: -1
    property real dragStartX: 0
    property real dragStartY: 0
    property real dragX: 0
    property real dragY: 0
    property bool dragActive: false
    property bool dragConsumedClick: false
    property int dragInsertIndex: -1
    property string dragZone: ""
    property string dragLabel: ""
    property var dragSourcePaths: []
    readonly property bool editingLocked: root.transferInFlight
                                          || (root.presetManager && root.presetManager.busy)
    readonly property var catalogPresets: root.fileBridge ? root.fileBridge.combinedPresets : []
    // MASS_UPLOAD_ACK_PROGRESS_OVERLAY_V1 — modal visual feedback follows
    // native ACK/verified readback, never a timer-driven fictional percentage.
    property bool transferInFlight: false
    property bool transferSucceeded: false
    property string transferError: ""
    // OFFLINE_MASS_UPLOAD_CONNECT_GUIDANCE_V1: keep the Upload action
    // discoverable while offline; clicking only shows guidance, never sends IO.
    property string connectNotice: ""
    Timer {
        id: connectNoticeTimer
        interval: 5500
        repeat: false
        onTriggered: root.connectNotice = ""
    }
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
    readonly property var sourcePresets: {
        var revision = root.stagedRevision  // explicit invalidation for ListModel mutations
        var all = root.catalogPresets || []
        return all.filter(function(entry) {
            return !root.targetContains(String(entry.path || ""))
        })
    }

    ListModel { id: targetModel }

    function clearSourceSelection() {
        sourceIndex = -1
        sourceAnchor = -1
        selectedSourceIndexes = []
    }

    onSourcePresetsChanged: clearSourceSelection()

    function selectionPosition(index) {
        return selectedSourceIndexes.indexOf(index) + 1
    }
    function isSourceSelected(index) {
        return selectionPosition(index) > 0
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
            // The direction is meaningful: picking Blues then Shift+Pop
            // Rock appends Blues, Jazz, Sholawat, ... Pop Rock in that order.
            var direction = index >= sourceAnchor ? 1 : -1
            for (var i = sourceAnchor; ; i += direction) {
                if (Boolean(presets[i].valid) && next.indexOf(i) < 0)
                    next.push(i)
                if (i === index)
                    break
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
        // Do not sort. Ctrl deselect/reselect moves that pick to the end.
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
        stagedRevision++
        cancelTransferGesture()
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

    function addEntry(entry, atIndex) {
        if (editingLocked || !entry || !Boolean(entry.valid) || targetModel.count >= maxSlots)
            return false
        var path = String(entry.path || "")
        if (path.length === 0 || targetContains(path))
            return false
        var next = {
            fileName: String(entry.fileName || "preset.k500"),
            displayName: String(entry.displayName || entry.presetName || entry.fileName || "K500 PRESET"),
            presetName: String(entry.presetName || ""),
            originLabel: String(entry.originLabel || (entry.source === "folder" ? "LOCAL" : "SONKUPIK")),
            path: path
        }
        var at = atIndex === undefined ? targetModel.count
                                       : Math.max(0, Math.min(targetModel.count, atIndex))
        targetModel.insert(at, next)
        targetIndex = at
        stagedRevision++
        return true
    }

    function addSelected() {
        var source = root.sourcePresets
        // Exact manual pick order, valid-only, no duplicates; cap at ten
        // physical device slots. The native 10->1 store protocol is untouched.
        var selected = selectedSourceIndexes.slice()
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

    function returnEntry(index) {
        if (editingLocked || index < 0 || index >= targetModel.count)
            return false
        targetModel.remove(index)
        targetIndex = targetModel.count === 0 ? -1 : Math.min(index, targetModel.count - 1)
        stagedRevision++
        return true
    }

    function removeSelected() {
        return returnEntry(targetIndex)
    }

    function clearTarget() {
        if (editingLocked)
            return
        targetModel.clear()
        targetIndex = -1
        stagedRevision++
    }

    function moveTarget(from, insertAt) {
        if (editingLocked || from < 0 || from >= targetModel.count)
            return false
        var at = Math.max(0, Math.min(targetModel.count, insertAt))
        var to = at > from ? at - 1 : at
        if (to !== from)
            targetModel.move(from, to, 1)
        targetIndex = to
        return true
    }

    // Coordinates are mapped to Window.contentItem; no DropArea duplicates,
    // no shadow stage model and no QML Drag/MouseArea competing grabs.
    function targetInsertionAt(px, py) {
        var pos = slotCanvas.mapFromItem(root.contentItem, px, py)
        if (pos.x < 0 || pos.x > slotCanvas.width || pos.y < 0 || pos.y > slotCanvas.height)
            return -1
        var rowPitch = slotCanvas.slotRowHeight + 3
        return Math.max(0, Math.min(targetModel.count,
               Math.floor((pos.y - 6 + slotCanvas.slotRowHeight / 2) / rowPitch)))
    }
    function overSourceAt(px, py) {
        var pos = sourceList.mapFromItem(root.contentItem, px, py)
        return pos.x >= 0 && pos.x <= sourceList.width
               && pos.y >= 0 && pos.y <= sourceList.height
    }
    function beginTransferGesture(kind, index, px, py) {
        if (editingLocked)
            return
        dragKind = kind
        dragIndex = index
        dragStartX = px
        dragStartY = py
        dragX = px
        dragY = py
        dragActive = false
        dragConsumedClick = false
        dragZone = ""
        dragInsertIndex = -1
        dragSourcePaths = []
        if (kind === "source") {
            var data = sourcePresets
            if (index < 0 || index >= data.length)
                return
            // Drag selected Ctrl/Shift group in chronological order; otherwise
            // drag exactly the item under the pointer.
            var order = isSourceSelected(index) ? selectedSourceIndexes.slice() : [index]
            dragSourcePaths = order.filter(function(i) {
                return i >= 0 && i < data.length && Boolean(data[i].valid)
            }).map(function(i) { return String(data[i].path || "") })
            dragLabel = String(data[index].displayName || data[index].fileName || "PRESET")
        } else if (kind === "target" && index >= 0 && index < targetModel.count) {
            dragLabel = String(targetModel.get(index).displayName)
        }
    }
    function updateTransferGesture(px, py) {
        if (!dragKind || editingLocked)
            return
        dragX = px
        dragY = py
        if (!dragActive && Math.abs(px - dragStartX) + Math.abs(py - dragStartY) > 11)
            dragActive = true
        if (!dragActive)
            return
        dragInsertIndex = targetInsertionAt(px, py)
        dragZone = dragInsertIndex >= 0 ? "target"
                 : overSourceAt(px, py) ? "source" : ""
    }
    function finishTransferGesture(px, py) {
        if (!dragKind)
            return
        updateTransferGesture(px, py)
        var wasDragging = dragActive
        var kind = dragKind
        var from = dragIndex
        var dropAt = dragInsertIndex
        var paths = dragSourcePaths.slice()
        var zone = dragZone
        dragKind = ""
        dragActive = false
        dragZone = ""
        dragInsertIndex = -1
        dragConsumedClick = wasDragging
        if (!wasDragging || editingLocked)
            return
        if (kind === "target") {
            if (zone === "source") returnEntry(from)
            else if (zone === "target") moveTarget(from, dropAt)
        } else if (kind === "source" && zone === "target") {
            // Resolve by path at drop time. A Sync/Folder refresh may have
            // changed catalogue row positions during the gesture.
            var catalog = catalogPresets || []
            var inserted = 0
            for (var i = 0; i < paths.length && targetModel.count < maxSlots; ++i) {
                var path = paths[i]
                for (var j = 0; j < catalog.length; ++j) {
                    if (String(catalog[j].path || "") === path) {
                        if (addEntry(catalog[j], dropAt + inserted))
                            inserted++
                        break
                    }
                }
            }
        }
    }
    function cancelTransferGesture() {
        dragKind = ""
        dragActive = false
        dragZone = ""
        dragInsertIndex = -1
    }

    function uploadTransfer() {
        if (!fileBridge || !presetManager || targetModel.count === 0 || root.dragActive)
            return
        if (!presetManager.usbStoreAvailable) {
            root.connectNotice = "K500 belum terhubung lewat USB. Tutup jendela ini, lalu tekan Connect di bagian atas aplikasi."
            connectNoticeTimer.restart()
            return
        }
        if (presetManager.busy)
            return
        connectNoticeTimer.stop()
        root.connectNotice = ""
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

            // MASS_UPLOAD_QUIET_COPY_V9 — one title; shortcuts available on hover.
            // MASS_UPLOAD_VISUAL_WORKFLOW_V3 — selection/staging/upload stay separate.
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Text {
                    Layout.fillWidth: true
                    text: "MASS UPLOAD PRESETS"
                    color: Theme.text
                    font.family: Theme.monoFamily
                    font.pixelSize: 12
                    font.weight: Font.Bold
                    font.letterSpacing: .65
                }
                Text {
                    text: "?"
                    color: Theme.textSoft
                    font.family: Theme.monoFamily
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                    ToolTip.visible: workflowHelp.containsMouse
                    ToolTip.delay: 450
                    ToolTip.text: "Ctrl+click selects presets in your chosen order. Shift+click selects a range. Drag to move between panels and reorder."
                    MouseArea {
                        id: workflowHelp
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.WhatsThisCursor
                    }
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
                                font.pixelSize: 10
                                font.weight: Font.DemiBold
                                font.letterSpacing: .55
                            }
                            Text {
                                anchors.right: parent.right
                                anchors.rightMargin: 12
                                anchors.verticalCenter: parent.verticalCenter
                                text: String(root.sourcePresets.length) + " AVAILABLE" + (root.selectedSourceIndexes.length ? "  ·  " + root.selectedSourceIndexes.length + " SELECTED" : "")
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
                                // Normal sync success and folder paths are not
                                // permanent captions. Keep failures and activity visible.
                                Text {
                                    Layout.fillWidth: true
                                    text: root.fileBridge && root.fileBridge.officialSyncBusy
                                          ? "Syncing presets…"
                                          : (root.fileBridge ? String(root.fileBridge.officialSyncError || "") : "")
                                    color: root.fileBridge && root.fileBridge.officialSyncBusy ? Theme.textSoft : Theme.amber
                                    font.family: Theme.monoFamily
                                    font.pixelSize: 9
                                    elide: Text.ElideRight
                                }
                                SoftButton {
                                    Layout.preferredWidth: 64
                                    text: "Folder"
                                    compact: true
                                    enabled: !!root.fileBridge
                                    onClicked: folderDialog.open()
                                    HoverHandler { id: folderHelp }
                                    ToolTip.visible: folderHelp.hovered
                                    ToolTip.delay: 500
                                    ToolTip.text: root.fileBridge && String(root.fileBridge.presetFolder || "").length > 0
                                                  ? String(root.fileBridge.presetFolder) : "Choose a folder of .k500 presets"
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
                                    interactive: !root.dragActive
                                    displaced: Transition {
                                        NumberAnimation { properties: "x,y"; duration: 145
                                                          easing.type: Easing.OutCubic }
                                    }

                                    delegate: Rectangle {
                                        required property int index
                                        required property var modelData
                                        width: sourceList.width
                                        height: 36
                                        radius: 6
                                        readonly property bool validPreset: Boolean(modelData.valid)
                                        readonly property bool selected: root.isSourceSelected(index)
                                        opacity: root.dragActive && root.dragKind === "source"
                                                 && root.dragSourcePaths.indexOf(String(modelData.path || "")) >= 0 ? 0.45 : 1
                                        Behavior on opacity { NumberAnimation { duration: 115 } }
                                        readonly property string origin: String(modelData.originLabel || (modelData.source === "folder" ? "LOCAL" : "SONKUPIK"))
                                        // MASS_UPLOAD_CANONICAL_BADGE_V1 — stable preset source identity.
                                        // Not a row index and never a destination K500 hardware slot.
                                        readonly property int catalogNumber: Number(modelData.catalogNumber || 0)
                                        readonly property bool numberedOfficial: origin === "SONKUPIK" && catalogNumber >= 1 && catalogNumber <= 40
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
                                            // Source identity is number + name only.
                                            // Leave modelData intact for native validation.
                                            Text {
                                                Layout.fillWidth: true
                                                text: String(modelData.displayName || modelData.presetName || modelData.fileName || "K500 PRESET")
                                                color: validPreset ? Theme.text : Theme.textDim
                                                font.family: Theme.monoFamily
                                                font.pixelSize: 11
                                                font.weight: Font.DemiBold
                                                elide: Text.ElideRight
                                            }
                                            Text {
                                                text: !validPreset ? "INVALID"
                                                      : selected ? "#" + String(root.selectionPosition(index)).padStart(2, "0") : ""
                                                color: selected ? Theme.accent : Theme.amber
                                                font.family: Theme.monoFamily
                                                font.pixelSize: 9
                                                font.weight: Font.Bold
                                            }
                                        }

                                        MouseArea {
                                            id: sourceMouse
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            preventStealing: true
                                            cursorShape: validPreset ? Qt.OpenHandCursor : Qt.ArrowCursor
                                            enabled: validPreset && !root.editingLocked
                                            onPressed: function(mouse) {
                                                var p = mapToItem(root.contentItem, mouse.x, mouse.y)
                                                root.beginTransferGesture("source", index, p.x, p.y)
                                            }
                                            onPositionChanged: function(mouse) {
                                                if (!pressed) return
                                                var p = mapToItem(root.contentItem, mouse.x, mouse.y)
                                                root.updateTransferGesture(p.x, p.y)
                                            }
                                            onReleased: function(mouse) {
                                                var p = mapToItem(root.contentItem, mouse.x, mouse.y)
                                                root.finishTransferGesture(p.x, p.y)
                                            }
                                            onCanceled: root.cancelTransferGesture()
                                            onClicked: function(mouse) {
                                                if (!root.dragConsumedClick)
                                                    root.selectSource(index, mouse.modifiers)
                                            }
                                            onDoubleClicked: {
                                                if (!root.dragConsumedClick) {
                                                    root.selectSource(index, Qt.NoModifier)
                                                    root.addSelected()
                                                }
                                            }
                                        }
                                    }

                                    Text {
                                        anchors.centerIn: parent
                                        visible: sourceList.count === 0
                                        width: parent.width - 30
                                        horizontalAlignment: Text.AlignHCenter
                                        text: root.catalogPresets.length > 0
                                          ? "All available presets are in Device Slots."
                                          : "No presets. Choose Folder or Sync."
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
                    // MASS_UPLOAD_STAGING_ACTIONS_V3 — these edit the PC-side
                    // upload plan only. Existing selection chronology is unchanged.
                    // FINAL_ADD_DIRECTION_COUNT_V1_1_2 — keep plain Add wording,
                    // but preserve selected count and rightward action direction.
                    // Both buttons stage a PC-side plan, never write USB.
                    SoftButton { Layout.fillWidth: true; text: root.selectedSourceIndexes.length > 1 ? "Add (" + root.selectedSourceIndexes.length + ") →" : "Add →"; compact: true; enabled: root.selectedSourceIndexes.length > 0 && targetModel.count < root.maxSlots; onClicked: root.addSelected() }
                    SoftButton { Layout.fillWidth: true; text: "Add All →"; compact: true; enabled: sourceList.count > 0 && targetModel.count < root.maxSlots; onClicked: root.addAll() }
                    Item { Layout.preferredHeight: 12 }
                    SoftButton { Layout.fillWidth: true; text: "← Remove"; compact: true; enabled: root.targetIndex >= 0; onClicked: root.removeSelected() }
                    SoftButton { Layout.fillWidth: true; text: "Clear plan"; compact: true; enabled: targetModel.count > 0; onClicked: root.clearTarget() }
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
                                font.pixelSize: 10
                                font.weight: Font.DemiBold
                                font.letterSpacing: .55
                            }
                            Text {
                                anchors.right: parent.right
                                anchors.rightMargin: 12
                                anchors.verticalCenter: parent.verticalCenter
                                text: String(targetModel.count) + " / 10 STAGED"
                                color: targetModel.count >= root.maxSlots ? Theme.amber : Theme.accent
                                font.family: Theme.monoFamily
                                font.pixelSize: 9
                                font.weight: Font.DemiBold
                            }
                            Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: 1; color: Theme.borderSoft }
                        }

                        Rectangle {
                            id: slotCanvas
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.margins: 10
                            radius: 8
                            color: "#090D11"
                            border.width: 1
                            border.color: "#050708"
                            clip: true

                            // MASS_UPLOAD_TEN_SLOT_SKELETON_V3: an inert, fixed
                            // 10-slot plan under the existing ListView. A staged
                            // row fully overlays its matching placeholder. There
                            // is no fake readback, second staging model, or edit.
                            // The min-height dialog reduces rows instead of
                            // clipping the last hardware slot.
                            readonly property real slotRowHeight: Math.min(38, Math.max(24,
                                (height - 12 - (root.maxSlots - 1) * 3) / root.maxSlots))
                            // MASS_TRANSFER_INSERTION_GUIDE_V1 — precise slot insertion,
                            // not a second list, and no animation on the hardware plan.
                            Rectangle {
                                z: 6
                                visible: root.dragActive && root.dragZone === "target"
                                         && root.dragInsertIndex >= 0
                                         && (!root.editingLocked)
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.leftMargin: 9
                                anchors.rightMargin: 9
                                height: 3
                                radius: 2
                                y: 5 + Math.min(root.maxSlots, root.dragInsertIndex)
                                   * (slotCanvas.slotRowHeight + 3)
                                color: Theme.accent
                                opacity: .95
                            }

                            Column {
                                id: emptySlotPlan
                                anchors.fill: parent
                                anchors.margins: 6
                                spacing: 3
                                Repeater {
                                    model: root.maxSlots
                                    delegate: Rectangle {
                                        required property int index
                                        width: emptySlotPlan.width
                                        height: slotCanvas.slotRowHeight
                                        radius: 6
                                        // Opacity, not visible: Column must retain
                                        // empty-row geometry when slots are staged.
                                        opacity: index < targetModel.count ? 0 : 1
                                        color: "#0B1115"
                                        border.width: 1
                                        border.color: "#253039"
                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.leftMargin: 8
                                            anchors.rightMargin: 8
                                            spacing: 8
                                            Rectangle {
                                                Layout.preferredWidth: 30
                                                Layout.preferredHeight: 21
                                                radius: 5
                                                color: "#11191E"
                                                border.width: 1
                                                border.color: Theme.borderSoft
                                                Text {
                                                    anchors.centerIn: parent
                                                    text: String(index + 1).padStart(2, "0")
                                                    color: Theme.textDim
                                                    font.family: Theme.monoFamily
                                                    font.pixelSize: 9
                                                    font.weight: Font.Medium
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            ListView {
                                id: targetList
                                anchors.fill: parent
                                anchors.margins: 6
                                z: 1
                                spacing: 3
                                clip: true
                                model: targetModel
                                interactive: !root.dragActive
                                displaced: Transition {
                                    NumberAnimation { properties: "x,y"; duration: 150
                                                      easing.type: Easing.OutCubic }
                                }
                                add: Transition {
                                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 125 }
                                }

                                delegate: Rectangle {
                                    required property int index
                                    required property string displayName
                                    required property string fileName
                                    required property string path
                                    required property string originLabel
                                    width: targetList.width
                                    height: slotCanvas.slotRowHeight
                                    radius: 6
                                    readonly property bool selected: index === root.targetIndex
                                    opacity: root.dragActive && root.dragKind === "target"
                                             && root.dragIndex === index ? 0.35 : 1
                                    Behavior on opacity { NumberAnimation { duration: 115 } }
                                    color: selected ? "#15252A" : targetMouse.containsMouse ? "#12181D" : "#0D1115"
                                    border.width: 1
                                    border.color: selected ? Theme.accentSoft : "#252D34"
                                    // Traceability remains available on hover, not
                                    // duplicated under every destination preset.
                                    ToolTip.visible: targetMouse.containsMouse
                                    ToolTip.delay: 600
                                    ToolTip.text: originLabel + " · " + fileName

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
                                        Text {
                                            text: "⋮⋮"
                                            font.pixelSize: 13
                                            color: Theme.textDim
                                        }
                                        Text {
                                            Layout.fillWidth: true
                                            text: displayName
                                            color: Theme.text
                                            font.family: Theme.monoFamily
                                            font.pixelSize: 11
                                            font.weight: Font.DemiBold
                                            elide: Text.ElideRight
                                        }
                                    }

                                    MouseArea {
                                        id: targetMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        preventStealing: true
                                        cursorShape: Qt.OpenHandCursor
                                        enabled: !root.editingLocked
                                        onPressed: function(mouse) {
                                            var p = mapToItem(root.contentItem, mouse.x, mouse.y)
                                            root.beginTransferGesture("target", index, p.x, p.y)
                                        }
                                        onPositionChanged: function(mouse) {
                                            if (!pressed) return
                                            var p = mapToItem(root.contentItem, mouse.x, mouse.y)
                                            root.updateTransferGesture(p.x, p.y)
                                        }
                                        onReleased: function(mouse) {
                                            var p = mapToItem(root.contentItem, mouse.x, mouse.y)
                                            root.finishTransferGesture(p.x, p.y)
                                        }
                                        onCanceled: root.cancelTransferGesture()
                                        onClicked: {
                                            if (!root.dragConsumedClick)
                                                root.targetIndex = index
                                        }
                                        onDoubleClicked: {
                                            if (!root.dragConsumedClick) {
                                                root.targetIndex = index
                                                root.removeSelected()
                                            }
                                        }
                                    }
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
                             ? "" : "Connect K500 via USB to upload")
                    color: root.fileBridge && String(root.fileBridge.lastError || "").length > 0 ? Theme.amber : Theme.textDim
                    font.family: Theme.monoFamily
                    font.pixelSize: 9
                    elide: Text.ElideRight
                }
                SoftButton { Layout.preferredWidth: 86; text: "Cancel"; compact: true; onClicked: root.visible = false }
                SoftButton {
                    Layout.preferredWidth: 150
                    text: root.presetManager && root.presetManager.storeBusy ? "Uploading…" : "Upload to K500"
                    compact: true
                    enabled: targetModel.count > 0
                             && !!root.presetManager
                             && !root.presetManager.busy
                    onClicked: root.uploadTransfer()
                }
            }
        }

        // MASS_TRANSFER_FLOATING_PRESET_V1 — visual ghost tracks the pointer;
        // no clone is ever inserted into the plan until the drop is accepted.
        Rectangle {
            z: 70
            visible: root.dragActive
            x: Math.max(10, Math.min(parent.width - width - 10, root.dragX - width / 2))
            y: Math.max(8, Math.min(parent.height - height - 8, root.dragY - height / 2))
            width: 252
            height: 42
            radius: 8
            color: "#1B3038"
            border.width: 1
            border.color: root.dragZone === "target" || root.dragZone === "source"
                          ? Theme.accent : "#607782"
            opacity: .97
            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8
                Text { text: "⋮⋮"; color: Theme.accent; font.pixelSize: 13 }
                Text {
                    Layout.fillWidth: true
                    text: root.dragLabel
                    elide: Text.ElideRight
                    color: Theme.text
                    font.family: Theme.monoFamily
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                }
                Text {
                    text: root.dragKind === "source" && root.dragSourcePaths.length > 1
                          ? "+" + String(root.dragSourcePaths.length) : ""
                    color: Theme.accent
                    font.pixelSize: 10
                }
            }
        }

        // In-dialog toast: the Mass Upload window is modal, so the user must
        // close it before pressing the main TopBar Connect button.
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 60
            width: Math.min(parent.width - 56, 720)
            height: connectToastText.implicitHeight + 24
            z: 80
            radius: 9
            color: "#46321E"
            border.width: 1
            border.color: "#E8B865"
            visible: root.connectNotice.length > 0

            Text {
                id: connectToastText
                anchors.fill: parent
                anchors.margins: 12
                text: root.connectNotice
                color: "#FFF2D9"
                font.family: Theme.fontFamily
                font.pixelSize: 13
                font.weight: Font.DemiBold
                wrapMode: Text.WordWrap
                verticalAlignment: Text.AlignVCenter
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