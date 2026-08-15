import QtQuick
import QtQuick.Layouts

Item {
    id: root

    required property var engine
    property int sectionIndex: 1
    property int micChannel: 0
    property bool micEqLinked: false
    readonly property int lowerRackHeight: 304
    readonly property int masterWidth: 188

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

    // Values mirror the web DEFAULT FLAT screenshots so PEQ + lower rack can
    // be reviewed against the same state instead of against fabricated data.
    LocalEqModel { id: micAModel; bandCount:10; defaultHpfHz:20; defaultLpfHz:20000; defaultHpType:"HP LR 24"; defaultLpType:"LP LR 24" }
    LocalEqModel { id: micBModel; bandCount:10; defaultHpfHz:20; defaultLpfHz:20000; defaultHpType:"HP LR 24"; defaultLpType:"LP LR 24" }
    LocalEqModel { id: reverbModel; bandCount:5; defaultHpfHz:217; defaultLpfHz:12000; defaultHpType:"HP Butter 12"; defaultLpType:"LP Butter 12" }
    LocalEqModel { id: echoModel; bandCount:5; defaultHpfHz:700; defaultLpfHz:4400; defaultHpType:"HP Butter 12"; defaultLpType:"LP Butter 12" }
    LocalEqModel { id: mainModel; bandCount:7; defaultHpfHz:20; defaultLpfHz:20000; defaultHpType:"HP Butter 12"; defaultLpType:"LP Butter 12" }
    LocalEqModel { id: surroundModel; bandCount:5; defaultHpfHz:20; defaultLpfHz:20000; defaultHpType:"HP Bessel 12"; defaultLpType:"LP Bessel 12" }
    LocalEqModel { id: centerModel; bandCount:5; defaultHpfHz:20; defaultLpfHz:20000; defaultHpType:"HP Butter 12"; defaultLpType:"LP Butter 12" }
    LocalEqModel { id: subModel; bandCount:5; defaultHpfHz:40; defaultLpfHz:95; defaultHpType:"HP Butter 24"; defaultLpType:"LP Butter 24" }

    StackLayout {
        anchors.fill: parent
        currentIndex: root.sectionIndex === 8 ? 1 : 0

        Item {
            ColumnLayout {
                anchors.fill: parent
                spacing: 12

                SectionEqGraph {
                    bandModel: root.activeEqModel()
                    sectionLabel: root.activeEqLabel()
                    showMicSelector: root.sectionIndex === 1
                    micChannel: root.micChannel
                    eqLinked: root.micEqLinked
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 0
                    onMicChannelRequested: function(channel) { root.micChannel = channel }
                    onEqLinkRequested: function(linked) { root.micEqLinked = linked }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: root.lowerRackHeight
                    Layout.minimumHeight: root.lowerRackHeight
                    Layout.maximumHeight: root.lowerRackHeight
                    spacing: 12

                    StackLayout {
                        id: lowerStack
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        currentIndex: Math.max(0, Math.min(6, root.sectionIndex - 1))

                        // MIC: web geometry ≈ .66fr / 1.5fr / 184 px.
                        Item {
                            RowLayout {
                                anchors.fill: parent
                                spacing: 12

                                RackFaderPanel {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    Layout.preferredWidth: 266
                                    Layout.minimumWidth: 220
                                    title: "Mic Inputs"
                                    channels: [
                                        {label:"MIC A",value:96,from:0,to:100,step:1,unit:"",decimals:0},
                                        {label:"MIC B",value:96,from:0,to:100,step:1,unit:"",decimals:0},
                                        {label:"FBX",badge:"A+B",value:7,from:0,to:20,step:1,unit:"",decimals:0}
                                    ]
                                }
                                RackDynamicsPanel {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    Layout.preferredWidth: 604
                                    Layout.minimumWidth: 390
                                    title: "Vocal Dynamics"
                                    includeGate: true
                                    gate: -70; threshold: -12; ratio: 3; attack: 10; release: 200
                                }
                                RackFilterPanel {
                                    Layout.preferredWidth: 184
                                    Layout.minimumWidth: 184
                                    Layout.maximumWidth: 184
                                    Layout.fillHeight: true
                                    title: "Band Limits"
                                    fields: [
                                        {label:"HPF",value:root.activeEqModel().hpfHz,from:20,to:20000,step:1,unit:"Hz",decimals:0},
                                        {label:"LPF",value:root.activeEqModel().lpfHz,from:20,to:20000,step:1,unit:"Hz",decimals:0}
                                    ]
                                    hpType: root.activeEqModel().hpType
                                    lpType: root.activeEqModel().lpType
                                    onFieldEdited: function(index,value){ if(index===0)root.activeEqModel().setHpfHz(value);else root.activeEqModel().setLpfHz(value) }
                                    onHpTypeEdited: function(value){ root.activeEqModel().setHpType(value) }
                                    onLpTypeEdited: function(value){ root.activeEqModel().setLpType(value) }
                                }
                            }
                        }

                        // REVERB: web uses one wide room-engine panel + 208 px tone.
                        Item {
                            RowLayout {
                                anchors.fill: parent
                                spacing: 12
                                RackFaderPanel {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    Layout.minimumWidth: 330
                                    title: "Reverb"
                                    channels: [
                                        {label:"LEVEL",value:100,from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"DECAY",value:1575,from:100,to:5000,step:5,unit:"ms",decimals:0},
                                        {label:"PRE",value:25,from:0,to:300,step:1,unit:"ms",decimals:0}
                                    ]
                                }
                                RackFilterPanel {
                                    Layout.preferredWidth: 208
                                    Layout.minimumWidth: 208
                                    Layout.maximumWidth: 208
                                    Layout.fillHeight: true
                                    title: "Tone"
                                    fields: [
                                        {label:"HPF",value:reverbModel.hpfHz,from:20,to:20000,step:1,unit:"Hz",decimals:0},
                                        {label:"LPF",value:reverbModel.lpfHz,from:20,to:20000,step:1,unit:"Hz",decimals:0}
                                    ]
                                    hpType: reverbModel.hpType
                                    lpType: reverbModel.lpType
                                    onFieldEdited: function(index,value){ if(index===0)reverbModel.setHpfHz(value);else reverbModel.setLpfHz(value) }
                                    onHpTypeEdited: function(value){ reverbModel.setHpType(value) }
                                    onLpTypeEdited: function(value){ reverbModel.setLpType(value) }
                                }
                            }
                        }

                        // ECHO: web uses one wide delay-engine panel + 208 px tone.
                        Item {
                            RowLayout {
                                anchors.fill: parent
                                spacing: 12
                                RackFaderPanel {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    Layout.minimumWidth: 330
                                    title: "Echo"
                                    channels: [
                                        {label:"LEVEL",value:100,from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"REPEAT",value:12,from:0,to:100,step:1,unit:"",decimals:0},
                                        {label:"DELAY",value:400,from:0,to:1000,step:1,unit:"ms",decimals:0}
                                    ]
                                }
                                RackFilterPanel {
                                    Layout.preferredWidth: 208
                                    Layout.minimumWidth: 208
                                    Layout.maximumWidth: 208
                                    Layout.fillHeight: true
                                    title: "Tone"
                                    fields: [
                                        {label:"HPF",value:echoModel.hpfHz,from:20,to:20000,step:1,unit:"Hz",decimals:0},
                                        {label:"LPF",value:echoModel.lpfHz,from:20,to:20000,step:1,unit:"Hz",decimals:0}
                                    ]
                                    hpType: echoModel.hpType
                                    lpType: echoModel.lpType
                                    onFieldEdited: function(index,value){ if(index===0)echoModel.setHpfHz(value);else echoModel.setLpfHz(value) }
                                    onHpTypeEdited: function(value){ echoModel.setHpType(value) }
                                    onLpTypeEdited: function(value){ echoModel.setLpType(value) }
                                }
                            }
                        }

                        // MAIN: web output-page ≈ 1.04fr / 1.34fr / 18vw.
                        Item {
                            RowLayout {
                                anchors.fill: parent
                                spacing: 12
                                RackFaderPanel {
                                    Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 344; Layout.minimumWidth: 300
                                    title: "Main Bus"
                                    channels: [
                                        {label:"L",value:12,from:-37.5,to:24,step:.5,unit:"dB",decimals:1},
                                        {label:"R",value:12,from:-37.5,to:24,step:.5,unit:"dB",decimals:1},
                                        {label:"MIC",value:100,from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"MUSIC",value:100,from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"REV",value:90,from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"ECHO",value:95,from:0,to:100,step:1,unit:"%",decimals:0}
                                    ]
                                }
                                RackDynamicsPanel {
                                    Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 443; Layout.minimumWidth: 320
                                    title: "Output Compressor"; threshold:-3; ratio:18; attack:7; release:100
                                }
                                RackFilterPanel {
                                    Layout.preferredWidth: 267; Layout.minimumWidth:238; Layout.maximumWidth:292; Layout.fillHeight:true
                                    title: "Band Limits / Delay"
                                    fields:[
                                        {label:"HPF",value:mainModel.hpfHz,from:20,to:20000,step:1,unit:"Hz",decimals:0},
                                        {label:"LPF",value:mainModel.lpfHz,from:20,to:20000,step:1,unit:"Hz",decimals:0}
                                    ]
                                    hpType:mainModel.hpType; lpType:mainModel.lpType
                                    onFieldEdited:function(index,value){if(index===0)mainModel.setHpfHz(value);else mainModel.setLpfHz(value)}
                                    onHpTypeEdited:function(value){mainModel.setHpType(value)}
                                    onLpTypeEdited:function(value){mainModel.setLpType(value)}
                                }
                            }
                        }

                        // SURROUND.
                        Item {
                            RowLayout {
                                anchors.fill: parent
                                spacing: 12
                                RackFaderPanel {
                                    Layout.fillWidth:true; Layout.fillHeight:true; Layout.preferredWidth:344; Layout.minimumWidth:300
                                    title:"Surround Bus"
                                    channels:[
                                        {label:"L",value:12,from:-37.5,to:24,step:.5,unit:"dB",decimals:1},
                                        {label:"R",value:12,from:-37.5,to:24,step:.5,unit:"dB",decimals:1},
                                        {label:"MIC",value:87,from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"MUSIC",value:85,from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"REV",value:80,from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"ECHO",value:75,from:0,to:100,step:1,unit:"%",decimals:0}
                                    ]
                                }
                                RackDynamicsPanel {
                                    Layout.fillWidth:true; Layout.fillHeight:true; Layout.preferredWidth:443; Layout.minimumWidth:320
                                    title:"Output Compressor"; threshold:-20; ratio:100; attack:1; release:100
                                }
                                RackFilterPanel {
                                    Layout.preferredWidth:267; Layout.minimumWidth:238; Layout.maximumWidth:292; Layout.fillHeight:true
                                    title:"Band Limits / Delay"
                                    fields:[
                                        {label:"L DELAY",value:3,from:0,to:50,step:1,unit:"ms",decimals:0},
                                        {label:"R DELAY",value:4,from:0,to:50,step:1,unit:"ms",decimals:0},
                                        {label:"HPF",value:surroundModel.hpfHz,from:20,to:20000,step:1,unit:"Hz",decimals:0},
                                        {label:"LPF",value:surroundModel.lpfHz,from:20,to:20000,step:1,unit:"Hz",decimals:0}
                                    ]
                                    hpType:surroundModel.hpType; lpType:surroundModel.lpType
                                    onFieldEdited:function(index,value){if(index===2)surroundModel.setHpfHz(value);else if(index===3)surroundModel.setLpfHz(value)}
                                    onHpTypeEdited:function(value){surroundModel.setHpType(value)}
                                    onLpTypeEdited:function(value){surroundModel.setLpType(value)}
                                }
                            }
                        }

                        // CENTER.
                        Item {
                            RowLayout {
                                anchors.fill: parent
                                spacing: 12
                                RackFaderPanel {
                                    Layout.fillWidth:true; Layout.fillHeight:true; Layout.preferredWidth:344; Layout.minimumWidth:300
                                    title:"Center Bus"
                                    channels:[
                                        {label:"CTR",value:12,from:-37.5,to:24,step:.5,unit:"dB",decimals:1},
                                        {label:"MIC",value:88,from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"MUSIC",value:85,from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"REV",value:87,from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"ECHO",value:85,from:0,to:100,step:1,unit:"%",decimals:0}
                                    ]
                                }
                                RackDynamicsPanel {
                                    Layout.fillWidth:true; Layout.fillHeight:true; Layout.preferredWidth:443; Layout.minimumWidth:320
                                    title:"Output Compressor"; threshold:-20; ratio:100; attack:1; release:100
                                }
                                RackFilterPanel {
                                    Layout.preferredWidth:267; Layout.minimumWidth:238; Layout.maximumWidth:292; Layout.fillHeight:true
                                    title:"Band Limits / Delay"
                                    fields:[
                                        {label:"HPF",value:centerModel.hpfHz,from:20,to:20000,step:1,unit:"Hz",decimals:0},
                                        {label:"LPF",value:centerModel.lpfHz,from:20,to:20000,step:1,unit:"Hz",decimals:0}
                                    ]
                                    hpType:centerModel.hpType; lpType:centerModel.lpType
                                    onFieldEdited:function(index,value){if(index===0)centerModel.setHpfHz(value);else centerModel.setLpfHz(value)}
                                    onHpTypeEdited:function(value){centerModel.setHpType(value)}
                                    onLpTypeEdited:function(value){centerModel.setLpType(value)}
                                }
                            }
                        }

                        // SUBWOOFER.
                        Item {
                            RowLayout {
                                anchors.fill: parent
                                spacing: 12
                                RackFaderPanel {
                                    Layout.fillWidth:true; Layout.fillHeight:true; Layout.preferredWidth:344; Layout.minimumWidth:300
                                    title:"Subwoofer Bus"
                                    channels:[
                                        {label:"SUB",value:12,from:-37.5,to:24,step:.5,unit:"dB",decimals:1},
                                        {label:"MIC",value:0,from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"MUSIC",value:88,from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"REV",value:0,from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"ECHO",value:0,from:0,to:100,step:1,unit:"%",decimals:0}
                                    ]
                                }
                                RackDynamicsPanel {
                                    Layout.fillWidth:true; Layout.fillHeight:true; Layout.preferredWidth:443; Layout.minimumWidth:320
                                    title:"Output Compressor"; threshold:-20; ratio:100; attack:25; release:100
                                }
                                RackFilterPanel {
                                    Layout.preferredWidth:267; Layout.minimumWidth:238; Layout.maximumWidth:292; Layout.fillHeight:true
                                    title:"Band Limits / Delay"
                                    fields:[
                                        {label:"HPF",value:subModel.hpfHz,from:20,to:20000,step:1,unit:"Hz",decimals:0},
                                        {label:"LPF",value:subModel.lpfHz,from:20,to:20000,step:1,unit:"Hz",decimals:0}
                                    ]
                                    hpType:subModel.hpType; lpType:subModel.lpType
                                    onFieldEdited:function(index,value){if(index===0)subModel.setHpfHz(value);else subModel.setLpfHz(value)}
                                    onHpTypeEdited:function(value){subModel.setHpType(value)}
                                    onLpTypeEdited:function(value){subModel.setLpType(value)}
                                }
                            }
                        }
                    }

                    MasterStripPanel {
                        engine: root.engine
                        Layout.preferredWidth: root.masterWidth
                        Layout.minimumWidth: root.masterWidth
                        Layout.maximumWidth: root.masterWidth
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
