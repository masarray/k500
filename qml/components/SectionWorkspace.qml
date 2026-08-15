import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property int sectionIndex: 1
    property string activeMode: ""
    readonly property var profile: profileFor(sectionIndex)

    function p(label, value, from, to, unit, decimals, step, logarithmic) {
        return {
            label: label,
            value: value,
            from: from,
            to: to,
            unit: unit,
            decimals: decimals === undefined ? 1 : decimals,
            step: step === undefined ? 0.1 : step,
            logarithmic: logarithmic === undefined ? false : logarithmic
        }
    }

    function profileFor(index) {
        switch (index) {
        case 1:
            return {
                kicker: "DUAL VOCAL INPUT", title: "Mic", icon: "mic-2", accent: "#5EDDD4",
                subtitle: "Two-channel vocal front end · gain staging · tone · dynamics",
                heroTitle: "VOCAL FRONT END",
                heroParams: [
                    p("MIC 1 GAIN", 0.0, -24, 24, "dB", 1, 0.5),
                    p("MIC 2 GAIN", 0.0, -24, 24, "dB", 1, 0.5),
                    p("HPF", 90, 40, 300, "Hz", 0, 5),
                    p("PRESENCE", 1.5, -12, 12, "dB", 1, 0.1),
                    p("AIR", 1.0, -12, 12, "dB", 1, 0.1),
                    p("DE-ESS", 22, 0, 100, "%", 0, 1)
                ],
                leftTitle: "MIC 1 CHANNEL", leftNote: "Primary vocal strip",
                leftParams: [p("LOW", 0, -12, 12, "dB", 1, 0.1), p("MID", 0, -12, 12, "dB", 1, 0.1), p("HIGH", 0, -12, 12, "dB", 1, 0.1), p("COMP", 28, 0, 100, "%", 0, 1), p("LEVEL", 0, -60, 12, "dB", 1, 0.5)],
                midTitle: "MIC 2 CHANNEL", midNote: "Secondary vocal strip",
                midParams: [p("LOW", 0, -12, 12, "dB", 1, 0.1), p("MID", 0, -12, 12, "dB", 1, 0.1), p("HIGH", 0, -12, 12, "dB", 1, 0.1), p("COMP", 28, 0, 100, "%", 0, 1), p("LEVEL", 0, -60, 12, "dB", 1, 0.5)],
                rightTitle: "VOCAL BUS", rightNote: "Shared output and routing",
                rightParams: [p("WIDTH", 100, 0, 200, "%", 0, 1), p("FX SEND", -12, -60, 6, "dB", 1, 0.5), p("PAN", 0, -100, 100, "%", 0, 1), p("BUS LEVEL", 0, -60, 12, "dB", 1, 0.5)],
                modes: ["DUAL MONO", "LINKED", "STEREO"]
            }
        case 2:
            return {
                kicker: "ROOM TAIL", title: "Reverb", icon: "sparkles", accent: "#A58AE8",
                subtitle: "Early reflections · room body · decay shaping · wet/dry control",
                heroTitle: "ROOM ENGINE",
                heroParams: [p("PRE-DELAY", 28, 0, 180, "ms", 0, 1), p("DECAY", 2.4, 0.2, 10, "s", 1, 0.1), p("ROOM SIZE", 68, 0, 100, "%", 0, 1), p("DIFFUSION", 72, 0, 100, "%", 0, 1), p("DAMPING", 58, 0, 100, "%", 0, 1), p("MIX", 18, 0, 100, "%", 0, 1)],
                leftTitle: "EARLY REFLECTIONS", leftNote: "Attack and space definition",
                leftParams: [p("LEVEL", -8, -60, 6, "dB", 1, 0.5), p("SIZE", 46, 0, 100, "%", 0, 1), p("SPREAD", 72, 0, 100, "%", 0, 1), p("LOW CUT", 140, 20, 1200, "Hz", 0, 10, true), p("HIGH CUT", 9.5, 2, 20, "kHz", 1, 0.1)],
                midTitle: "TAIL SHAPER", midNote: "Density and spectral decay",
                midParams: [p("DENSITY", 74, 0, 100, "%", 0, 1), p("MOD DEPTH", 12, 0, 100, "%", 0, 1), p("LOW DECAY", 92, 20, 180, "%", 0, 1), p("HIGH DECAY", 66, 20, 180, "%", 0, 1), p("AIR", 1.2, -12, 12, "dB", 1, 0.1)],
                rightTitle: "REVERB OUTPUT", rightNote: "Mode and return level",
                rightParams: [p("RETURN", -6, -60, 6, "dB", 1, 0.5), p("WIDTH", 118, 0, 200, "%", 0, 1), p("DUCKING", 16, 0, 100, "%", 0, 1), p("PAN", 0, -100, 100, "%", 0, 1)],
                modes: ["HALL", "PLATE", "ROOM", "CHAMBER"]
            }
        case 3:
            return {
                kicker: "DELAY ENGINE", title: "Echo", icon: "repeat", accent: "#69AEEA",
                subtitle: "Tempo-aware delay · feedback contour · stereo motion · vocal ducking",
                heroTitle: "ECHO CORE",
                heroParams: [p("TIME", 320, 40, 1200, "ms", 0, 1), p("FEEDBACK", 28, 0, 92, "%", 0, 1), p("MIX", 14, 0, 100, "%", 0, 1), p("SPREAD", 58, 0, 100, "%", 0, 1), p("LOW CUT", 180, 20, 1800, "Hz", 0, 10, true), p("HIGH CUT", 7.8, 2, 20, "kHz", 1, 0.1)],
                leftTitle: "TAP GEOMETRY", leftNote: "Timing and stereo placement",
                leftParams: [p("L OFFSET", -8, -50, 50, "ms", 0, 1), p("R OFFSET", 12, -50, 50, "ms", 0, 1), p("PING PONG", 54, 0, 100, "%", 0, 1), p("MOD RATE", 0.6, 0.1, 8, "Hz", 1, 0.1), p("MOD DEPTH", 7, 0, 100, "%", 0, 1)],
                midTitle: "TONE & DUCKING", midNote: "Keep repeats behind the vocal",
                midParams: [p("DAMPING", 46, 0, 100, "%", 0, 1), p("SATURATION", 8, 0, 100, "%", 0, 1), p("DUCKING", 34, 0, 100, "%", 0, 1), p("ATTACK", 18, 1, 200, "ms", 0, 1), p("RELEASE", 420, 40, 2000, "ms", 0, 10)],
                rightTitle: "ECHO OUTPUT", rightNote: "Return routing and width",
                rightParams: [p("RETURN", -8, -60, 6, "dB", 1, 0.5), p("WIDTH", 112, 0, 200, "%", 0, 1), p("PAN", 0, -100, 100, "%", 0, 1), p("DRY TRIM", 0, -12, 12, "dB", 1, 0.1)],
                modes: ["STEREO", "PING PONG", "CENTER", "WIDE"]
            }
        case 4:
            return {
                kicker: "FRONT OUTPUT", title: "Main", icon: "speaker", accent: "#F0B928",
                subtitle: "Main L/R output · tonal balance · loudness control · protection",
                heroTitle: "MAIN OUTPUT PROCESSOR",
                heroParams: [p("LEVEL", 0, -60, 12, "dB", 1, 0.5), p("BALANCE", 0, -100, 100, "%", 0, 1), p("LOW SHELF", 0, -12, 12, "dB", 1, 0.1), p("HIGH SHELF", 0, -12, 12, "dB", 1, 0.1), p("HPF", 28, 20, 240, "Hz", 0, 1), p("LPF", 20, 4, 20, "kHz", 1, 0.1)],
                leftTitle: "OUTPUT TONE", leftNote: "Broad tonal trim",
                leftParams: [p("BASS", 0, -12, 12, "dB", 1, 0.1), p("LOW MID", 0, -12, 12, "dB", 1, 0.1), p("HIGH MID", 0, -12, 12, "dB", 1, 0.1), p("TREBLE", 0, -12, 12, "dB", 1, 0.1), p("AIR", 0, -12, 12, "dB", 1, 0.1)],
                midTitle: "DYNAMICS", midNote: "Output headroom and protection",
                midParams: [p("THRESHOLD", -2, -24, 0, "dB", 1, 0.1), p("RELEASE", 180, 20, 1200, "ms", 0, 10), p("SOFT CLIP", 12, 0, 100, "%", 0, 1), p("MAKEUP", 0, -12, 12, "dB", 1, 0.1), p("CEILING", -0.5, -6, 0, "dB", 1, 0.1)],
                rightTitle: "MAIN ROUTING", rightNote: "Front speaker topology",
                rightParams: [p("WIDTH", 100, 0, 200, "%", 0, 1), p("L TRIM", 0, -12, 12, "dB", 1, 0.1), p("R TRIM", 0, -12, 12, "dB", 1, 0.1), p("DELAY", 0, 0, 80, "ms", 1, 0.1)],
                modes: ["STEREO", "MONO", "L/R LINK"]
            }
        case 5:
            return {
                kicker: "REAR FIELD", title: "Surround", icon: "waves", accent: "#57D49A",
                subtitle: "Rear ambience field · decorrelation · distance · spatial balance",
                heroTitle: "SURROUND FIELD",
                heroParams: [p("LEVEL", -8, -60, 12, "dB", 1, 0.5), p("WIDTH", 138, 0, 200, "%", 0, 1), p("DISTANCE", 62, 0, 100, "%", 0, 1), p("DECORRELATE", 34, 0, 100, "%", 0, 1), p("DELAY", 18, 0, 80, "ms", 1, 0.1), p("AMBIENCE", 22, 0, 100, "%", 0, 1)],
                leftTitle: "REAR TONE", leftNote: "Spectral placement behind the listener",
                leftParams: [p("LOW CUT", 120, 20, 1500, "Hz", 0, 10, true), p("BASS", -1, -12, 12, "dB", 1, 0.1), p("MID", -0.5, -12, 12, "dB", 1, 0.1), p("TREBLE", -1.5, -12, 12, "dB", 1, 0.1), p("HIGH CUT", 14, 3, 20, "kHz", 1, 0.1)],
                midTitle: "FIELD SHAPER", midNote: "Depth without pulling focus rearward",
                midParams: [p("EARLY", 18, 0, 100, "%", 0, 1), p("LATE", 26, 0, 100, "%", 0, 1), p("CROSSFEED", 12, 0, 100, "%", 0, 1), p("MOTION", 8, 0, 100, "%", 0, 1), p("FOCUS", 56, 0, 100, "%", 0, 1)],
                rightTitle: "REAR ROUTING", rightNote: "Rear pair alignment",
                rightParams: [p("L TRIM", 0, -12, 12, "dB", 1, 0.1), p("R TRIM", 0, -12, 12, "dB", 1, 0.1), p("BALANCE", 0, -100, 100, "%", 0, 1), p("POLARITY", 0, 0, 1, "", 0, 1)],
                modes: ["REAR STEREO", "DIFFUSE", "AMBIENCE"]
            }
        case 6:
            return {
                kicker: "VOCAL FOCUS", title: "Center", icon: "radio-tower", accent: "#F07A85",
                subtitle: "Center image · vocal anchoring · intelligibility · center speaker control",
                heroTitle: "CENTER FOCUS",
                heroParams: [p("LEVEL", -3, -60, 12, "dB", 1, 0.5), p("FOCUS", 66, 0, 100, "%", 0, 1), p("PRESENCE", 1.5, -12, 12, "dB", 1, 0.1), p("AIR", 0.8, -12, 12, "dB", 1, 0.1), p("HPF", 100, 40, 500, "Hz", 0, 5), p("DELAY", 0, 0, 80, "ms", 1, 0.1)],
                leftTitle: "VOCAL TONE", leftNote: "Keep speech and singing intelligible",
                leftParams: [p("BODY", 0, -12, 12, "dB", 1, 0.1), p("NASAL", 0, -12, 12, "dB", 1, 0.1), p("PRESENCE", 1, -12, 12, "dB", 1, 0.1), p("SIBILANCE", -1, -12, 12, "dB", 1, 0.1), p("AIR", 1, -12, 12, "dB", 1, 0.1)],
                midTitle: "CENTER DYNAMICS", midNote: "Stable center image under loud passages",
                midParams: [p("COMP", 24, 0, 100, "%", 0, 1), p("ATTACK", 18, 1, 200, "ms", 0, 1), p("RELEASE", 220, 20, 1200, "ms", 0, 10), p("DE-ESS", 18, 0, 100, "%", 0, 1), p("LIMIT", -1, -12, 0, "dB", 1, 0.1)],
                rightTitle: "CENTER ROUTING", rightNote: "Speaker and phantom-center behavior",
                rightParams: [p("MUSIC BLEED", -18, -60, 0, "dB", 1, 0.5), p("FX BLEED", -12, -60, 0, "dB", 1, 0.5), p("TRIM", 0, -12, 12, "dB", 1, 0.1), p("POLARITY", 0, 0, 1, "", 0, 1)],
                modes: ["VOCAL CENTER", "PHANTOM", "FULL MIX"]
            }
        case 7:
            return {
                kicker: "BASS MANAGEMENT", title: "Sub", icon: "activity", accent: "#D8C15B",
                subtitle: "Sub crossover · phase alignment · low-frequency contour · limiter",
                heroTitle: "SUBWOOFER PROCESSOR",
                heroParams: [p("LEVEL", -3, -60, 12, "dB", 1, 0.5), p("LPF", 90, 40, 220, "Hz", 0, 1), p("PHASE", 0, 0, 180, "deg", 0, 1), p("DELAY", 0, 0, 40, "ms", 1, 0.1), p("BASS BOOST", 0, -12, 12, "dB", 1, 0.1), p("LIMIT", -1.5, -18, 0, "dB", 1, 0.1)],
                leftTitle: "CROSSOVER", leftNote: "Main-to-sub handoff",
                leftParams: [p("LPF FREQ", 90, 40, 220, "Hz", 0, 1), p("SLOPE", 24, 6, 48, "dB", 0, 6), p("MAIN HPF", 70, 20, 220, "Hz", 0, 1), p("OVERLAP", 8, 0, 100, "%", 0, 1), p("ALIGN", 50, 0, 100, "%", 0, 1)],
                midTitle: "LOW CONTOUR", midNote: "Weight without boom",
                midParams: [p("SUBSONIC", 28, 15, 60, "Hz", 0, 1), p("PUNCH", 1, -12, 12, "dB", 1, 0.1), p("WEIGHT", 1.5, -12, 12, "dB", 1, 0.1), p("MUD CUT", -1, -12, 12, "dB", 1, 0.1), p("HARMONICS", 6, 0, 100, "%", 0, 1)],
                rightTitle: "SUB OUTPUT", rightNote: "Protection and topology",
                rightParams: [p("TRIM", 0, -12, 12, "dB", 1, 0.1), p("CEILING", -1, -12, 0, "dB", 1, 0.1), p("RELEASE", 240, 20, 1400, "ms", 0, 10), p("POLARITY", 0, 0, 1, "", 0, 1)],
                modes: ["MONO SUB", "STEREO SUB", "LFE"]
            }
        default:
            return {
                kicker: "GLOBAL SETUP", title: "System", icon: "settings-2", accent: "#8FA4B2",
                subtitle: "Device connection · global gain structure · safety · synchronization",
                heroTitle: "SYSTEM CONTROL",
                heroParams: [p("GLOBAL TRIM", 0, -18, 12, "dB", 1, 0.1), p("STARTUP LEVEL", -18, -60, 0, "dB", 1, 0.5), p("UI SCALE", 100, 80, 140, "%", 0, 1), p("SYNC RATE", 20, 5, 60, "Hz", 0, 1), p("METER RATE", 30, 10, 60, "Hz", 0, 1), p("SAFETY", 100, 0, 100, "%", 0, 1)],
                leftTitle: "DEVICE", leftNote: "K500 transport preferences",
                leftParams: [p("BT TIMEOUT", 8, 1, 30, "s", 0, 1), p("USB TIMEOUT", 5, 1, 30, "s", 0, 1), p("RETRY", 3, 0, 10, "", 0, 1), p("TX COALESCE", 18, 0, 100, "ms", 0, 1), p("RX FILTER", 12, 0, 100, "ms", 0, 1)],
                midTitle: "GLOBAL SAFETY", midNote: "Limits applied across every output",
                midParams: [p("MAX LEVEL", 6, -12, 12, "dB", 1, 0.1), p("MUTE RAMP", 24, 0, 500, "ms", 0, 1), p("START RAMP", 80, 0, 1000, "ms", 0, 10), p("PEAK HOLD", 1.2, 0, 5, "s", 1, 0.1), p("CLIP WARN", -1, -12, 0, "dB", 1, 0.1)],
                rightTitle: "WORKFLOW", rightNote: "Preset and session behavior",
                rightParams: [p("AUTOSAVE", 30, 5, 300, "s", 0, 5), p("HISTORY", 50, 0, 200, "", 0, 1), p("FADE LOAD", 120, 0, 1000, "ms", 0, 10), p("CONFIRM", 1, 0, 1, "", 0, 1)],
                modes: ["AUTO CONNECT", "BT PRIORITY", "USB PRIORITY", "MANUAL"]
            }
        }
    }

    onSectionIndexChanged: activeMode = profile.modes.length ? profile.modes[0] : ""
    Component.onCompleted: activeMode = profile.modes.length ? profile.modes[0] : ""

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        StudioPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 278
            Layout.minimumHeight: 250

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Rectangle {
                        Layout.preferredWidth: 36
                        Layout.preferredHeight: 36
                        radius: 8
                        color: "#0B1217"
                        border.width: 1
                        border.color: root.profile.accent
                        LucideIcon {
                            anchors.centerIn: parent
                            width: 18
                            height: 18
                            name: root.profile.icon
                            color: root.profile.accent
                            strokeWidth: 1.9
                        }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: -1
                        Text {
                            text: root.profile.kicker
                            color: Theme.textDim
                            font.family: Theme.fontFamily
                            font.pixelSize: 8
                            font.weight: Font.DemiBold
                            font.letterSpacing: 1.05
                        }
                        Text {
                            text: root.profile.title + "  ·  " + root.profile.heroTitle
                            color: Theme.text
                            font.family: Theme.fontFamily
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                        }
                        Text {
                            text: root.profile.subtitle
                            color: Theme.textDim
                            font.family: Theme.fontFamily
                            font.pixelSize: 9
                        }
                    }
                    Rectangle {
                        Layout.preferredWidth: 72
                        Layout.preferredHeight: 26
                        radius: 6
                        color: "#0A1212"
                        border.width: 1
                        border.color: root.profile.accent
                        opacity: 0.95
                        Text {
                            anchors.centerIn: parent
                            text: "ACTIVE"
                            color: root.profile.accent
                            font.family: Theme.fontFamily
                            font.pixelSize: 8
                            font.weight: Font.Bold
                            font.letterSpacing: 0.8
                        }
                    }
                    SoftButton { Layout.preferredWidth: 66; text: "RESET"; compact: true }
                }

                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.borderSoft; opacity: 0.65 }

                GridLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    columns: 2
                    columnSpacing: 18
                    rowSpacing: 4

                    Repeater {
                        model: root.profile.heroParams
                        delegate: ParameterSlider {
                            required property var modelData
                            property real localValue: Number(modelData.value)
                            Layout.fillWidth: true
                            label: modelData.label
                            value: localValue
                            from: modelData.from
                            to: modelData.to
                            step: modelData.step
                            decimals: modelData.decimals
                            unit: modelData.unit
                            defaultValue: modelData.value
                            logarithmic: modelData.logarithmic
                            accentColor: root.profile.accent
                            onValueEdited: function(v) { localValue = v }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            Repeater {
                model: [
                    { title: root.profile.leftTitle, note: root.profile.leftNote, params: root.profile.leftParams },
                    { title: root.profile.midTitle, note: root.profile.midNote, params: root.profile.midParams }
                ]
                delegate: StudioPanel {
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumWidth: 300

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 13
                        spacing: 7
                        Text { text: modelData.title; color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: 9; font.weight: Font.Bold; font.letterSpacing: 0.7 }
                        Text { text: modelData.note; color: Theme.textDim; font.family: Theme.fontFamily; font.pixelSize: 8 }
                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.borderSoft; opacity: 0.62 }
                        Repeater {
                            model: modelData.params
                            delegate: ParameterSlider {
                                required property var modelData
                                property real localValue: Number(modelData.value)
                                Layout.fillWidth: true
                                label: modelData.label
                                value: localValue
                                from: modelData.from
                                to: modelData.to
                                step: modelData.step
                                decimals: modelData.decimals
                                unit: modelData.unit
                                defaultValue: modelData.value
                                logarithmic: modelData.logarithmic
                                accentColor: root.profile.accent
                                onValueEdited: function(v) { localValue = v }
                            }
                        }
                        Item { Layout.fillHeight: true }
                    }
                }
            }

            StudioPanel {
                Layout.preferredWidth: 330
                Layout.minimumWidth: 300
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 13
                    spacing: 7
                    Text { text: root.profile.rightTitle; color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: 9; font.weight: Font.Bold; font.letterSpacing: 0.7 }
                    Text { text: root.profile.rightNote; color: Theme.textDim; font.family: Theme.fontFamily; font.pixelSize: 8 }
                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.borderSoft; opacity: 0.62 }

                    Text { text: "MODE"; color: Theme.textDim; font.family: Theme.fontFamily; font.pixelSize: 8; font.weight: Font.DemiBold; font.letterSpacing: 0.55 }
                    StudioComboBox {
                        Layout.fillWidth: true
                        model: root.profile.modes
                        value: root.activeMode
                        accentColor: root.profile.accent
                        onValueEdited: function(v) { root.activeMode = v }
                    }

                    Repeater {
                        model: root.profile.rightParams
                        delegate: ParameterSlider {
                            required property var modelData
                            property real localValue: Number(modelData.value)
                            Layout.fillWidth: true
                            label: modelData.label
                            value: localValue
                            from: modelData.from
                            to: modelData.to
                            step: modelData.step
                            decimals: modelData.decimals
                            unit: modelData.unit
                            defaultValue: modelData.value
                            logarithmic: modelData.logarithmic
                            accentColor: root.profile.accent
                            onValueEdited: function(v) { localValue = v }
                        }
                    }

                    Item { Layout.fillHeight: true }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        SoftButton { Layout.fillWidth: true; text: "BYPASS"; compact: true }
                        SoftButton { Layout.fillWidth: true; text: "MUTE"; compact: true; danger: true }
                    }
                }
            }
        }
    }
}
