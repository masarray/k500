import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    required property var studioEngine
    visible: true
    width: 1484
    height: 920
    minimumWidth: 1260
    minimumHeight: 800
    title: "SONKUPIK STUDIO — Karaoke Processor"
    color: Theme.bg

    readonly property int lowerRackHeight: 304
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

                SectionWorkspace {
                    engine: root.studioEngine
                    sectionIndex: root.selectedSection
                }
            }
        }
    }
}
