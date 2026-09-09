import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    required property var studioEngine
    required property var deviceManager
    visible: true
    width: 1484
    height: 920
    minimumWidth: 1260
    minimumHeight: 800
    title: "SonKuPik K500 — Karaoke Processor"
    color: Theme.bg

    readonly property int lowerRackHeight: 304
    property int selectedSection: 0

    // CRASH_SAFE_SECTION_RELOAD_V1
    // Processor sections use different EQ model shapes (Mic=10, Main=7,
    // Reverb/Echo/etc=5). Never hot-swap those models inside one live QML
    // graph instance. Destroy the old workspace first, then create the next
    // workspace on the following event-loop turn. StudioEngine remains alive,
    // so this changes UI lifetime only and never device/protocol state.
    function reloadProcessorWorkspace() {
        if (root.selectedSection === 0) {
            sectionWorkspaceLoader.active = false
            return
        }
        sectionWorkspaceLoader.active = false
        Qt.callLater(function() {
            if (root.selectedSection !== 0)
                sectionWorkspaceLoader.active = true
        })
    }
    onSelectedSectionChanged: reloadProcessorWorkspace()

    background: Rectangle {
        gradient: Gradient {
            GradientStop { position:0;color:"#0C1116" }
            GradientStop { position:.48;color:Theme.bg }
            GradientStop { position:1;color:"#04070A" }
        }
    }

    Component {
        id: sectionWorkspaceComponent
        SectionWorkspace {
            engine: root.studioEngine
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        TopBar {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            Layout.minimumHeight: 52
            Layout.maximumHeight: 52
            deviceManager: root.deviceManager
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            SectionDrawer {
                Layout.preferredWidth: 170
                Layout.minimumWidth: 170
                Layout.maximumWidth: 170
                Layout.fillHeight: true
                selectedSection: root.selectedSection
                onSectionSelected: function(index) { root.selectedSection = index }
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: root.selectedSection === 0 ? 0 : 1

                Item {
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 12

                        EqGraph {
                            bandModel: root.studioEngine.musicEqBands
                            engine: root.studioEngine
                            hpfFreq: root.studioEngine.hpfHz
                            lpfFreq: root.studioEngine.lpfHz
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.minimumHeight: 0
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: root.lowerRackHeight
                            Layout.minimumHeight: root.lowerRackHeight
                            Layout.maximumHeight: root.lowerRackHeight
                            spacing: 12

                            MusicInputPanel {
                                engine: root.studioEngine
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.preferredWidth: 557
                                Layout.minimumWidth: 440
                            }

                            MusicTonePanel {
                                engine: root.studioEngine
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.preferredWidth: 316
                                Layout.minimumWidth: 224
                            }

                            FilterPanel {
                                engine: root.studioEngine
                                Layout.preferredWidth: 180
                                Layout.minimumWidth: 180
                                Layout.maximumWidth: 180
                                Layout.fillHeight: true
                            }

                            MasterStripPanel {
                                engine: root.studioEngine
                                Layout.preferredWidth: 188
                                Layout.minimumWidth: 188
                                Layout.maximumWidth: 188
                                Layout.fillHeight: true
                            }
                        }
                    }
                }

                Loader {
                    id: sectionWorkspaceLoader
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    active: false
                    sourceComponent: sectionWorkspaceComponent
                    onLoaded: {
                        // Deliberately imperative: the old workspace must never
                        // observe the next sectionIndex before it is destroyed.
                        if (item)
                            item.sectionIndex = root.selectedSection
                    }
                }
            }
        }
    }

    // P1_MIC_EQ_LINK_UI_BRIDGE_V1
    // SectionEqGraph owns the local toggle and SectionWorkspace mirrors it.
    // Keep the hardware path at the application boundary through StudioEngine.
    Connections {
        target: sectionWorkspaceLoader.item
        enabled: target !== null
        function onMicEqLinkedChanged() {
            if (root.selectedSection === 1)
                root.studioEngine.editDevicePath("mic.eqLink", target.micEqLinked)
        }
    }
}
