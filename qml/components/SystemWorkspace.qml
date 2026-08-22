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
    readonly property var defaultDeviceSlots: [
        "ARTIST GEN3 ARI", "PODCAST REBORN", "DANGDUT GEN3 ARI", "KARAOKE ARTIST",
        "AKUSTIK GEN3 ARI", "IMAM QORI GEN 3", "JAZZ GEN3 ARI", "ROCK GEN3 ARI",
        "MC CERAMAH GEN 3", "ADZAN MEKAH GEN3"
    ]
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
                        Text { anchors.left:parent.left;anchors.leftMargin:12;anchors.verticalCenter:parent.verticalCenter;text:"PRESET FILES";color:Theme.text;font.family:Theme.monoFamily;font.pixelSize:10;font.weight:Font.Bold;font.letterSpacing:1.05 }
                        Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:1;color:Theme.borderSoft }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.margins: 12
                        spacing: 9

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                Layout.fillWidth:true
                                text:root.fileBridge&&root.fileBridge.loaded?String(root.fileBridge.sourcePath):"No .k500 preset loaded"
                                color:root.fileBridge&&root.fileBridge.loaded?Theme.accent:Theme.textDim
                                font.family:Theme.monoFamily;font.pixelSize:10;font.weight:Font.Bold;elide:Text.ElideMiddle
                            }
                            SoftButton {
                                Layout.preferredWidth:62;text:"Open";compact:true
                                enabled:root.offlineFileMode&&!!root.fileBridge
                                onClicked:openPresetDialog.open()
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            radius: 10
                            color: "#090D11"
                            border.width: 1
                            border.color: "#050708"

                            Rectangle {
                                anchors.left:parent.left;anchors.right:parent.right;anchors.top:parent.top
                                anchors.margins:9;height:36;radius:7
                                gradient: Gradient { GradientStop{position:0;color:"#151A1F"} GradientStop{position:1;color:"#0B0F13"} }
                                border.width:1;border.color:root.fileBridge&&root.fileBridge.loaded?Theme.accentSoft:"#242C33"
                                RowLayout {
                                    anchors.fill:parent;anchors.leftMargin:10;anchors.rightMargin:10;spacing:8
                                    Text{text:"PC";color:Theme.amber;font.family:Theme.monoFamily;font.pixelSize:9;font.weight:Font.Bold}
                                    Text{Layout.fillWidth:true;text:root.fileBridge&&root.fileBridge.loaded?String(root.fileBridge.presetName):"OPEN A K500 PRESET";color:Theme.text;font.family:Theme.monoFamily;font.pixelSize:10;font.weight:Font.Bold;elide:Text.ElideRight}
                                    Text{
                                        text:root.fileBridge&&root.fileBridge.checksumOk?(root.fileBridge.dirty?"EDITED OK":"CHECKSUM OK"):"NO FILE"
                                        color:root.fileBridge&&root.fileBridge.checksumOk?Theme.accent:Theme.textDim
                                        font.family:Theme.monoFamily;font.pixelSize:8;font.weight:Font.Bold
                                    }
                                }
                            }

                            Text {
                                anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom
                                anchors.leftMargin:10;anchors.rightMargin:10;anchors.bottomMargin:9
                                text:root.fileBridge&&String(root.fileBridge.lastError||"").length>0
                                     ?String(root.fileBridge.lastError)
                                     :(root.fileBridge&&root.fileBridge.loaded&&root.fileBridge.dirty
                                       ?("Verified edit · "+String(root.fileBridge.changedByteCount)+" changed byte(s) incl. checksum")
                                       :(root.pcUploadReady
                                         ?("Ready · upload to selected slot "+String(root.selectedDeviceSlot+1))
                                         :(root.offlineFileMode?"Safe preview · validated source bytes":"USB connection required for PC preset upload")))
                                color:root.fileBridge&&String(root.fileBridge.lastError||"").length>0?Theme.amber:Theme.textDim
                                font.family:Theme.monoFamily;font.pixelSize:8;elide:Text.ElideRight
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            SoftButton {
                                Layout.fillWidth:true;text:"Save as";compact:true
                                enabled:root.fileBridge&&root.fileBridge.loaded
                                onClicked:savePresetDialog.open()
                            }
                            SoftButton {
                                Layout.fillWidth:true
                                text:root.presetManager&&root.presetManager.storeBusy?"Uploading…":"Upload to device"
                                compact:true
                                enabled:root.pcUploadReady
                                onClicked:root.uploadLoadedPreset()
                            }
                            SoftButton { Layout.fillWidth:true;text:"Mass upload";compact:true;enabled:false }
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
