#!/usr/bin/env node
// PEQ_CAPTION_SPATIAL_LAYOUT_V2 — actual production geometry in Node VM;
// tests frequency-anchor proximity, caption collisions, plot edges, inspector.
"use strict";
const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");
const root = path.join(__dirname, "../..");
const layoutSource = fs.readFileSync(path.join(root, "qml/components/CrossoverCaptionLayout.js"), "utf8");
const qml = fs.readFileSync(path.join(root, "qml/components/SectionEqGraph.qml"), "utf8");
const crossoverInspector = fs.readFileSync(path.join(root, "qml/components/CrossoverInspector.qml"), "utf8");
const bandInspector = fs.readFileSync(path.join(root, "qml/components/BandInspector.qml"), "utf8");
const studioCpp = fs.readFileSync(path.join(root, "src/StudioEngine.cpp"), "utf8");
const studioHdr = fs.readFileSync(path.join(root, "src/StudioEngine.h"), "utf8");
const nativeSelfTest = fs.readFileSync(path.join(root, "src/main.cpp"), "utf8");
const lowerPanel = fs.readFileSync(path.join(root, "qml/components/SectionWorkspace.qml"), "utf8");
const ctx = {};
vm.createContext(ctx);
vm.runInContext(layoutSource.replace(/^\.pragma library\s*/m, ""), ctx);
assert.equal(typeof ctx.place, "function");
assert.match(qml, /import "CrossoverCaptionLayout.js" as CaptionPlacement/);
assert.match(qml, /captionPositions: CaptionPlacement\.place\(/);
assert.match(qml, /hpW:hpfCaption\.width/);
assert.match(qml, /lpW:lpfCaption\.width/);
assert.match(qml, /obstacle:root\.selectedTarget/);
assert.match(qml, /id:hpfCaption/);
assert.match(qml, /id:lpfCaption/);
assert.match(qml, /elide:Text\.ElideRight/);
// PEQ_BYPASS_QUIET_CAPTIONS_V3 — no tiny outlined shouty text. Device
// state is represented by native grey/amber on captions, guides and nodes.
const captions = qml.match(/Text \{\n\s+id:(?:hpf|lpf)Caption[\s\S]*?\n\s+\}/g) || [];
assert.equal(captions.length, 2, "exactly two crossover captions");
for (const c of captions) {
    assert.doesNotMatch(c, /BYPASS|Text\.Outline|font\.pixelSize:10/);
    assert.match(c, /font\.family:Theme\.fontFamily/);
    assert.match(c, /font\.pixelSize:11/);
    assert.doesNotMatch(c, /renderType:Text\.QtRendering/, "use application-level native AA");
}
assert.match(captions[0], /text:"HP "\+root\.fmtF\(root\.bands\.hpfHz\)\+" Hz"/);
assert.match(captions[1], /text:"LP "\+root\.fmtF\(root\.bands\.lpfHz\)\+" Hz"/);
assert.match(qml, /readonly property color hpGuideColor: root\.hpfBypassed \? Theme\.textDim : Theme\.amber/);
assert.match(qml, /readonly property color lpGuideColor: root\.lpfBypassed \? Theme\.textDim : Theme\.amber/);
for (const color of ["hpGuideColor", "lpGuideColor"]) {
    assert.ok(qml.split("color:root." + color).length >= 4,
        color + " used across line, node and caption");
}


const rect = (p, w, h) => ({x:p.x,y:p.y,w,h});
const collide = (a,b,g=0) => a.x < b.x+b.w+g && a.x+a.w+g > b.x
    && a.y < b.y+b.h+g && a.y+a.h+g > b.y;
function verify(name, options) {
    const o = Object.assign({width:1220,height:372,left:56,right:1198,
        top:24,bottom:334,hpX:392,lpX:505,hpW:105,lpW:110,
        hpH:14,lpH:14,obstacle:null}, options);
    const result = ctx.place(o);
    const a = rect(result.hp,o.hpW,o.hpH);
    const b = rect(result.lp,o.lpW,o.lpH);
    for (const box of [a,b]) {
        assert.ok(Number.isFinite(box.x) && Number.isFinite(box.y), name + " finite");
        assert.ok(box.x >= 0 && box.x+box.w <= o.width, name + " horizontal bounds");
        assert.ok(box.y >= 0 && box.y+box.h <= o.height, name + " vertical bounds");
    }
    assert.ok(!collide(a,b,6), name + " captions must not collide");
    for (const box of [a,b]) {
        assert.ok(box.y >= 12, name + " must clear top graph border");
        assert.ok(box.x >= o.left + 12, name + " must clear left plot border");
        assert.ok(box.x + box.w <= o.right - 12, name + " must clear right plot border");
    }
    if (o.obstacle) {
        assert.ok(!collide(a,o.obstacle,3), name + " HP must avoid inspector");
        assert.ok(!collide(b,o.obstacle,3), name + " LP must avoid inspector");
    }
    return result;
}
verify("subwoofer HP40 LP82", {});
verify("subwoofer HP40 LP95", {hpX:392,lpX:524,hpW:112,lpW:115});
verify("coincident crossovers", {hpX:460,lpX:460,hpW:115,lpW:115});
verify("same-low-frequency", {hpX:58,lpX:58});
verify("both rightmost", {hpX:1184,lpX:1184});
verify("normal wide split", {hpX:58,lpX:1192});
verify("hovering active band inspector", {hpX:392,lpX:505,
    obstacle:{x:355,y:42,w:320,h:112}});
verify("coincident with active crossover inspector", {hpX:505,lpX:505,
    obstacle:{x:345,y:42,w:320,h:112}});
verify("125-percent compact graph", {width:850,height:290,left:42,right:831,
    top:19,bottom:258,hpX:120,lpX:173,hpW:112,lpW:117,
    obstacle:{x:60,y:175,w:300,h:78}});
verify("150-percent constrained graph", {width:660,height:247,left:37,right:644,
    top:17,bottom:220,hpX:110,lpX:145,hpW:115,lpW:118,
    obstacle:{x:47,y:150,w:300,h:78}});
// INSPECTOR_OPTICAL_PADDING_V3 / BAND_CHIP_OPTICAL_LAYOUT_V3
// Scope must remain presentation-only, avoid negative spacers, and preserve
// actionable signals and data ownership for 5/7/10-band instances.
for (const inspector of [crossoverInspector, bandInspector]) {
    assert.match(inspector, /INSPECTOR_OPTICAL_PADDING_V3/);
    assert.match(inspector, /implicitWidth: 340/);
    assert.match(inspector, /implicitHeight: 120/);
    assert.match(inspector, /anchors.leftMargin: 16/);
    assert.match(inspector, /anchors.rightMargin: 16/);
    assert.match(inspector, /anchors.topMargin: 14/);
    assert.match(inspector, /anchors.bottomMargin: 14/);
    assert.match(inspector, /spacing: 3/);
    assert.doesNotMatch(inspector, /spacing: -1|anchors.margins: -4/);
    assert.match(inspector, /onClicked: root.resetRequested\(\)/);
}
assert.match(qml, /BAND_CHIP_OPTICAL_LAYOUT_V3/);
assert.match(qml, /Layout.preferredHeight:50;radius:10/);
assert.match(qml, /anchors.leftMargin:18/);
assert.match(qml, /anchors.rightMargin:12/);
assert.match(qml, /anchors.topMargin:7/);
assert.match(qml, /anchors.bottomMargin:7/);
assert.match(qml, /Item \{Layout.fillWidth:true\}/);
assert.doesNotMatch(qml, /parent.width-34|parent.width-54/);
assert.equal((qml.match(/graph.width-32\);height:120/g)||[]).length,2,
    "both inspectors must have the updated hit-safe height");
// FX_FIXED_FREQUENCY_ONLY_V1 — do not regress to a synthetic BYPASS state.
// Manufacturer's captured Reverb CMD0B / Echo CMD0D store only cutoff Hz.
assert.match(qml, /readonly property bool fxFrequencyOnly: root.sectionLabel === "Reverb" \|\| root.sectionLabel === "Echo"/);
assert.match(qml, /readonly property bool hpfBypassed: !root.fxFrequencyOnly/);
assert.match(qml, /readonly property bool lpfBypassed: !root.fxFrequencyOnly/);
assert.match(qml, /if\(root.fxFrequencyOnly\)return \/\/ Native FX CMD/);
assert.match(qml, /frequencyOnly:root.fxFrequencyOnly/);
assert.match(qml, /onTypeEdited:function\(v\)\{if\(!root.fxFrequencyOnly\)/);
assert.match(crossoverInspector, /property bool frequencyOnly: false/);
assert.match(crossoverInspector, /visible: !root.frequencyOnly/g);
assert.match(crossoverInspector, /readonly property bool bypassed: !root.frequencyOnly/);
assert.equal((lowerPanel.match(/showTypes: false/g) || []).length, 2);
for (const fx of ["reverb", "echo"]) {
    const name = fx === "reverb" ? "m_reverbEqBands" : "m_echoEqBands";
    assert.ok(studioCpp.includes(name + '.configure(5,'), fx + " configured");
    const offset = studioCpp.indexOf(name + '.configure(5,');
    assert.match(studioCpp.slice(offset, offset+240),
        /QStringLiteral\("HP Butter 12"\), QStringLiteral\("LP Butter 12"\)/);
}
assert.match(studioCpp, /FX_FIXED_FREQUENCY_ONLY_V1/);
assert.match(studioHdr, /const bool fixedFx = section == QStringLiteral\("reverb"\)/);
assert.match(nativeSelfTest, /studioEngine.reverbEqBands\(\)->setHpType\(QStringLiteral\("Bypass"\)\)/);
console.log("PASS PEQ fixed FX cutoff invariant: Reverb/Echo always active, frequency only, no type write");
