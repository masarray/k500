import QtQuick
import QtQuick.Layouts

StudioPanel {
    id: root
    required property var engine
    property int selectedFader: 0
    property int selectedSource: 0
    readonly property var sourceOptions: ["INPUT1", "INPUT2", "BT", "UDISK", "OPTIC", "UAUDIO"]
    implicitHeight: 304
    accentTop: false

    // MUSIC_SOURCE_SINGLE_CHOICE_V1
    // Native K500 exposes six mutually-exclusive playback sources but only five
    // physical gain parameters. OPTIC and UAUDIO intentionally share DIGITAL gain.
    // MUSIC_DIGITAL_GAIN_MIRROR_V1
    // Present that one DIGITAL gain as two synchronized visual strips so every
    // source key has an unambiguous fader directly below it. No extra protocol
    // parameter is invented: both strips read/write engine.digitalGain.
    function sourceFromDevice() {
        var state = root.engine && root.engine.deviceState ? root.engine.deviceState : null
        var music = state ? state.music : null
        var raw = music ? Number(music.sourceRaw) : 0
        return isFinite(raw) && raw >= 0 && raw <= 5 ? Math.round(raw) : 0
    }

    function chooseSource(index) {
        var next = Math.max(0, Math.min(5, Number(index)))
        if (root.selectedSource === next)
            return
        root.selectedSource = next
        root.engine.editDevicePath("music.sourceRaw", next)
    }

    Component.onCompleted: root.selectedSource = root.sourceFromDevice()

    Connections {
        target: root.engine
        function onDeviceStateChanged() {
            root.selectedSource = root.sourceFromDevice()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 35
            Text {
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                text: "MUSIC INPUT"
                color: Theme.text
                font.family: Theme.monoFamily
                font.pixelSize: 10
                font.weight: Font.Bold
                font.letterSpacing: 1.05
            }
            Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:1;color:Theme.borderSoft;opacity:.78 }
        }

        RowLayout {
            // MUSIC_SOURCE_OVER_FADER_LAYOUT_V2
            // MIXER_SOURCE_ILLUMINATED_SELECT_V1
            // Six source keys now each own a visually direct fader strip.
            // OPTIC + UAUDIO are two UI mirrors of the same native DIGITAL gain.
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.topMargin: 10
            Layout.bottomMargin: 10
            spacing: 0

            InputFader {
                Layout.fillWidth:true; Layout.fillHeight:true
                label:"INPUT1 GAIN"; sourceText:"INPUT1"; active:root.selectedSource===0
                sourceButtonMixerSelect:true
                value:root.engine.input1Gain; from:-12; to:12
                selected:root.selectedFader===0
                onSourceRequested:root.chooseSource(0)
                onActivated:root.selectedFader=0
                onValueEdited:function(v){root.engine.input1Gain=v}
                accentColor:Theme.blue
            }
            InputFader {
                Layout.fillWidth:true; Layout.fillHeight:true
                label:"INPUT2 GAIN"; sourceText:"INPUT2"; active:root.selectedSource===1
                sourceButtonMixerSelect:true
                value:root.engine.input2Gain; from:-12; to:12
                selected:root.selectedFader===1
                onSourceRequested:root.chooseSource(1)
                onActivated:root.selectedFader=1
                onValueEdited:function(v){root.engine.input2Gain=v}
                accentColor:Theme.blue
            }
            InputFader {
                Layout.fillWidth:true; Layout.fillHeight:true
                label:"BT GAIN"; sourceText:"BT"; active:root.selectedSource===2
                sourceButtonMixerSelect:true
                value:root.engine.bluetoothGain; from:-12; to:12
                selected:root.selectedFader===2
                onSourceRequested:root.chooseSource(2)
                onActivated:root.selectedFader=2
                onValueEdited:function(v){root.engine.bluetoothGain=v}
                accentColor:Theme.accent
            }
            InputFader {
                Layout.fillWidth:true; Layout.fillHeight:true
                label:"UDISK GAIN"; sourceText:"UDISK"; active:root.selectedSource===3
                sourceButtonMixerSelect:true
                value:root.engine.uDiskGain; from:-12; to:12
                selected:root.selectedFader===3
                onSourceRequested:root.chooseSource(3)
                onActivated:root.selectedFader=3
                onValueEdited:function(v){root.engine.uDiskGain=v}
                accentColor:Theme.violet
            }

            Repeater {
                model: root.sourceOptions

                delegate: Item {
                    required property int index
                    required property string modelData
                    readonly property bool checked: root.selectedSource === index
                    visible: index >= 4
                    Layout.fillWidth: visible
                    Layout.fillHeight: visible
                    Layout.preferredWidth: visible ? 76 : 0
                    Layout.minimumWidth: visible ? 54 : 0
                    Layout.maximumWidth: visible ? 16777215 : 0

                    InputFader {
                        anchors.fill: parent
                        label:"DIGITAL GAIN"
                        sourceText:modelData
                        active:parent.checked
                        sourceButtonMixerSelect: true
                        value:root.engine.digitalGain; from:-12; to:12
                        selected:root.selectedFader===parent.index
                        onSourceRequested:root.chooseSource(parent.index)
                        onActivated:root.selectedFader=parent.index
                        onValueEdited:function(v){root.engine.digitalGain=v}
                        accentColor:Theme.amber
                    }
                }
            }
        }
    }
}
