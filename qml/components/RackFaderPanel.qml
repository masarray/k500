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

    // P1_RACK_FADER_LIVE_BRIDGE_V1
    // Reusable rack panels resolve the owning StudioEngine through the visual
    // parent chain. They never reference Controller/DeviceManager/I/O directly.
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
        if (!section.length) return "" // Reverb/Echo detail writes are not verified.

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
            Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:1;color:Theme.borderSoft;opacity:.72 }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 10
            Layout.rightMargin: 10
            Layout.topMargin: 10
            Layout.bottomMargin: 10
            spacing: 2

            Repeater {
                model: root.channels
                delegate: Item {
                    id: channel
                    required property int index
                    required property var modelData
                    Layout.fillWidth: !root.compactCluster
                    Layout.preferredWidth: root.compactCluster ? 62 : -1
                    Layout.minimumWidth: root.compactCluster ? 62 : 48
                    Layout.fillHeight: true
                    property real localValue: Number(modelData.value)
                    readonly property bool selected: root.selectedFader === channel.index

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 4

                        Item {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: Math.max(48,channel.width-4)
                            Layout.preferredHeight: 25
                            Column {
                                anchors.centerIn: parent
                                spacing: -1
                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: String(channel.modelData.label || "")
                                    color: rackFader.highlighted ? rackFader.accentColor : Theme.textDim
                                    style: rackFader.highlighted ? Text.Outline : Text.Normal
                                    styleColor: rackFader.highlighted ? Qt.rgba(rackFader.accentColor.r,rackFader.accentColor.g,rackFader.accentColor.b,.34) : "transparent"
                                    font.family: Theme.monoFamily
                                    font.pixelSize: 9
                                    font.weight: rackFader.highlighted ? Font.Bold : Font.DemiBold
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
                        }

                        Item { Layout.preferredHeight: 4 }

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
                                root.dispatchLive(channel.modelData.label, v)
                            }
                        }

                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: 58
                            Layout.preferredHeight: 23
                            radius: 8
                            color: rackFader.highlighted ? "#081013" : "#05080A"
                            border.width: 1
                            border.color: rackFader.highlighted ? root.accentColor : "#020304"
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
                            Behavior on border.color { ColorAnimation { duration: 75 } }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }
            }

            Item { visible: root.compactCluster; Layout.fillWidth: true; Layout.fillHeight: true }
        }
    }
}
