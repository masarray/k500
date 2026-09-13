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
    readonly property int rightPanelWidth: 216
    property int selectedSection: 0

    // USER_STORAGE_LAYOUT_V1 — executable/runtime stays in Program Files while
    // user-owned K500 data gets a stable, discoverable Documents home.
    AppStorageManager {
        id: appStorage
    }

    // SMART_APP_UPDATE_V1 — checks stable GitHub Releases on a restrained
    // cadence; user consent is required before download/install begins.
    AppUpdateManager {
        id: appUpdater
    }

    Component.onCompleted: {
        appStorage.ensureUserStorage(root.deviceManager ? root.deviceManager.presetFileBridge : null)
        appUpdater.startAutomaticCheck()
    }

    background: Rectangle {
        gradient: Gradient {
            GradientStop { position:0;color:"#0C1116" }
            GradientStop { position:.48;color:Theme.bg }
            GradientStop { position:1;color:"#04070A" }
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
            engine: root.studioEngine
            onAboutRequested: aboutDialog.open()
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

                            // RIGHT_COLUMN_WIDTH_PARITY_V1
                            // Music follows the same two fixed right columns as Mic:
                            // crossover/filter panel 216 px + Master Strip 216 px.
                            FilterPanel {
                                engine: root.studioEngine
                                Layout.preferredWidth: root.rightPanelWidth
                                Layout.minimumWidth: root.rightPanelWidth
                                Layout.maximumWidth: root.rightPanelWidth
                                Layout.fillHeight: true
                            }

                            MasterStripPanel {
                                engine: root.studioEngine
                                Layout.preferredWidth: root.rightPanelWidth
                                Layout.minimumWidth: root.rightPanelWidth
                                Layout.maximumWidth: root.rightPanelWidth
                                Layout.fillHeight: true
                            }
                        }
                    }
                }

                SectionWorkspace {
                    id: sectionWorkspace
                    engine: root.studioEngine
                    sectionIndex: root.selectedSection
                }
            }
        }
    }

    // ABOUT_FLOATING_CARD_V1 — window-owned so it centers over the complete
    // application, not merely over the toolbar or current processor panel.
    AboutDialog {
        id: aboutDialog
    }

    UpdateDialog {
        id: updateDialog
        manager: appUpdater
    }

    // P1_MIC_EQ_LINK_UI_BRIDGE_V1
    // SectionEqGraph owns the local toggle and SectionWorkspace mirrors it.
    // Keep the hardware path at the application boundary through StudioEngine.
    Connections {
        target: sectionWorkspace
        function onMicEqLinkedChanged() {
            root.studioEngine.editDevicePath("mic.eqLink", sectionWorkspace.micEqLinked)
        }
    }
}
