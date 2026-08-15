import QtQuick
import QtQuick.Layouts

Item {
    id: root

    required property var engine
    property int sectionIndex: 1
    property int micChannel: 0
    property bool micEqLinked: false
    readonly property int lowerRackHeight: 304

    function activeEqModel() {
        switch (sectionIndex) {
        case 1: return micChannel === 0 ? micAModel : micBModel
        case 2: return reverbModel
        case 3: return echoModel
        case 4: return mainModel
        case 5: return surroundModel
        case 6: return centerModel
        case 7: return subModel
        default: return micAModel
        }
    }
    function activeEqLabel() {
        switch (sectionIndex) {
        case 1: return micChannel === 0 ? "Mic A" : "Mic B"
        case 2: return "Reverb"
        case 3: return "Echo"
        case 4: return "Main"
        case 5: return "Surround"
        case 6: return "Center"
        case 7: return "Subwoofer"
        default: return "Mic A"
        }
    }

    LocalEqModel { id: micAModel; bandCount: 10; hpfHz: 20; lpfHz: 20000 }
    LocalEqModel { id: micBModel; bandCount: 10; hpfHz: 20; lpfHz: 20000 }
    LocalEqModel { id: reverbModel; bandCount: 5; hpfHz: 20; lpfHz: 20000 }
    LocalEqModel { id: echoModel; bandCount: 5; hpfHz: 20; lpfHz: 20000 }
    LocalEqModel { id: mainModel; bandCount: 7; hpfHz: 20; lpfHz: 20000 }
    LocalEqModel { id: surroundModel; bandCount: 5; hpfHz: 20; lpfHz: 20000 }
    LocalEqModel { id: centerModel; bandCount: 5; hpfHz: 20; lpfHz: 20000 }
    LocalEqModel { id: subModel; bandCount: 5; hpfHz: 20; lpfHz: 20000 }

    StackLayout {
        anchors.fill: parent
        currentIndex: root.sectionIndex === 8 ? 1 : 0

        Item {
            ColumnLayout {
                anchors.fill: parent
                spacing: 10

                SectionEqGraph {
                    bandModel: root.activeEqModel()
                    sectionLabel: root.activeEqLabel()
                    showMicSelector: root.sectionIndex === 1
                    micChannel: root.micChannel
                    eqLinked: root.micEqLinked
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 430
                    onMicChannelRequested: function(channel) { root.micChannel = channel }
                    onEqLinkRequested: function(linked) { root.micEqLinked = linked }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: root.lowerRackHeight
                    Layout.minimumHeight: root.lowerRackHeight
                    Layout.maximumHeight: root.lowerRackHeight
                    spacing: 10

                    StackLayout {
                        id: lowerStack
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        currentIndex: Math.max(0, Math.min(6, root.sectionIndex - 1))

                        // MIC — exact web page structure: Mic Inputs + Vocal Dynamics + Band Limits.
                        Item {
                            RowLayout {
                                anchors.fill: parent
                                spacing: 10
                                RackFaderPanel {
                                    Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 290
                                    eyebrow: "INPUT MIXER"; title: "Mic Inputs"
                                    channels: [
                                        {label:"MIC A",value:96,from:0,to:100,step:1,unit:"",decimals:0},
                                        {label:"MIC B",value:96,from:0,to:100,step:1,unit:"",decimals:0},
                                        {label:"FBX",badge:"A+B",value:7,from:0,to:20,step:1,unit:"",decimals:0}
                                    ]
                                }
                                RackDynamicsPanel {
                                    Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 520
                                    title: "Vocal Dynamics"; includeGate: true
                                    gate: -70; threshold: -12; ratio: 3; attack: 10; release: 200
                                }
                                RackFilterPanel {
                                    Layout.preferredWidth: 245; Layout.fillHeight: true
                                    eyebrow: "FILTERS"; title: "Band Limits"
                                    fields: [
                                        {label:"HPF",value:20,from:20,to:20000,step:1,unit:"Hz",decimals:0},
                                        {label:"LPF",value:20000,from:20,to:20000,step:1,unit:"Hz",decimals:0}
                                    ]
                                    hpType: "HP LR 24"; lpType: "LP LR 24"
                                }
                            }
                        }

                        // REVERB — exact web structure: Room Engine + Tone.
                        Item {
                            RowLayout {
                                anchors.fill: parent; spacing: 10
                                RackFaderPanel {
                                    Layout.fillWidth:true; Layout.fillHeight:true; Layout.preferredWidth:440
                                    eyebrow:"ROOM ENGINE"; title:"Reverb"
                                    channels:[
                                        {label:"LEVEL",value:24,from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"DECAY",value:1800,from:100,to:5000,step:10,unit:"ms",decimals:0},
                                        {label:"PRE",value:28,from:0,to:300,step:1,unit:"ms",decimals:0}
                                    ]
                                }
                                RackFilterPanel {
                                    Layout.fillWidth:true; Layout.fillHeight:true; Layout.preferredWidth:420
                                    eyebrow:"EFFECT FILTERS"; title:"Tone"
                                    fields:[
                                        {label:"HPF",value:80,from:20,to:20000,step:1,unit:"Hz",decimals:0},
                                        {label:"LPF",value:12000,from:20,to:20000,step:1,unit:"Hz",decimals:0}
                                    ]
                                    hpType:"HP LR 24"; lpType:"LP LR 24"
                                }
                            }
                        }

                        // ECHO — exact web structure: Delay Engine + Tone.
                        Item {
                            RowLayout {
                                anchors.fill: parent; spacing: 10
                                RackFaderPanel {
                                    Layout.fillWidth:true; Layout.fillHeight:true; Layout.preferredWidth:440
                                    eyebrow:"DELAY ENGINE"; title:"Echo"
                                    channels:[
                                        {label:"LEVEL",value:22,from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"REPEAT",value:28,from:0,to:100,step:1,unit:"",decimals:0},
                                        {label:"DELAY",value:320,from:0,to:1000,step:1,unit:"ms",decimals:0}
                                    ]
                                }
                                RackFilterPanel {
                                    Layout.fillWidth:true; Layout.fillHeight:true; Layout.preferredWidth:420
                                    eyebrow:"DELAY FILTERS"; title:"Tone"
                                    fields:[
                                        {label:"HPF",value:100,from:20,to:20000,step:1,unit:"Hz",decimals:0},
                                        {label:"LPF",value:10000,from:20,to:20000,step:1,unit:"Hz",decimals:0}
                                    ]
                                    hpType:"HP LR 24"; lpType:"LP LR 24"
                                }
                            }
                        }

                        // MAIN — exact web output-page structure.
                        Item {
                            RowLayout {
                                anchors.fill: parent; spacing: 10
                                RackFaderPanel {
                                    Layout.fillWidth:true; Layout.fillHeight:true; Layout.preferredWidth:430
                                    eyebrow:"FRONT OUTPUT"; title:"Main Bus"
                                    channels:[
                                        {label:"L",value:0,from:-37.5,to:24,step:.5,unit:"dB",decimals:1},
                                        {label:"R",value:0,from:-37.5,to:24,step:.5,unit:"dB",decimals:1},
                                        {label:"MIC",value:100,from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"MUSIC",value:100,from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"REV",value:35,from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"ECHO",value:35,from:0,to:100,step:1,unit:"%",decimals:0}
                                    ]
                                }
                                RackDynamicsPanel { Layout.fillWidth:true; Layout.fillHeight:true; Layout.preferredWidth:430; title:"Output Compressor"; threshold:-6; ratio:3; attack:10; release:200 }
                                RackFilterPanel {
                                    Layout.preferredWidth:245; Layout.fillHeight:true
                                    eyebrow:"CROSSOVER"; title:"Band Limits / Delay"
                                    fields:[
                                        {label:"HPF",value:20,from:20,to:20000,step:1,unit:"Hz",decimals:0},
                                        {label:"LPF",value:20000,from:20,to:20000,step:1,unit:"Hz",decimals:0}
                                    ]
                                    hpType:"HP LR 24"; lpType:"LP LR 24"
                                }
                            }
                        }

                        // SURROUND — same output architecture, with channel delays.
                        Item {
                            RowLayout {
                                anchors.fill: parent; spacing: 10
                                RackFaderPanel {
                                    Layout.fillWidth:true; Layout.fillHeight:true; Layout.preferredWidth:430
                                    eyebrow:"REAR FIELD"; title:"Surround Bus"
                                    channels:[
                                        {label:"L",value:-3,from:-37.5,to:24,step:.5,unit:"dB",decimals:1},
                                        {label:"R",value:-3,from:-37.5,to:24,step:.5,unit:"dB",decimals:1},
                                        {label:"MIC",value:70,from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"MUSIC",value:70,from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"REV",value:45,from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"ECHO",value:30,from:0,to:100,step:1,unit:"%",decimals:0}
                                    ]
                                }
                                RackDynamicsPanel { Layout.fillWidth:true; Layout.fillHeight:true; Layout.preferredWidth:430; title:"Output Compressor"; threshold:-8; ratio:3; attack:12; release:220 }
                                RackFilterPanel {
                                    Layout.preferredWidth:245; Layout.fillHeight:true
                                    eyebrow:"CROSSOVER"; title:"Band Limits / Delay"
                                    fields:[
                                        {label:"L DELAY",value:18,from:0,to:50,step:1,unit:"ms",decimals:0},
                                        {label:"R DELAY",value:18,from:0,to:50,step:1,unit:"ms",decimals:0},
                                        {label:"HPF",value:80,from:20,to:20000,step:1,unit:"Hz",decimals:0},
                                        {label:"LPF",value:16000,from:20,to:20000,step:1,unit:"Hz",decimals:0}
                                    ]
                                    hpType:"HP LR 24"; lpType:"LP LR 24"
                                }
                            }
                        }

                        // CENTER — same output architecture.
                        Item {
                            RowLayout {
                                anchors.fill: parent; spacing: 10
                                RackFaderPanel {
                                    Layout.fillWidth:true; Layout.fillHeight:true; Layout.preferredWidth:430
                                    eyebrow:"VOCAL ANCHOR"; title:"Center Bus"
                                    channels:[
                                        {label:"CTR",value:-3,from:-37.5,to:24,step:.5,unit:"dB",decimals:1},
                                        {label:"MIC",value:90,from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"MUSIC",value:50,from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"REV",value:25,from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"ECHO",value:20,from:0,to:100,step:1,unit:"%",decimals:0}
                                    ]
                                }
                                RackDynamicsPanel { Layout.fillWidth:true; Layout.fillHeight:true; Layout.preferredWidth:430; title:"Output Compressor"; threshold:-8; ratio:3; attack:12; release:220 }
                                RackFilterPanel {
                                    Layout.preferredWidth:245; Layout.fillHeight:true
                                    eyebrow:"CROSSOVER"; title:"Band Limits / Delay"
                                    fields:[
                                        {label:"HPF",value:80,from:20,to:20000,step:1,unit:"Hz",decimals:0},
                                        {label:"LPF",value:16000,from:20,to:20000,step:1,unit:"Hz",decimals:0}
                                    ]
                                    hpType:"HP LR 24"; lpType:"LP LR 24"
                                }
                            }
                        }

                        // SUB — same output architecture.
                        Item {
                            RowLayout {
                                anchors.fill: parent; spacing: 10
                                RackFaderPanel {
                                    Layout.fillWidth:true; Layout.fillHeight:true; Layout.preferredWidth:430
                                    eyebrow:"BASS MANAGEMENT"; title:"Subwoofer Bus"
                                    channels:[
                                        {label:"SUB",value:-3,from:-37.5,to:24,step:.5,unit:"dB",decimals:1},
                                        {label:"MIC",value:25,from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"MUSIC",value:100,from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"REV",value:10,from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"ECHO",value:10,from:0,to:100,step:1,unit:"%",decimals:0}
                                    ]
                                }
                                RackDynamicsPanel { Layout.fillWidth:true; Layout.fillHeight:true; Layout.preferredWidth:430; title:"Output Compressor"; threshold:-4; ratio:4; attack:18; release:260 }
                                RackFilterPanel {
                                    Layout.preferredWidth:245; Layout.fillHeight:true
                                    eyebrow:"CROSSOVER"; title:"Band Limits / Delay"
                                    fields:[
                                        {label:"HPF",value:20,from:20,to:20000,step:1,unit:"Hz",decimals:0},
                                        {label:"LPF",value:120,from:20,to:20000,step:1,unit:"Hz",decimals:0}
                                    ]
                                    hpType:"HP LR 24"; lpType:"LP LR 24"
                                }
                            }
                        }
                    }

                    MasterStripPanel {
                        engine: root.engine
                        Layout.preferredWidth: 212
                        Layout.minimumWidth: 202
                        Layout.maximumWidth: 226
                        Layout.fillHeight: true
                    }
                }
            }
        }

        SystemWorkspace {
            engine: root.engine
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
