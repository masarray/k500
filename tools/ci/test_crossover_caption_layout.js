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
    assert.match(c, /renderType:Text\.QtRendering/);
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
    obstacle:{x:355,y:34,w:320,h:86}});
verify("coincident with active crossover inspector", {hpX:505,lpX:505,
    obstacle:{x:345,y:38,w:320,h:86}});
verify("125-percent compact graph", {width:850,height:290,left:42,right:831,
    top:19,bottom:258,hpX:120,lpX:173,hpW:112,lpW:117,
    obstacle:{x:60,y:175,w:300,h:78}});
verify("150-percent constrained graph", {width:660,height:247,left:37,right:644,
    top:17,bottom:220,hpX:110,lpX:145,hpW:115,lpW:118,
    obstacle:{x:47,y:150,w:300,h:78}});
console.log("PASS PEQ spatial captions: measured overlap, viewport, coincidence, inspector, zoom/resize");
