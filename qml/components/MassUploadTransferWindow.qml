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

    property int sourceIndex: -1
    property int targetIndex: -1
    readonly property int maxSlots: 10
    // Unified source replaces the old folderPresets-only Mass Upload source.
    // folderPresets remains part of combinedPresets together with SONKUPIK official presets.
    readonly property var sourcePresets: root.fileBridge ? root.fileBridge.combinedPresets : []

    ListModel { id: targetModel }

    function openTransfer() {
        sourceIndex = -1
        targetIndex = -1
        targetModel.clear()
        if (fileBridge) {
            fileBridge.refreshPresetFolder()
            fileBridge.syncOfficialPresets()
        }
        visible = true
        requestActivate()
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
        if (sourceIndex < 0 || sourceIndex >= source.length)
            return
        addEntry(source[sourceIndex])
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
            presetManager.massUploadSlotImages(entries)
            visible = false
        }
    }

    FolderDialog {
        id: folderDialog
        title: "Select folder containing your K500 presets"
        onAccepted: {
            if (root.fileBridge) {
                root.fileBridge.setPresetFolder(selectedFolder)
                root.sourceIndex = -1
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
                    text: "Mix official SonKuPik presets and your own local presets, then map up to 10 selections to K500 Device Slots."
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
                                text: String(root.sourcePresets.length) + " PRESETS"
                                color: Theme.accent
                                font.family: Theme.monoFamily
                                font.pixelSize: 8
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
                                        font.pixelSize: 8
                                        elide: Text.ElideRight
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        text: root.fileBridge && String(root.fileBridge.presetFolder || "").length > 0
                                              ? ("LOCAL · " + String(root.fileBridge.presetFolder))
                                              : "LOCAL · choose a folder for your own presets"
                                        color: Theme.textDim
                                        font.family: Theme.monoFamily
                                        font.pixelSize: 7
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
                                        readonly property bool selected: index === root.sourceIndex
                                        readonly property string origin: String(modelData.originLabel || (modelData.source === "folder" ? "LOCAL" : "SONKUPIK"))
                                        color: selected ? "#15252A" : sourceMouse.containsMouse ? "#12181D" : "#0D1115"
                                        border.width: 1
                                        border.color: selected ? Theme.accentSoft : validPreset ? "#252D34" : "#553A32"

                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.leftMargin: 8
                                            anchors.rightMargin: 8
                                            spacing: 7
                                            Rectangle {
                                                Layout.preferredWidth: origin === "SONKUPIK" ? 62 : 42
                                                Layout.preferredHeight: 20
                                                radius: 5
                                                color: origin === "SONKUPIK" ? "#18252A" : "#171B20"
                                                border.width: 1
                                                border.color: origin === "SONKUPIK" ? Theme.accentSoft : Theme.borderSoft
                                                Text {
                                                    anchors.centerIn: parent
                                                    text: origin
                                                    color: origin === "SONKUPIK" ? Theme.accent : Theme.textSoft
                                                    font.family: Theme.monoFamily
                                                    font.pixelSize: 7
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
                                                    font.pixelSize: 9
                                                    font.weight: Font.Bold
                                                    elide: Text.ElideRight
                                                }
                                                Text {
                                                    Layout.fillWidth: true
                                                    text: String(modelData.description || modelData.fileName || "")
                                                    color: Theme.textDim
                                                    font.family: Theme.fontFamily
                                                    font.pixelSize: 7
                                                    elide: Text.ElideRight
                                                }
                                            }
                                            Text {
                                                text: validPreset ? "READY" : "INVALID"
                                                color: validPreset ? Theme.accent : Theme.amber
                                                font.family: Theme.monoFamily
                                                font.pixelSize: 7
                                                font.weight: Font.Bold
                                            }
                                        }

                                        MouseArea {
                                            id: sourceMouse
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: validPreset ? Qt.PointingHandCursor : Qt.ArrowCursor
                                            enabled: validPreset
                                            onClicked: root.sourceIndex = index
                                            onDoubleClicked: { root.sourceIndex = index; root.addSelected() }
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
                    SoftButton { Layout.fillWidth: true; text: "Add  >"; compact: true; enabled: root.sourceIndex >= 0 && targetModel.count < root.maxSlots; onClicked: root.addSelected() }
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
    }
}
