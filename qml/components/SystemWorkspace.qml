import QtQuick
import QtQuick.Layouts

Item {
    id: root
    required property var engine

    property int selectedDeviceSlot: 3
    property var deviceSlots: [
        "ARTIST GEN3 ARI", "PODCAST REBORN", "DANGDUT GEN3 ARI", "KARAOKE ARTIST",
        "AKUSTIK GEN3 ARI", "IMAM QORI GEN 3", "JAZZ GEN3 ARI", "ROCK GEN3 ARI",
        "MC CERAMAH GEN 3", "ADZAN MEKAH GEN3"
    ]
    readonly property int lowerRackHeight: 304

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            StudioPanel {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 466
                accentTop: false

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 35
                        Text { anchors.left:parent.left;anchors.leftMargin:12;anchors.verticalCenter:parent.verticalCenter;text:"PRESET FILES";color:Theme.text;font.family:Theme.monoFamily;font.pixelSize:10;font.weight:Font.Bold;font.letterSpacing:1.05 }
                        Rectangle { anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:1;color:Theme.borderSoft }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.margins: 12
                        spacing: 9

                        RowLayout {
                            Layout.fillWidth: true
                            Text { Layout.fillWidth:true;text:"D:\\Documents\\SONKUPIK STUDIO Presets";color:Theme.accent;font.family:Theme.monoFamily;font.pixelSize:10;font.weight:Font.Bold;elide:Text.ElideMiddle }
                            SoftButton { Layout.preferredWidth:62;text:"Refresh";compact:true;checked:true }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            radius: 10
                            color: "#090D11"
                            border.width: 1
                            border.color: "#050708"

                            Rectangle {
                                anchors.left:parent.left;anchors.right:parent.right;anchors.top:parent.top
                                anchors.margins:9;height:36;radius:7
                                gradient: Gradient { GradientStop{position:0;color:"#151A1F"} GradientStop{position:1;color:"#0B0F13"} }
                                border.width:1;border.color:"#242C33"
                                RowLayout {
                                    anchors.fill:parent;anchors.leftMargin:10;anchors.rightMargin:10;spacing:8
                                    Text{text:"1";color:Theme.amber;font.family:Theme.monoFamily;font.pixelSize:9;font.weight:Font.Bold}
                                    Text{Layout.fillWidth:true;text:"KARAOKE ARTIST LUXURY";color:Theme.text;font.family:Theme.monoFamily;font.pixelSize:10;font.weight:Font.Bold;elide:Text.ElideRight}
                                    Text{text:"FACTORY";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.weight:Font.Bold}
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            SoftButton { Layout.fillWidth:true;text:"Save to PC";compact:true }
                            SoftButton { Layout.fillWidth:true;text:"Upload to device";compact:true }
                            SoftButton { Layout.fillWidth:true;text:"Mass upload";compact:true }
                        }
                    }
                }
            }

            StudioPanel {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 432
                accentTop: false

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Item {
                        Layout.fillWidth:true
                        Layout.preferredHeight:35
                        Text{anchors.left:parent.left;anchors.leftMargin:12;anchors.verticalCenter:parent.verticalCenter;text:"DEVICE PRESET SLOTS";color:Theme.text;font.family:Theme.monoFamily;font.pixelSize:10;font.weight:Font.Bold;font.letterSpacing:1.05}
                        Rectangle{anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:1;color:Theme.borderSoft}
                    }

                    ColumnLayout {
                        Layout.fillWidth:true
                        Layout.fillHeight:true
                        Layout.margins:12
                        spacing:8

                        RowLayout {
                            Layout.fillWidth:true
                            Text{Layout.fillWidth:true;text:"4 · KARAOKE ARTIST";color:Theme.accent;font.family:Theme.monoFamily;font.pixelSize:12;font.weight:Font.Bold}
                            Rectangle{width:52;height:24;radius:12;color:"#0B1715";border.width:1;border.color:"#31584E";Text{anchors.centerIn:parent;text:"ACTIVE";color:Theme.accent;font.family:Theme.monoFamily;font.pixelSize:8;font.weight:Font.Bold}}
                        }

                        Rectangle {
                            Layout.fillWidth:true
                            Layout.fillHeight:true
                            radius:10
                            color:"#090D11"
                            border.width:1
                            border.color:"#050708"
                            clip:true

                            Column {
                                anchors.fill:parent
                                anchors.margins:7
                                spacing:3
                                Repeater {
                                    model:root.deviceSlots
                                    delegate:Rectangle {
                                        required property int index
                                        required property string modelData
                                        width:parent.width
                                        height:27
                                        radius:6
                                        readonly property bool active:index===3
                                        readonly property bool selected:index===root.selectedDeviceSlot
                                        color:selected?"#151B20":"#0D1115"
                                        border.width:1
                                        border.color:active?"#8B6A08":selected?Theme.accentSoft:"#252D34"
                                        RowLayout {
                                            anchors.fill:parent;anchors.leftMargin:9;anchors.rightMargin:9;spacing:7
                                            Text{text:index+1;color:Theme.amber;font.family:Theme.monoFamily;font.pixelSize:9;font.weight:Font.Bold}
                                            Text{Layout.fillWidth:true;text:modelData;color:Theme.text;font.family:Theme.monoFamily;font.pixelSize:9;font.weight:Font.Bold;elide:Text.ElideRight}
                                            Text{text:active?"ACTIVE":"";color:Theme.accent;font.family:Theme.monoFamily;font.pixelSize:8;font.weight:Font.Bold}
                                        }
                                        MouseArea{anchors.fill:parent;cursorShape:Qt.PointingHandCursor;onClicked:root.selectedDeviceSlot=index}
                                    }
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth:true
                            spacing:5
                            Rectangle{width:13;height:13;radius:2;color:"#4A5055";border.width:1;border.color:"#60676C"}
                            Text{text:"Use init volume";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:9}
                            Item{Layout.fillWidth:true}
                        }

                        RowLayout {
                            Layout.fillWidth:true
                            spacing:8
                            SoftButton{Layout.fillWidth:true;text:"Recall";compact:true}
                            SoftButton{Layout.fillWidth:true;text:"Save";compact:true}
                            SoftButton{Layout.fillWidth:true;text:"Reset all";compact:true}
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 355
                spacing: 12

                StudioPanel {
                    Layout.fillWidth:true
                    Layout.preferredHeight:212
                    accentTop:false
                    ColumnLayout {
                        anchors.fill:parent
                        spacing:0
                        Item{Layout.fillWidth:true;Layout.preferredHeight:35;Text{anchors.left:parent.left;anchors.leftMargin:12;anchors.verticalCenter:parent.verticalCenter;text:"BT NAME";color:Theme.text;font.family:Theme.monoFamily;font.pixelSize:10;font.weight:Font.Bold;font.letterSpacing:1.05}Rectangle{anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:1;color:Theme.borderSoft}}
                        ColumnLayout {
                            Layout.fillWidth:true;Layout.fillHeight:true;Layout.margins:12;spacing:8
                            Text{text:"BT NAME";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.letterSpacing:1.1}
                            Rectangle{Layout.fillWidth:true;Layout.preferredHeight:29;radius:6;color:"#080C10";border.width:1;border.color:"#050708";Text{anchors.left:parent.left;anchors.leftMargin:9;anchors.verticalCenter:parent.verticalCenter;text:"KTV_BT_00AB12";color:Theme.amber;font.family:Theme.monoFamily;font.pixelSize:10;font.weight:Font.Bold}}
                            Text{text:"BLE NAME";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.letterSpacing:1.1}
                            Rectangle{Layout.fillWidth:true;Layout.preferredHeight:29;radius:6;color:"#080C10";border.width:1;border.color:"#050708";Text{anchors.left:parent.left;anchors.leftMargin:9;anchors.verticalCenter:parent.verticalCenter;text:"KTV_BLE_00AB12";color:Theme.amber;font.family:Theme.monoFamily;font.pixelSize:10;font.weight:Font.Bold}}
                            Item{Layout.fillHeight:true}
                            RowLayout{Layout.fillWidth:true;spacing:8;SoftButton{Layout.fillWidth:true;text:"Modify";compact:true}SoftButton{Layout.fillWidth:true;text:"Reset";compact:true}}
                        }
                    }
                }

                StudioPanel {
                    Layout.fillWidth:true
                    Layout.fillHeight:true
                    accentTop:false
                    ColumnLayout {
                        anchors.fill:parent
                        spacing:0
                        Item{Layout.fillWidth:true;Layout.preferredHeight:35;Text{anchors.left:parent.left;anchors.leftMargin:12;anchors.verticalCenter:parent.verticalCenter;text:"LOCK / ADMIN";color:Theme.text;font.family:Theme.monoFamily;font.pixelSize:10;font.weight:Font.Bold;font.letterSpacing:1.05}Rectangle{anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:1;color:Theme.borderSoft}}
                        ColumnLayout {
                            Layout.fillWidth:true;Layout.fillHeight:true;Layout.margins:12;spacing:8
                            RowLayout {
                                Layout.fillWidth:true;spacing:8
                                ColumnLayout{Layout.fillWidth:true;spacing:4;Text{text:"LOCK KEY";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.letterSpacing:1.0}Rectangle{Layout.fillWidth:true;Layout.preferredHeight:29;radius:6;color:"#080C10";border.width:1;border.color:"#050708";Text{anchors.left:parent.left;anchors.leftMargin:9;anchors.verticalCenter:parent.verticalCenter;text:"••••";color:Theme.amber;font.family:Theme.monoFamily;font.pixelSize:10}}}
                                ColumnLayout{Layout.fillWidth:true;spacing:4;Text{text:"ADMIN";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.letterSpacing:1.0}Rectangle{Layout.fillWidth:true;Layout.preferredHeight:29;radius:6;color:"#080C10";border.width:1;border.color:"#050708";Text{anchors.left:parent.left;anchors.leftMargin:9;anchors.verticalCenter:parent.verticalCenter;text:"••••";color:Theme.amber;font.family:Theme.monoFamily;font.pixelSize:10}}}
                            }
                            RowLayout {
                                Layout.fillWidth:true;spacing:9
                                Repeater {
                                    model:[{label:"Unlock",on:true},{label:"Lock",on:false},{label:"Admin",on:true}]
                                    delegate:RowLayout {
                                        required property var modelData
                                        spacing:4
                                        Rectangle{width:13;height:13;radius:2;color:modelData.on?Theme.accent:"#E6E9EB";border.width:1;border.color:modelData.on?Theme.accentSoft:"#B8C0C6";Text{anchors.centerIn:parent;visible:modelData.on;text:"✓";color:"#071012";font.pixelSize:10;font.weight:Font.Bold}}
                                        Text{text:modelData.label;color:Theme.text;font.family:Theme.fontFamily;font.pixelSize:9}
                                    }
                                }
                                Item{Layout.fillWidth:true}
                            }
                            Item{Layout.fillHeight:true}
                            Text{text:"NEW PASSWORD";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.letterSpacing:1.1}
                            RowLayout {
                                Layout.fillWidth:true;spacing:8
                                Rectangle{Layout.fillWidth:true;Layout.preferredHeight:29;radius:6;color:"#080C10";border.width:1;border.color:"#050708"}
                                SoftButton{Layout.preferredWidth:58;text:"Modify";compact:true}
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: root.lowerRackHeight
            Layout.minimumHeight: root.lowerRackHeight
            Layout.maximumHeight: root.lowerRackHeight
            spacing: 12

            RackFaderPanel {
                Layout.fillWidth:true
                Layout.fillHeight:true
                Layout.preferredWidth:466
                title:"Startup Limits"
                channels:[
                    {label:"MUSIC INIT",value:25,from:0,to:84,step:1,unit:"",decimals:0},
                    {label:"MUSIC MAX",value:84,from:0,to:84,step:1,unit:"",decimals:0},
                    {label:"MIC INIT",value:25,from:0,to:84,step:1,unit:"",decimals:0},
                    {label:"MIC MAX",value:84,from:0,to:84,step:1,unit:"",decimals:0},
                    {label:"EFFECT INIT",value:25,from:0,to:84,step:1,unit:"",decimals:0}
                ]
            }

            StudioPanel {
                Layout.fillWidth:true
                Layout.fillHeight:true
                Layout.preferredWidth:432
                accentTop:false
                ColumnLayout {
                    anchors.fill:parent
                    spacing:0
                    Item{Layout.fillWidth:true;Layout.preferredHeight:35;Text{anchors.left:parent.left;anchors.leftMargin:12;anchors.verticalCenter:parent.verticalCenter;text:"RECORDING / MIC TRIGGER";color:Theme.text;font.family:Theme.monoFamily;font.pixelSize:10;font.weight:Font.Bold;font.letterSpacing:1.05}Rectangle{anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:1;color:Theme.borderSoft}}
                    RowLayout {
                        Layout.fillWidth:true;Layout.fillHeight:true;Layout.margins:10;spacing:10
                        Rectangle {
                            Layout.fillWidth:true;Layout.fillHeight:true;radius:10;color:"#151B21";border.width:1;border.color:Theme.borderSoft
                            ColumnLayout {
                                anchors.fill:parent;anchors.margins:8;spacing:5
                                Text{text:"RECORDING";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.letterSpacing:1.1}
                                RowLayout {
                                    Layout.fillWidth:true;Layout.fillHeight:true;spacing:7
                                    Repeater {
                                        model:[{label:"UDISK REC",value:4,from:1,to:6},{label:"USB REC",value:4,from:1,to:6}]
                                        delegate:ColumnLayout {
                                            id: recChannel
                                            required property var modelData
                                            property real localValue: Number(modelData.value)
                                            Layout.fillWidth:true;Layout.fillHeight:true;spacing:3
                                            Text{Layout.alignment:Qt.AlignHCenter;text:modelData.label;color:recFader.highlighted?recFader.accentColor:Theme.textDim;style:recFader.highlighted?Text.Outline:Text.Normal;styleColor:recFader.highlighted?Qt.rgba(recFader.accentColor.r,recFader.accentColor.g,recFader.accentColor.b,.34):"transparent";font.family:Theme.monoFamily;font.pixelSize:8;font.weight:recFader.highlighted?Font.DemiBold:Font.Normal;Behavior on color{ColorAnimation{duration:75}}Behavior on styleColor{ColorAnimation{duration:75}}}
                                            StudioFader{id:recFader;Layout.fillHeight:true;Layout.preferredWidth:48;Layout.alignment:Qt.AlignHCenter;value:recChannel.localValue;from:modelData.from;to:modelData.to;step:1;defaultValue:modelData.value;onValueEdited:function(v){recChannel.localValue=v}}
                                            Rectangle{Layout.alignment:Qt.AlignHCenter;Layout.preferredWidth:48;Layout.preferredHeight:23;radius:8;color:"#080C10";border.width:1;border.color:recFader.highlighted?recFader.accentColor:"#050708";Behavior on border.color{ColorAnimation{duration:75}}Text{anchors.centerIn:parent;text:recChannel.localValue;color:Theme.amber;font.family:Theme.monoFamily;font.pixelSize:9;font.weight:Font.Bold}}
                                        }
                                    }
                                }
                            }
                        }
                        Rectangle {
                            Layout.fillWidth:true;Layout.fillHeight:true;radius:10;color:"#151B21";border.width:1;border.color:Theme.borderSoft
                            ColumnLayout {
                                anchors.fill:parent;anchors.margins:8;spacing:5
                                Text{text:"MIC TRIGGER";color:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:8;font.letterSpacing:1.1}
                                RowLayout {
                                    Layout.fillWidth:true;Layout.fillHeight:true;spacing:7
                                    Repeater {
                                        model:[{label:"THRESHOLD",value:-50,from:-80,to:0,unit:"dB"},{label:"HOLD TIME",value:6,from:0,to:30,unit:"s"}]
                                        delegate:ColumnLayout {
                                            id: triggerChannel
                                            required property var modelData
                                            property real localValue: Number(modelData.value)
                                            Layout.fillWidth:true;Layout.fillHeight:true;spacing:3
                                            Text{Layout.alignment:Qt.AlignHCenter;text:modelData.label;color:triggerFader.highlighted?triggerFader.accentColor:Theme.textDim;style:triggerFader.highlighted?Text.Outline:Text.Normal;styleColor:triggerFader.highlighted?Qt.rgba(triggerFader.accentColor.r,triggerFader.accentColor.g,triggerFader.accentColor.b,.34):"transparent";font.family:Theme.monoFamily;font.pixelSize:8;font.weight:triggerFader.highlighted?Font.DemiBold:Font.Normal;Behavior on color{ColorAnimation{duration:75}}Behavior on styleColor{ColorAnimation{duration:75}}}
                                            StudioFader{id:triggerFader;Layout.fillHeight:true;Layout.preferredWidth:48;Layout.alignment:Qt.AlignHCenter;value:triggerChannel.localValue;from:modelData.from;to:modelData.to;step:1;defaultValue:modelData.value;onValueEdited:function(v){triggerChannel.localValue=v}}
                                            Rectangle{Layout.alignment:Qt.AlignHCenter;Layout.preferredWidth:52;Layout.preferredHeight:23;radius:8;color:"#080C10";border.width:1;border.color:triggerFader.highlighted?triggerFader.accentColor:"#050708";Behavior on border.color{ColorAnimation{duration:75}}Row{anchors.centerIn:parent;spacing:3;Text{text:triggerChannel.localValue;color:Theme.amber;font.family:Theme.monoFamily;font.pixelSize:9;font.weight:Font.Bold}Text{text:modelData.unit;color:triggerFader.highlighted?Theme.textSoft:Theme.textDim;font.family:Theme.monoFamily;font.pixelSize:7;anchors.baseline:parent.children[0].baseline;Behavior on color{ColorAnimation{duration:75}}}}}
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            MasterStripPanel {
                engine: root.engine
                Layout.fillWidth:true
                Layout.fillHeight:true
                Layout.preferredWidth:355
            }
        }
    }
}
