import QtQuick
import QtQuick.Layouts

Item {
    id: root
    required property var engine

    property int selectedPcPreset: 0
    property int selectedDeviceSlot: 3
    property var pcPresets: [
        "ARTIST GEN3 ARI", "PODCAST REBORN", "DANGDUT GEN3 ARI", "KARAOKE ARTIST",
        "AKUSTIK GEN3 ARI", "IMAM QORI GEN 3", "JAZZ GEN3 ARI", "ROCK GEN3 ARI",
        "MC CERAMAH GEN 3", "ADZAN MEKAH GEN3"
    ]

    GridLayout {
        anchors.fill: parent
        columns: 12
        rows: 2
        columnSpacing: 10
        rowSpacing: 10

        StudioPanel {
            Layout.columnSpan: 4
            Layout.rowSpan: 1
            Layout.fillWidth: true
            Layout.fillHeight: true
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 11; spacing: 7
                RowLayout {
                    Layout.fillWidth:true
                    ColumnLayout { spacing:0; Text{text:"PC MODE";color:Theme.textDim;font.family:Theme.fontFamily;font.pixelSize:8;font.weight:Font.DemiBold;font.letterSpacing:1.0} Text{text:"PRESET FILES";color:Theme.text;font.family:Theme.fontFamily;font.pixelSize:10;font.weight:Font.Bold} }
                    Item{Layout.fillWidth:true}
                    SoftButton{text:"REFRESH";compact:true;checked:true}
                }
                Rectangle{Layout.fillWidth:true;Layout.preferredHeight:1;color:Theme.borderSoft}
                Rectangle {
                    Layout.fillWidth:true; Layout.fillHeight:true; radius:6; color:"#080C10"; border.width:1; border.color:Theme.borderSoft; clip:true
                    ListView {
                        anchors.fill:parent; anchors.margins:5; spacing:3; model:root.pcPresets
                        delegate: Rectangle {
                            required property int index; required property string modelData
                            width:ListView.view.width; height:27; radius:5
                            color:index===root.selectedPcPreset?"#10272A":"transparent"
                            border.width:index===root.selectedPcPreset?1:0; border.color:Theme.accentSoft
                            RowLayout { anchors.fill:parent; anchors.leftMargin:7; anchors.rightMargin:7; spacing:6
                                Text{text:String(index+1).padStart(2,"0");color:Theme.textDim;font.family:Theme.fontFamily;font.pixelSize:8}
                                Text{Layout.fillWidth:true;text:modelData;color:index===root.selectedPcPreset?Theme.accent:Theme.textSoft;font.family:Theme.fontFamily;font.pixelSize:9;font.weight:Font.DemiBold;elide:Text.ElideRight}
                                Text{text:index===root.selectedPcPreset?"LOADED":"FACTORY";color:index===root.selectedPcPreset?Theme.accent:Theme.textFaint;font.family:Theme.fontFamily;font.pixelSize:7;font.weight:Font.Bold}
                            }
                            MouseArea{anchors.fill:parent;cursorShape:Qt.PointingHandCursor;onClicked:root.selectedPcPreset=index}
                        }
                    }
                }
                RowLayout { Layout.fillWidth:true; spacing:6
                    SoftButton{Layout.fillWidth:true;text:"SAVE TO PC";compact:true}
                    SoftButton{Layout.fillWidth:true;text:"UPLOAD TO DEVICE";compact:true;checked:true}
                    SoftButton{Layout.fillWidth:true;text:"MASS UPLOAD";compact:true}
                }
            }
        }

        StudioPanel {
            Layout.columnSpan: 4
            Layout.rowSpan: 1
            Layout.fillWidth: true
            Layout.fillHeight: true
            ColumnLayout {
                anchors.fill:parent; anchors.margins:11; spacing:7
                RowLayout { Layout.fillWidth:true
                    ColumnLayout{spacing:0;Text{text:"EQUIPMENT / DEVICE MODE";color:Theme.textDim;font.family:Theme.fontFamily;font.pixelSize:8;font.weight:Font.DemiBold;font.letterSpacing:1.0}Text{text:"DEVICE PRESET SLOTS";color:Theme.text;font.family:Theme.fontFamily;font.pixelSize:10;font.weight:Font.Bold}}
                    Item{Layout.fillWidth:true}
                    Rectangle{width:48;height:22;radius:5;color:"#0B1715";border.width:1;border.color:"#31584E";Text{anchors.centerIn:parent;text:"ACTIVE";color:Theme.green;font.family:Theme.fontFamily;font.pixelSize:7;font.weight:Font.Bold}}
                }
                Rectangle{Layout.fillWidth:true;Layout.preferredHeight:1;color:Theme.borderSoft}
                Rectangle {
                    Layout.fillWidth:true; Layout.fillHeight:true; radius:6; color:"#080C10"; border.width:1; border.color:Theme.borderSoft; clip:true
                    ListView {
                        anchors.fill:parent; anchors.margins:5; spacing:3; model:root.pcPresets
                        delegate: Rectangle {
                            required property int index; required property string modelData
                            width:ListView.view.width; height:27; radius:5
                            readonly property bool active:index===3
                            color:index===root.selectedDeviceSlot?"#10272A":"transparent"
                            border.width:index===root.selectedDeviceSlot?1:0;border.color:Theme.accentSoft
                            RowLayout {anchors.fill:parent;anchors.leftMargin:7;anchors.rightMargin:7;spacing:6
                                Text{text:index+1;color:Theme.textDim;font.family:Theme.fontFamily;font.pixelSize:8}
                                Text{Layout.fillWidth:true;text:modelData;color:active?Theme.accent:Theme.textSoft;font.family:Theme.fontFamily;font.pixelSize:9;font.weight:Font.DemiBold;elide:Text.ElideRight}
                                Text{text:active?"ACTIVE":index===root.selectedDeviceSlot?"SELECT":"";color:active?Theme.green:Theme.textDim;font.family:Theme.fontFamily;font.pixelSize:7;font.weight:Font.Bold}
                            }
                            MouseArea{anchors.fill:parent;cursorShape:Qt.PointingHandCursor;onClicked:root.selectedDeviceSlot=index}
                        }
                    }
                }
                RowLayout{Layout.fillWidth:true;spacing:6;SoftButton{Layout.fillWidth:true;text:"RECALL";compact:true}SoftButton{Layout.fillWidth:true;text:"SAVE";compact:true;checked:true}SoftButton{Layout.fillWidth:true;text:"RESET ALL";compact:true}}
            }
        }

        ColumnLayout {
            Layout.columnSpan: 2
            Layout.rowSpan: 1
            Layout.fillWidth:true
            Layout.fillHeight:true
            spacing:10
            StudioPanel {
                Layout.fillWidth:true;Layout.fillHeight:true
                ColumnLayout{anchors.fill:parent;anchors.margins:11;spacing:7
                    Text{text:"BLUETOOTH";color:Theme.textDim;font.family:Theme.fontFamily;font.pixelSize:8;font.weight:Font.DemiBold;font.letterSpacing:1.0}
                    Text{text:"BT NAME";color:Theme.text;font.family:Theme.fontFamily;font.pixelSize:10;font.weight:Font.Bold}
                    ValueField{Layout.fillWidth:true;title:"BT NAME";value:500;from:0;to:999;step:1;decimals:0;unit:""}
                    ValueField{Layout.fillWidth:true;title:"BLE NAME";value:500;from:0;to:999;step:1;decimals:0;unit:""}
                    Item{Layout.fillHeight:true}
                    RowLayout{Layout.fillWidth:true;SoftButton{Layout.fillWidth:true;text:"MODIFY";compact:true}SoftButton{Layout.fillWidth:true;text:"RESET";compact:true}}
                }
            }
            StudioPanel {
                Layout.fillWidth:true;Layout.fillHeight:true
                ColumnLayout{anchors.fill:parent;anchors.margins:11;spacing:7
                    Text{text:"ACCESS";color:Theme.textDim;font.family:Theme.fontFamily;font.pixelSize:8;font.weight:Font.DemiBold;font.letterSpacing:1.0}
                    Text{text:"LOCK / ADMIN";color:Theme.text;font.family:Theme.fontFamily;font.pixelSize:10;font.weight:Font.Bold}
                    RowLayout{Layout.fillWidth:true;SoftButton{Layout.fillWidth:true;text:"UNLOCK";compact:true;checked:true}SoftButton{Layout.fillWidth:true;text:"LOCK";compact:true}SoftButton{Layout.fillWidth:true;text:"ADMIN";compact:true;checked:true}}
                    Item{Layout.fillHeight:true}
                    SoftButton{Layout.fillWidth:true;text:"MODIFY PASSWORD";compact:true}
                }
            }
        }

        MasterStripPanel {
            engine: root.engine
            Layout.columnSpan: 2
            Layout.rowSpan: 1
            Layout.fillWidth:true
            Layout.fillHeight:true
        }

        RackFaderPanel {
            Layout.columnSpan: 5
            Layout.rowSpan: 1
            Layout.fillWidth:true
            Layout.fillHeight:true
            eyebrow:"SAFE BOOT"
            title:"Startup Limits"
            channels:[
                {label:"MUSIC INIT",value:35,from:0,to:84,step:1,unit:"",decimals:0},
                {label:"MUSIC MAX",value:84,from:0,to:84,step:1,unit:"",decimals:0},
                {label:"MIC INIT",value:35,from:0,to:84,step:1,unit:"",decimals:0},
                {label:"MIC MAX",value:84,from:0,to:84,step:1,unit:"",decimals:0},
                {label:"EFFECT INIT",value:35,from:0,to:84,step:1,unit:"",decimals:0}
            ]
        }

        RackFaderPanel {
            Layout.columnSpan: 5
            Layout.rowSpan: 1
            Layout.fillWidth:true
            Layout.fillHeight:true
            eyebrow:"USB / UDISK · DANCE MODE"
            title:"Recording / Mic Trigger"
            channels:[
                {label:"UDISK REC",value:4,from:1,to:6,step:1,unit:"",decimals:0},
                {label:"USB REC",value:4,from:1,to:6,step:1,unit:"",decimals:0},
                {label:"THRESHOLD",value:-50,from:-80,to:0,step:1,unit:"dB",decimals:0},
                {label:"HOLD TIME",value:6,from:0,to:30,step:1,unit:"s",decimals:0}
            ]
        }
    }
}
