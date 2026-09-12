import QtQuick
import QtQuick.Layouts

StudioPanel {
    id: root

    property string eyebrow: ""
    property string title: "Mic Inputs"
    property var channels: []
    property color accentColor: Theme.accent
    property bool compactCluster: title === "Reverb" || title === "Echo"
    property int selectedFader: -1
    readonly property real faderHeight: 160
    accentTop: false

    // PREMIUM_FX_SPACE_PANEL_V1
    // Reverb/Echo deliberately keep the shared rack geometry, but use the space
    // as a dedicated instrument surface: three premium knobs at the left and a
    // parameter-driven visual field at the right. The field is not an analyzer;
    // it visualizes the actual exposed K500 parameters without inventing Size,
    // Diffusion, Width or other unsupported controls.
    // FX_FIELD_ZERO_LEVEL_HIDE_V2 — LEVEL 0 means no visible effect field at all.
    // FX_FIELD_CAPTION_BASELINE_V2 — the field card bottom follows the 118px
    // premium knob/value capsule bottom instead of filling the complete rack.
    // STATIC_ECHO_TAIL_V2 — Echo uses a calm stationary tap-tail, never a travelling dot.
    property real fxVisual0: 0
    property real fxVisual1: 0
    property real fxVisual2: 0
    readonly property bool reverbMode: root.title === "Reverb"
    readonly property real fxLevelNorm: root.clamp(root.fxVisual0 / 100.0, 0, 1)
    readonly property bool fxVisualActive: root.fxVisual0 > 0.0001

    function clamp(v,a,b){ return Math.max(a,Math.min(b,v)) }
    function syncFxVisuals() {
        if (!root.compactCluster || !root.channels || root.channels.length < 3) return
        root.fxVisual0 = Number(root.channels[0].value)
        root.fxVisual1 = Number(root.channels[1].value)
        root.fxVisual2 = Number(root.channels[2].value)
    }
    function setFxVisual(index,value) {
        if (index === 0) root.fxVisual0 = value
        else if (index === 1) root.fxVisual1 = value
        else if (index === 2) root.fxVisual2 = value
    }
    onChannelsChanged: syncFxVisuals()
    Component.onCompleted: syncFxVisuals()

    // P1_RACK_FADER_LIVE_BRIDGE_V1
    function studioEngine() {
        var p = root
        while (p) {
            if (p.engine && typeof p.engine.editDevicePath === "function") return p.engine
            p = p.parent
        }
        return null
    }
    function livePathFor(label) {
        var t = String(root.title || "")
        var l = String(label || "").toUpperCase()
        if (t === "Mic Inputs") {
            if (l === "MIC A") return "mic.micAVol"
            if (l === "MIC B") return "mic.micBVol"
            if (l === "FBX") return "mic.fbxLevel"
            return ""
        }
        var section = ""
        if (t === "Main Bus") section = "main"
        else if (t === "Surround Bus") section = "surround"
        else if (t === "Center Bus") section = "center"
        else if (t === "Subwoofer Bus") section = "sub"
        if (!section.length) return ""

        if (l === "L") return "outputs." + section + ".lVolDb"
        if (l === "R") return "outputs." + section + ".rVolDb"
        if (l === "CTR" || l === "SUB") return "outputs." + section + ".outputVolDb"
        if (l === "MIC") return "outputs." + section + ".micDirect"
        if (l === "MUSIC") return "outputs." + section + ".musicLevel"
        if (l === "REV") return "outputs." + section + ".reverbLevel"
        if (l === "ECHO") return "outputs." + section + ".echoLevel"
        return ""
    }
    function dispatchLive(label, value) {
        var path = livePathFor(label)
        var engine = studioEngine()
        if (path.length && engine) engine.editDevicePath(path, value)
    }
    function muteCapable(label) {
        var t = String(root.title || "")
        var l = String(label || "").toUpperCase()
        if (t === "Main Bus" || t === "Surround Bus") return l === "L" || l === "R"
        if (t === "Center Bus") return l === "CTR"
        if (t === "Subwoofer Bus") return l === "SUB"
        return false
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
                text: root.title.toUpperCase()
                color: Theme.text
                font.family: Theme.monoFamily
                font.pixelSize: 10
                font.weight: Font.Bold
                font.letterSpacing: 1.05
            }
            Text {
                visible: root.compactCluster
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                text: root.reverbMode ? "SPACE DESIGN" : "TIME DESIGN"
                color: Theme.textFaint
                font.family: Theme.monoFamily
                font.pixelSize: 8
                font.weight: Font.DemiBold
                font.letterSpacing: 1.15
            }
            Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:1;color:Theme.borderSoft;opacity:.72 }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Modern Reverb / Echo instrument surface.
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                anchors.topMargin: 10
                anchors.bottomMargin: 10
                spacing: 14
                visible: root.compactCluster

                RowLayout {
                    Layout.preferredWidth: 318
                    Layout.minimumWidth: 288
                    Layout.maximumWidth: 336
                    Layout.fillHeight: true
                    spacing: 3

                    Repeater {
                        model: root.channels
                        delegate: Item {
                            id: fxChannel
                            required property int index
                            required property var modelData
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.minimumWidth: 88
                            property real localValue: Number(modelData.value)

                            StudioKnob {
                                anchors.centerIn: parent
                                premium: true
                                compact: false
                                title: String(fxChannel.modelData.label || "")
                                value: fxChannel.localValue
                                from: Number(fxChannel.modelData.from)
                                to: Number(fxChannel.modelData.to)
                                step: Number(fxChannel.modelData.step || 1)
                                defaultValue: Number(fxChannel.modelData.value)
                                decimals: Number(fxChannel.modelData.decimals || 0)
                                unit: String(fxChannel.modelData.unit || "")
                                accentColor: root.accentColor
                                onValueEdited: function(v) {
                                    fxChannel.localValue = v
                                    root.setFxVisual(fxChannel.index,v)
                                    // Preserve the existing verified-write policy. FX controls do not
                                    // invent a transport path merely because their presentation changed.
                                    root.dispatchLive(fxChannel.modelData.label,v)
                                }
                            }
                        }
                    }
                }

                Item {
                    id: fieldSlot
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumWidth: 310

                    Rectangle {
                        id: fieldCard
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        // Premium knob implicitHeight is 118. This upward offset makes
                        // the card bottom land exactly on the value-capsule bottom.
                        anchors.verticalCenterOffset: -(fieldCard.height - 118) / 2
                        height: 180
                        radius: 10
                        border.width: 1
                        border.color: fieldMouse.containsMouse ? Theme.accentSoft : "#18242B"
                        clip: true
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#081015" }
                            GradientStop { position: 0.52; color: "#050A0E" }
                            GradientStop { position: 1.0; color: "#030609" }
                        }
                        Behavior on border.color { ColorAnimation { duration: 100 } }

                        MouseArea {
                            id: fieldMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.NoButton
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            height: 1
                            color: Theme.accent
                            opacity: .16
                        }

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            anchors.topMargin: 9
                            anchors.bottomMargin: 9
                            spacing: 3

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 27
                                spacing: 8

                                ColumnLayout {
                                    spacing: -1
                                    Text {
                                        text: root.reverbMode ? "REVERB FIELD" : "ECHO FIELD"
                                        color: Theme.text
                                        font.family: Theme.monoFamily
                                        font.pixelSize: 9
                                        font.weight: Font.Bold
                                        font.letterSpacing: 1.15
                                    }
                                    Text {
                                        text: "PARAMETER RESPONSE"
                                        color: Theme.textFaint
                                        font.family: Theme.monoFamily
                                        font.pixelSize: 7
                                        font.weight: Font.Medium
                                        font.letterSpacing: .9
                                    }
                                }

                                Item { Layout.fillWidth: true }

                                Rectangle {
                                    Layout.preferredWidth: root.reverbMode ? 88 : 92
                                    Layout.preferredHeight: 23
                                    radius: 7
                                    color: "#061014"
                                    border.width: 1
                                    border.color: "#17333A"
                                    Text {
                                        anchors.centerIn: parent
                                        text: root.reverbMode
                                              ? "DECAY  " + (root.fxVisual1/1000).toFixed(2) + " s"
                                              : "DELAY  " + Math.round(root.fxVisual2) + " ms"
                                        color: Theme.accent
                                        font.family: Theme.monoFamily
                                        font.pixelSize: 8
                                        font.weight: Font.Bold
                                    }
                                }

                                Rectangle {
                                    Layout.preferredWidth: 70
                                    Layout.preferredHeight: 23
                                    radius: 7
                                    color: "#0D0B05"
                                    border.width: 1
                                    border.color: "#392D10"
                                    Text {
                                        anchors.centerIn: parent
                                        text: root.reverbMode
                                              ? "WET  " + Math.round(root.fxVisual0) + "%"
                                              : "REP  " + Math.round(root.fxVisual1)
                                        color: Theme.amber
                                        font.family: Theme.monoFamily
                                        font.pixelSize: 8
                                        font.weight: Font.Bold
                                    }
                                }
                            }

                            Item {
                                id: fieldStage
                                Layout.fillWidth: true
                                Layout.fillHeight: true

                                // The whole visual response disappears at LEVEL 0. The card,
                                // labels and parameter readouts remain so the layout never jumps.
                                Item {
                                    id: activeFieldVisual
                                    anchors.fill: parent
                                    visible: root.fxVisualActive
                                    opacity: .35 + root.fxLevelNorm * .65
                                    Behavior on opacity { NumberAnimation { duration: 90 } }

                                    Rectangle {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: parent.width * .78
                                        height: 1
                                        color: Theme.accent
                                        opacity: .07
                                    }

                                    // Reverb: an orbital decay field. Ring size follows decay,
                                    // spread follows pre-delay and the complete field follows LEVEL.
                                    Item {
                                        id: reverbField
                                        visible: root.reverbMode
                                        width: Math.min(activeFieldVisual.width * .70, 330)
                                        height: Math.min(activeFieldVisual.height * .90, 116)
                                        anchors.centerIn: parent

                                        Repeater {
                                            model: 6
                                            delegate: Rectangle {
                                                required property int index
                                                readonly property real decayNorm: root.clamp((root.fxVisual1-100)/4900,0,1)
                                                readonly property real preNorm: root.clamp(root.fxVisual2/300,0,1)
                                                width: reverbField.width * (.30 + index*.095 + decayNorm*.035)
                                                height: reverbField.height * (.19 + index*.075 + preNorm*.018)
                                                anchors.centerIn: parent
                                                radius: height/2
                                                color: "transparent"
                                                border.width: index < 2 ? 1.3 : 1.0
                                                border.color: index % 3 === 0 ? Theme.amber : Theme.accent
                                                opacity: .09 + (5-index)*.018
                                                rotation: index*23
                                                NumberAnimation on rotation {
                                                    from: index*23
                                                    to: index*23 + (index%2===0 ? 360 : -360)
                                                    duration: 9000 + index*1900 + Math.round(root.clamp(root.fxVisual1,100,5000)*1.2)
                                                    loops: Animation.Infinite
                                                    running: root.visible && root.fxVisualActive && reverbField.visible
                                                }
                                            }
                                        }

                                        Item {
                                            id: particleOrbit
                                            anchors.fill: parent
                                            opacity: .48
                                            Repeater {
                                                model: 10
                                                delegate: Rectangle {
                                                    required property int index
                                                    width: index%3===0 ? 4 : 3
                                                    height: width
                                                    radius: width/2
                                                    color: index%4===0 ? Theme.amber : Theme.accent
                                                    x: particleOrbit.width/2 + Math.cos(index*Math.PI*2/10) * particleOrbit.width*.36 - width/2
                                                    y: particleOrbit.height/2 + Math.sin(index*Math.PI*2/10) * particleOrbit.height*.28 - height/2
                                                    opacity: .22 + (index%4)*.10
                                                }
                                            }
                                            NumberAnimation on rotation {
                                                from: 0; to: 360
                                                duration: 15000 + Math.round(root.clamp(root.fxVisual1,100,5000))
                                                loops: Animation.Infinite
                                                running: root.visible && root.fxVisualActive && reverbField.visible
                                            }
                                        }

                                        Rectangle {
                                            anchors.centerIn: parent
                                            width: 66; height: 66; radius: 33
                                            color: Theme.accent
                                            opacity: .075
                                            border.width: 1
                                            border.color: Theme.accentSoft
                                            ScaleAnimator on scale {
                                                from: .94; to: 1.06
                                                duration: 1800 + Math.round(root.clamp(root.fxVisual1,100,5000)*.18)
                                                loops: Animation.Infinite
                                                running: root.visible && root.fxVisualActive && reverbField.visible
                                            }
                                        }
                                        Rectangle {
                                            anchors.centerIn: parent
                                            width: 34; height: 34; radius: 17
                                            color: "#071418"
                                            border.width: 1
                                            border.color: Theme.accent
                                            Text {
                                                anchors.centerIn: parent
                                                text: "SPACE"
                                                color: Theme.accent
                                                font.family: Theme.monoFamily
                                                font.pixelSize: 7
                                                font.weight: Font.Bold
                                                font.letterSpacing: .8
                                            }
                                        }
                                    }

                                    // Echo: stationary temporal tap-tail. Delay controls total span,
                                    // Repeat controls persistence. Nothing travels across the panel.
                                    Item {
                                        id: echoField
                                        visible: !root.reverbMode
                                        width: Math.min(activeFieldVisual.width * .84, 390)
                                        height: Math.min(activeFieldVisual.height * .82, 110)
                                        anchors.centerIn: parent
                                        readonly property real delayNorm: root.clamp(root.fxVisual2 / 1000.0, 0, 1)
                                        readonly property real repeatNorm: root.clamp(root.fxVisual1 / 100.0, 0, 1)

                                        Item {
                                            id: echoTrain
                                            width: echoField.width * (.50 + echoField.delayNorm * .38)
                                            height: echoField.height * .66
                                            anchors.horizontalCenter: parent.horizontalCenter
                                            anchors.verticalCenter: parent.verticalCenter
                                            anchors.verticalCenterOffset: -4

                                            Rectangle {
                                                anchors.left: parent.left
                                                anchors.right: parent.right
                                                anchors.verticalCenter: parent.verticalCenter
                                                height: 1
                                                color: Theme.accent
                                                opacity: .10
                                            }

                                            Repeater {
                                                model: 7
                                                delegate: Item {
                                                    required property int index
                                                    width: 24
                                                    height: echoTrain.height
                                                    x: index * Math.max(1,(echoTrain.width-width)/6)
                                                    readonly property real persistence: Math.max(.08, echoField.repeatNorm)
                                                    readonly property real tapOpacity: index === 0
                                                                                       ? .92
                                                                                       : Math.max(.08, Math.pow(persistence, .32 + index*.26))

                                                    Rectangle {
                                                        anchors.horizontalCenter: parent.horizontalCenter
                                                        anchors.verticalCenter: parent.verticalCenter
                                                        width: index === 0 ? 4 : 3
                                                        height: Math.max(12, echoTrain.height * (.76 - index*.075))
                                                        radius: width/2
                                                        color: index === 0 ? Theme.amber : Theme.accent
                                                        opacity: parent.tapOpacity
                                                    }
                                                    Rectangle {
                                                        anchors.centerIn: parent
                                                        width: Math.max(7, 19-index*1.6)
                                                        height: width
                                                        radius: width/2
                                                        color: "transparent"
                                                        border.width: 1
                                                        border.color: index === 0 ? Theme.amber : Theme.accent
                                                        opacity: parent.tapOpacity * .42
                                                    }
                                                    Rectangle {
                                                        anchors.centerIn: parent
                                                        width: index === 0 ? 7 : 5
                                                        height: width
                                                        radius: width/2
                                                        color: index === 0 ? Theme.amber : Theme.accent
                                                        opacity: parent.tapOpacity * .82
                                                    }
                                                }
                                            }
                                        }

                                        Text {
                                            anchors.horizontalCenter: parent.horizontalCenter
                                            anchors.bottom: parent.bottom
                                            text: "TIME  /  REPEAT TAIL"
                                            color: Theme.textFaint
                                            font.family: Theme.monoFamily
                                            font.pixelSize: 7
                                            font.weight: Font.DemiBold
                                            font.letterSpacing: 1.1
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // MIXER_HEADER_MUTE_V2
            // Canonical mixer fader layout for every non-FX rack. Verified mute
            // controls remain in each channel header beside the caption.
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                anchors.topMargin: 10
                anchors.bottomMargin: 8
                spacing: 2
                visible: !root.compactCluster

                Repeater {
                    model: root.channels
                    delegate: Item {
                        id: channel
                        required property int index
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.minimumWidth: 48
                        Layout.fillHeight: true
                        property real localValue: Number(modelData.value)
                        property bool muted: false
                        readonly property bool selected: root.selectedFader === channel.index
                        readonly property bool canMute: root.muteCapable(modelData.label)
                        readonly property real muteFloor: Number(modelData.from)

                        // OUTPUT_MUTE_VERIFIED_FLOOR_V1
                        function setMuted(next) {
                            if (!channel.canMute || channel.muted === next) return
                            channel.muted = next
                            root.dispatchLive(channel.modelData.label,
                                              next ? channel.muteFloor : channel.localValue)
                        }

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 3

                            Item {
                                Layout.alignment: Qt.AlignHCenter
                                Layout.preferredWidth: Math.max(48,channel.width-4)
                                Layout.preferredHeight: 25

                                Row {
                                    anchors.centerIn: parent
                                    spacing: channel.canMute ? 4 : 0

                                    Column {
                                        anchors.verticalCenter: parent.verticalCenter
                                        spacing: -1
                                        Text {
                                            anchors.horizontalCenter: parent.horizontalCenter
                                            text: String(channel.modelData.label || "")
                                            color: channel.muted ? Theme.amber : rackFader.highlighted ? rackFader.accentColor : Theme.textDim
                                            style: rackFader.highlighted ? Text.Outline : Text.Normal
                                            styleColor: rackFader.highlighted ? Qt.rgba(rackFader.accentColor.r,rackFader.accentColor.g,rackFader.accentColor.b,.34) : "transparent"
                                            font.family: Theme.monoFamily
                                            font.pixelSize: 9
                                            font.weight: rackFader.highlighted || channel.muted ? Font.Bold : Font.DemiBold
                                            font.letterSpacing: .35
                                            Behavior on color { ColorAnimation { duration:75 } }
                                            Behavior on styleColor { ColorAnimation { duration:75 } }
                                        }
                                        Text {
                                            anchors.horizontalCenter: parent.horizontalCenter
                                            visible: String(channel.modelData.badge || "").length > 0
                                            text: String(channel.modelData.badge || "")
                                            color: rackFader.highlighted ? rackFader.accentColor : Theme.textFaint
                                            font.family: Theme.monoFamily
                                            font.pixelSize: 8
                                            font.weight: rackFader.highlighted ? Font.DemiBold : Font.Normal
                                            Behavior on color { ColorAnimation { duration:75 } }
                                        }
                                    }

                                    Rectangle {
                                        id: muteIconButton
                                        visible: channel.canMute
                                        width: 20; height: 20; radius: 6
                                        anchors.verticalCenter: parent.verticalCenter
                                        color: channel.muted ? "#211607" : mutePointer.containsMouse ? "#10171C" : "#090D10"
                                        border.width: 1
                                        border.color: channel.muted ? Theme.amber : mutePointer.containsMouse ? Theme.textDim : "#273038"

                                        LucideIcon {
                                            anchors.centerIn: parent
                                            width: 13; height: 13
                                            name: "volume-x"
                                            color: channel.muted ? Theme.amber : Theme.textSoft
                                            strokeWidth: channel.muted ? 2.1 : 1.8
                                        }
                                        MouseArea {
                                            id: mutePointer
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: channel.setMuted(!channel.muted)
                                        }
                                        Behavior on color { ColorAnimation { duration:75 } }
                                        Behavior on border.color { ColorAnimation { duration:75 } }
                                    }
                                }
                            }

                            Item { Layout.preferredHeight: 2 }

                            StudioFader {
                                id: rackFader
                                Layout.preferredHeight: root.faderHeight
                                Layout.minimumHeight: root.faderHeight
                                Layout.maximumHeight: root.faderHeight
                                Layout.preferredWidth: 48
                                Layout.alignment: Qt.AlignHCenter
                                value: channel.localValue
                                from: Number(channel.modelData.from)
                                to: Number(channel.modelData.to)
                                step: Number(channel.modelData.step || 1)
                                defaultValue: Number(channel.modelData.value)
                                accentColor: root.accentColor
                                selected: channel.selected
                                onActivated: root.selectedFader = channel.index
                                onValueEdited: function(v) {
                                    channel.localValue = v
                                    root.dispatchLive(channel.modelData.label,
                                                      channel.muted ? channel.muteFloor : v)
                                }
                            }

                            Rectangle {
                                Layout.alignment: Qt.AlignHCenter
                                Layout.preferredWidth: 58
                                Layout.preferredHeight: 23
                                radius: 8
                                color: channel.muted ? "#171208" : rackFader.highlighted ? "#081013" : "#05080A"
                                border.width: 1
                                border.color: channel.muted ? Theme.amber : rackFader.highlighted ? root.accentColor : "#020304"
                                Row {
                                    anchors.centerIn: parent
                                    spacing: 3
                                    Text {
                                        text: channel.localValue.toFixed(Number(channel.modelData.decimals || 0))
                                        color: Theme.amber
                                        font.family: Theme.monoFamily
                                        font.pixelSize: 9
                                        font.weight: Font.Bold
                                    }
                                    Text {
                                        visible: String(channel.modelData.unit || "").length > 0
                                        text: String(channel.modelData.unit || "")
                                        color: rackFader.highlighted ? Theme.textSoft : Theme.textDim
                                        font.family: Theme.monoFamily
                                        font.pixelSize: 7
                                        anchors.baseline: parent.children[0].baseline
                                        Behavior on color { ColorAnimation { duration:75 } }
                                    }
                                }
                                Behavior on border.color { ColorAnimation { duration:75 } }
                            }

                            Item { Layout.fillHeight: true }
                        }
                    }
                }
            }
        }
    }
}
