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
    // P3_AUTHORITATIVE_RECONCILIATION_UI_BARRIER_V1 — during initial sync or
    // P3 verification the central editor is read-only. This prevents a user
    // gesture from being visually accepted while Controller LIVE is paused.
    readonly property bool deviceSyncBarrier: deviceManager.status === "syncing"
    property int selectedSection: 0

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
            enabled: !root.deviceSyncBarrier
            opacity: enabled ? 1.0 : 0.72

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

    // SMART_UPDATE_UI_V1 — update discovery is silent on startup. Only a newer
    // public stable release opens the premium prompt; network failure never
    // interrupts K500 control or produces a startup warning. The dialog also
    // observes the preset transaction coordinator so a permanent device write
    // can never be interrupted by an app replacement initiated from the UI.
    UpdateDialog {
        id: updateDialog
        updateManager: AppUpdater
        deviceManager: root.deviceManager
    }

    Connections {
        target: AppUpdater
        function onUpdateChanged() {
            if (AppUpdater.updateAvailable && !updateDialog.opened)
                updateDialog.open()
        }
    }

    // UPDATE_DEVICE_TRANSACTION_BARRIER_P1 — pass the authoritative coordinator
    // state to the backend. The backend checks again AFTER the download finishes,
    // not only when the Update button was initially enabled.
    Component.onCompleted: AppUpdater.setDeviceTransactionBusy(
        !!root.deviceManager.presetManager && root.deviceManager.presetManager.busy)
    Connections {
        target: root.deviceManager ? root.deviceManager.presetManager : null
        function onBusyChanged() {
            AppUpdater.setDeviceTransactionBusy(root.deviceManager.presetManager.busy)
        }
    }

    // STARTUP_UPDATE_DISCOVERY_V1 — let the control surface paint and device
    // startup settle before the first network request. Long-running studio
    // sessions re-check every six hours; AppUpdateManager applies its own
    // successful-check throttle as the second guard against noisy polling.
    Timer {
        id: initialUpdateCheck
        interval: 2500
        repeat: false
        running: true
        onTriggered: AppUpdater.checkForUpdates(false)
    }
    Timer {
        interval: 6 * 60 * 60 * 1000
        repeat: true
        running: true
        onTriggered: AppUpdater.checkForUpdates(false)
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
