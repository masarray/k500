import QtQuick
import QtQuick.Layouts

Item {
    id: root

    required property var engine
    property int sectionIndex: 1
    property int micChannel: 0
    property bool micEqLinked: Boolean(groupValue("mic", "eqLink", false))
    readonly property int lowerRackHeight: 304
    readonly property int masterWidth: 188

    // K500_DEVICE_STATE_BINDINGS_V1
    function groupValue(group, key, fallback) {
        var state = engine && engine.deviceState ? engine.deviceState : null
        var object = state ? state[group] : null
        var value = object ? object[key] : undefined
        return value === undefined || value === null ? fallback : value
    }
    function nestedValue(group, section, key, fallback) {
        var state = engine && engine.deviceState ? engine.deviceState : null
        var parent = state ? state[group] : null
        var object = parent ? parent[section] : null
        var value = object ? object[key] : undefined
        return value === undefined || value === null ? fallback : value
    }
    function activeEqModel() {
        switch (sectionIndex) {
        case 1: return micChannel === 0 ? engine.micAEqBands : engine.micBEqBands
        case 2: return engine.reverbEqBands
        case 3: return engine.echoEqBands
        case 4: return engine.mainEqBands
        case 5: return engine.surroundEqBands
        case 6: return engine.centerEqBands
        case 7: return engine.subEqBands
        default: return engine.micAEqBands
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

                        Item {
                            RowLayout {
                                anchors.fill: parent
                                spacing: 12

                                RackFaderPanel {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    Layout.preferredWidth: 289
                                    Layout.minimumWidth: 248
                                    title: "Mic Inputs"
                                    channels: [
                                        {label:"MIC A",value:Number(root.groupValue("mic","micAVol",96)),from:0,to:100,step:1,unit:"",decimals:0},
                                        {label:"MIC B",value:Number(root.groupValue("mic","micBVol",96)),from:0,to:100,step:1,unit:"",decimals:0},
                                        {label:"FBX",badge:"A+B",value:Number(root.groupValue("mic","fbxLevel",7)),from:0,to:20,step:1,unit:"",decimals:0}
                                    ]
                                }
                                RackDynamicsPanel {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    Layout.preferredWidth: 607
                                    Layout.minimumWidth: 390
                                    title: "Vocal Dynamics"
                                    includeGate: true
                                    gate: Number(root.groupValue("mic","noiseGateDb",-70))
                                    threshold: Number(root.groupValue("mic","compThresholdDb",-12))
                                    ratio: Number(root.groupValue("mic","compRatio",3))
                                    attack: Number(root.groupValue("mic","attackMs",10))
                                    release: Number(root.groupValue("mic","releaseSec",0.2)) * 1000
                                }
                                RackFilterPanel {
                                    Layout.preferredWidth: 158
                                    Layout.minimumWidth: 158
                                    Layout.maximumWidth: 158
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

                        Item {
                            RowLayout {
                                anchors.fill: parent
                                spacing: 12
                                RackFaderPanel {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    Layout.minimumWidth: 380
                                    title: "Reverb"
                                    channels: [
                                        {label:"LEVEL",value:Number(root.nestedValue("effects","reverb","level",100)),from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"DECAY",value:Number(root.nestedValue("effects","reverb","decayMs",1575)),from:100,to:5000,step:5,unit:"ms",decimals:0},
                                        {label:"PRE",value:Number(root.nestedValue("effects","reverb","predelayMs",25)),from:0,to:300,step:1,unit:"ms",decimals:0}
                                    ]
                                }
                                RackFilterPanel {
                                    Layout.preferredWidth: 214
                                    Layout.minimumWidth: 214
                                    Layout.maximumWidth: 214
                                    Layout.fillHeight: true
                                    title: "Tone"
                                    fields: [
                                        {label:"HPF",value:root.engine.reverbEqBands.hpfHz,from:20,to:20000,step:1,unit:"Hz",decimals:0},
                                        {label:"LPF",value:root.engine.reverbEqBands.lpfHz,from:20,to:20000,step:1,unit:"Hz",decimals:0}
                                    ]
                                    hpType: root.engine.reverbEqBands.hpType
                                    lpType: root.engine.reverbEqBands.lpType
                                    onFieldEdited: function(index,value){ if(index===0)root.engine.reverbEqBands.setHpfHz(value);else root.engine.reverbEqBands.setLpfHz(value) }
                                    onHpTypeEdited: function(value){ root.engine.reverbEqBands.setHpType(value) }
                                    onLpTypeEdited: function(value){ root.engine.reverbEqBands.setLpType(value) }
                                }
                            }
                        }

                        Item {
                            RowLayout {
                                anchors.fill: parent
                                spacing: 12
                                RackFaderPanel {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    Layout.minimumWidth: 380
                                    title: "Echo"
                                    channels: [
                                        {label:"LEVEL",value:Number(root.nestedValue("effects","echo","level",100)),from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"REPEAT",value:Number(root.nestedValue("effects","echo","repeat",12)),from:0,to:100,step:1,unit:"",decimals:0},
                                        {label:"DELAY",value:Number(root.nestedValue("effects","echo","leftDelayMs",400)),from:0,to:1000,step:1,unit:"ms",decimals:0}
                                    ]
                                }
                                RackFilterPanel {
                                    Layout.preferredWidth: 214
                                    Layout.minimumWidth: 214
                                    Layout.maximumWidth: 214
                                    Layout.fillHeight: true
                                    title: "Tone"
                                    fields: [
                                        {label:"HPF",value:root.engine.echoEqBands.hpfHz,from:20,to:20000,step:1,unit:"Hz",decimals:0},
                                        {label:"LPF",value:root.engine.echoEqBands.lpfHz,from:20,to:20000,step:1,unit:"Hz",decimals:0}
                                    ]
                                    hpType: root.engine.echoEqBands.hpType
                                    lpType: root.engine.echoEqBands.lpType
                                    onFieldEdited: function(index,value){ if(index===0)root.engine.echoEqBands.setHpfHz(value);else root.engine.echoEqBands.setLpfHz(value) }
                                    onHpTypeEdited: function(value){ root.engine.echoEqBands.setHpType(value) }
                                    onLpTypeEdited: function(value){ root.engine.echoEqBands.setLpType(value) }
                                }
                            }
                        }

                        Item {
                            RowLayout {
                                anchors.fill: parent
                                spacing: 12
                                RackFaderPanel {
                                    Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 371; Layout.minimumWidth: 320
                                    title: "Main Bus"
                                    channels: [
                                        {label:"L",value:Number(root.nestedValue("outputs","main","lVolDb",12)),from:-37.5,to:24,step:.5,unit:"dB",decimals:1},
                                        {label:"R",value:Number(root.nestedValue("outputs","main","rVolDb",12)),from:-37.5,to:24,step:.5,unit:"dB",decimals:1},
                                        {label:"MIC",value:Number(root.nestedValue("outputs","main","micDirect",100)),from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"MUSIC",value:Number(root.nestedValue("outputs","main","musicLevel",100)),from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"REV",value:Number(root.nestedValue("outputs","main","reverbLevel",90)),from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"ECHO",value:Number(root.nestedValue("outputs","main","echoLevel",95)),from:0,to:100,step:1,unit:"%",decimals:0}
                                    ]
                                }
                                RackDynamicsPanel {
                                    Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 471; Layout.minimumWidth: 360
                                    title: "Output Compressor"
                                    threshold:Number(root.nestedValue("outputs","main","compThresholdDb",-3))
                                    ratio:Number(root.nestedValue("outputs","main","compRatio",18))
                                    attack:Number(root.nestedValue("outputs","main","attackMs",7))
                                    release:Number(root.nestedValue("outputs","main","releaseSec",0.1))*1000
                                }
                                RackFilterPanel {
                                    Layout.preferredWidth:212; Layout.minimumWidth:212; Layout.maximumWidth:212; Layout.fillHeight:true
                                    title: "Band Limits / Delay"
                                    fields:[
                                        {label:"HPF",value:root.engine.mainEqBands.hpfHz,from:20,to:20000,step:1,unit:"Hz",decimals:0},
                                        {label:"LPF",value:root.engine.mainEqBands.lpfHz,from:20,to:20000,step:1,unit:"Hz",decimals:0}
                                    ]
                                    hpType:root.engine.mainEqBands.hpType; lpType:root.engine.mainEqBands.lpType
                                    onFieldEdited:function(index,value){if(index===0)root.engine.mainEqBands.setHpfHz(value);else root.engine.mainEqBands.setLpfHz(value)}
                                    onHpTypeEdited:function(value){root.engine.mainEqBands.setHpType(value)}
                                    onLpTypeEdited:function(value){root.engine.mainEqBands.setLpType(value)}
                                }
                            }
                        }

                        Item {
                            RowLayout {
                                anchors.fill: parent
                                spacing: 12
                                RackFaderPanel {
                                    Layout.fillWidth:true; Layout.fillHeight:true; Layout.preferredWidth:371; Layout.minimumWidth:320
                                    title:"Surround Bus"
                                    channels:[
                                        {label:"L",value:Number(root.nestedValue("outputs","surround","lVolDb",12)),from:-37.5,to:24,step:.5,unit:"dB",decimals:1},
                                        {label:"R",value:Number(root.nestedValue("outputs","surround","rVolDb",12)),from:-37.5,to:24,step:.5,unit:"dB",decimals:1},
                                        {label:"MIC",value:Number(root.nestedValue("outputs","surround","micDirect",87)),from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"MUSIC",value:Number(root.nestedValue("outputs","surround","musicLevel",85)),from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"REV",value:Number(root.nestedValue("outputs","surround","reverbLevel",80)),from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"ECHO",value:Number(root.nestedValue("outputs","surround","echoLevel",75)),from:0,to:100,step:1,unit:"%",decimals:0}
                                    ]
                                }
                                RackDynamicsPanel {
                                    Layout.fillWidth:true; Layout.fillHeight:true; Layout.preferredWidth:471; Layout.minimumWidth:360
                                    title:"Output Compressor"
                                    threshold:Number(root.nestedValue("outputs","surround","compThresholdDb",-20))
                                    ratio:Number(root.nestedValue("outputs","surround","compRatio",100))
                                    attack:Number(root.nestedValue("outputs","surround","attackMs",1))
                                    release:Number(root.nestedValue("outputs","surround","releaseSec",0.1))*1000
                                }
                                RackFilterPanel {
                                    Layout.preferredWidth:212; Layout.minimumWidth:212; Layout.maximumWidth:212; Layout.fillHeight:true
                                    title:"Band Limits / Delay"
                                    fields:[
                                        {label:"L DELAY",value:Number(root.nestedValue("outputs","surround","lDelayMs",3)),from:0,to:50,step:1,unit:"ms",decimals:0},
                                        {label:"R DELAY",value:Number(root.nestedValue("outputs","surround","rDelayMs",4)),from:0,to:50,step:1,unit:"ms",decimals:0},
                                        {label:"HPF",value:root.engine.surroundEqBands.hpfHz,from:20,to:20000,step:1,unit:"Hz",decimals:0},
                                        {label:"LPF",value:root.engine.surroundEqBands.lpfHz,from:20,to:20000,step:1,unit:"Hz",decimals:0}
                                    ]
                                    hpType:root.engine.surroundEqBands.hpType; lpType:root.engine.surroundEqBands.lpType
                                    onFieldEdited:function(index,value){if(index===2)root.engine.surroundEqBands.setHpfHz(value);else if(index===3)root.engine.surroundEqBands.setLpfHz(value)}
                                    onHpTypeEdited:function(value){root.engine.surroundEqBands.setHpType(value)}
                                    onLpTypeEdited:function(value){root.engine.surroundEqBands.setLpType(value)}
                                }
                            }
                        }

                        Item {
                            RowLayout {
                                anchors.fill: parent
                                spacing: 12
                                RackFaderPanel {
                                    Layout.fillWidth:true; Layout.fillHeight:true; Layout.preferredWidth:371; Layout.minimumWidth:320
                                    title:"Center Bus"
                                    channels:[
                                        {label:"CTR",value:Number(root.nestedValue("outputs","center","outputVolDb",12)),from:-37.5,to:24,step:.5,unit:"dB",decimals:1},
                                        {label:"MIC",value:Number(root.nestedValue("outputs","center","micDirect",88)),from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"MUSIC",value:Number(root.nestedValue("outputs","center","musicLevel",85)),from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"REV",value:Number(root.nestedValue("outputs","center","reverbLevel",87)),from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"ECHO",value:Number(root.nestedValue("outputs","center","echoLevel",85)),from:0,to:100,step:1,unit:"%",decimals:0}
                                    ]
                                }
                                RackDynamicsPanel {
                                    Layout.fillWidth:true; Layout.fillHeight:true; Layout.preferredWidth:471; Layout.minimumWidth:360
                                    title:"Output Compressor"
                                    threshold:Number(root.nestedValue("outputs","center","compThresholdDb",-20))
                                    ratio:Number(root.nestedValue("outputs","center","compRatio",100))
                                    attack:Number(root.nestedValue("outputs","center","attackMs",1))
                                    release:Number(root.nestedValue("outputs","center","releaseSec",0.1))*1000
                                }
                                RackFilterPanel {
                                    Layout.preferredWidth:212; Layout.minimumWidth:212; Layout.maximumWidth:212; Layout.fillHeight:true
                                    title:"Band Limits / Delay"
                                    fields:[
                                        {label:"HPF",value:root.engine.centerEqBands.hpfHz,from:20,to:20000,step:1,unit:"Hz",decimals:0},
                                        {label:"LPF",value:root.engine.centerEqBands.lpfHz,from:20,to:20000,step:1,unit:"Hz",decimals:0}
                                    ]
                                    hpType:root.engine.centerEqBands.hpType; lpType:root.engine.centerEqBands.lpType
                                    onFieldEdited:function(index,value){if(index===0)root.engine.centerEqBands.setHpfHz(value);else root.engine.centerEqBands.setLpfHz(value)}
                                    onHpTypeEdited:function(value){root.engine.centerEqBands.setHpType(value)}
                                    onLpTypeEdited:function(value){root.engine.centerEqBands.setLpType(value)}
                                }
                            }
                        }

                        Item {
                            RowLayout {
                                anchors.fill: parent
                                spacing: 12
                                RackFaderPanel {
                                    Layout.fillWidth:true; Layout.fillHeight:true; Layout.preferredWidth:371; Layout.minimumWidth:320
                                    title:"Subwoofer Bus"
                                    channels:[
                                        {label:"SUB",value:Number(root.nestedValue("outputs","sub","outputVolDb",12)),from:-37.5,to:24,step:.5,unit:"dB",decimals:1},
                                        {label:"MIC",value:Number(root.nestedValue("outputs","sub","micDirect",0)),from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"MUSIC",value:Number(root.nestedValue("outputs","sub","musicLevel",88)),from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"REV",value:Number(root.nestedValue("outputs","sub","reverbLevel",0)),from:0,to:100,step:1,unit:"%",decimals:0},
                                        {label:"ECHO",value:Number(root.nestedValue("outputs","sub","echoLevel",0)),from:0,to:100,step:1,unit:"%",decimals:0}
                                    ]
                                }
                                RackDynamicsPanel {
                                    Layout.fillWidth:true; Layout.fillHeight:true; Layout.preferredWidth:471; Layout.minimumWidth:360
                                    title:"Output Compressor"
                                    threshold:Number(root.nestedValue("outputs","sub","compThresholdDb",-20))
                                    ratio:Number(root.nestedValue("outputs","sub","compRatio",100))
                                    attack:Number(root.nestedValue("outputs","sub","attackMs",25))
                                    release:Number(root.nestedValue("outputs","sub","releaseSec",0.1))*1000
                                }
                                RackFilterPanel {
                                    Layout.preferredWidth:212; Layout.minimumWidth:212; Layout.maximumWidth:212; Layout.fillHeight:true
                                    title:"Band Limits / Delay"
                                    fields:[
                                        {label:"HPF",value:root.engine.subEqBands.hpfHz,from:20,to:20000,step:1,unit:"Hz",decimals:0},
                                        {label:"LPF",value:root.engine.subEqBands.lpfHz,from:20,to:20000,step:1,unit:"Hz",decimals:0}
                                    ]
                                    hpType:root.engine.subEqBands.hpType; lpType:root.engine.subEqBands.lpType
                                    onFieldEdited:function(index,value){if(index===0)root.engine.subEqBands.setHpfHz(value);else root.engine.subEqBands.setLpfHz(value)}
                                    onHpTypeEdited:function(value){root.engine.subEqBands.setHpType(value)}
                                    onLpTypeEdited:function(value){root.engine.subEqBands.setLpType(value)}
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
