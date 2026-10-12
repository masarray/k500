#!/usr/bin/env node
/* OFFLINE_SYSTEM_DRAFT_IMMEDIATE_V2 — deterministic VM over REAL QML functions.
 * No mock of .k500 serialization, hardware success or Qt rendering is claimed.
 */
"use strict";
const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");
const impl = fs.readFileSync(path.join(__dirname, "../../qml/components/SystemWorkspaceImpl.qml"), "utf8");
const rack = fs.readFileSync(path.join(__dirname, "../../qml/components/RackFaderPanel.qml"), "utf8");
function extract(name) {
    const startAt = impl.indexOf("    function " + name + "(");
    assert.ok(startAt >= 0, "actual QML function missing: " + name);
    const begin = impl.indexOf("{", startAt);
    let depth = 0;
    for (let i = begin; i < impl.length; i++) {
        if (impl[i] === "{") ++depth;
        if (impl[i] === "}" && --depth === 0) return impl.slice(startAt, i + 1);
    }
    throw Error("unclosed QML function: " + name);
}
const writes = [];
const engine = {
    musicInitVol:25, musicMaxVol:84, micInitVol:25, micMaxVol:84, effectInitLevel:25,
    editDevicePath(p,v) { writes.push([p,v]); }
};
const bridge = {
    loaded:false, lastError:"", editPersistenceEnabled:false,
    previewLoadedPreset() { this.editPersistenceEnabled = true; ctx.offlineEditMode = true;
        ctx.offlineSystemEdits = {}; return true; }
};
const ctx = {
    deviceConnected:false, offlineEditMode:false,
    offlineSystemEdits:{}, stagedPresetReady:false,
    offlineStarterAvailable:true,
    fileBridge:bridge, engine,
    pcSaveNotice:"", pcSaveNoticeTimer:{restart(){}},
    startOfflineWorkingPreset() { bridge.loaded=true; ctx.offlineEditMode=true;
        ctx.offlineSystemEdits={}; return true; }
};
ctx.root = ctx;
vm.createContext(ctx);
const functions = [
    "pcNotice", "updateOfflineSystemEdit", "startupPath", "setEngineStartup",
    "editStartup", "unmappedMicDraft", "prepareOfflineSave"
];
vm.runInContext(functions.map(extract).join("\n"), ctx);
// No Preview / no file / no K500: visual edits work and cannot transmit.
ctx.editStartup("system.musicInitVol", 35);
ctx.updateOfflineSystemEdit("system.uDiskRecordVol", 6, false);
assert.equal(ctx.offlineSystemEdits["system.musicInitVol"], 35);
assert.equal(ctx.offlineSystemEdits["system.uDiskRecordVol"], 6);
assert.equal(engine.musicInitVol, 25, "draft must not mutate device-owned engine");
assert.equal(writes.length, 0, "no LIVE writes while freely moving offline");
// Save PC As explicitly materializes verified base and replays only supported edits.
assert.equal(ctx.prepareOfflineSave(), true);
assert.equal(bridge.loaded, true);
assert.equal(engine.musicInitVol,35);
assert.equal(writes.length,1);
assert.equal(writes[0][0],"system.uDiskRecordVol");
assert.equal(writes[0][1],6);
assert.equal(ctx.offlineSystemEdits["system.uDiskRecordVol"],6);
// No guessed Mic Trigger scalar mapping: block export rather than silent loss.
ctx.updateOfflineSystemEdit("system.danceMicHoldSec", 15, false);
assert.equal(ctx.prepareOfflineSave(),false);
assert.equal(writes.length,1);
assert.match(ctx.pcSaveNotice,/Mic Trigger/);
assert.match(impl,/interactionEnabled:!root\.deviceConnected/);
assert.match(impl,/readonly property bool channelEditable:!root\.deviceConnected/);
assert.match(impl,/if \(root\.fileBridge\.previewLoadedPreset\(\)\)[\s\S]*?offlineSystemEdits = \(\{\}\)/);
assert.match(impl,/if \(root\.deviceConnected\) \{\s*root\.offlineSystemEdits = \(\{\}\)/);
assert.doesNotMatch(impl,/NO PRESET STAGED|text: loadedPreset \? "STAGED"/);
assert.match(rack,/root\.systemEditHandler\(path, value\)/);
console.log("PASS offline System: free draft, hardware isolation, verified Save source, mapping replay, Mic Trigger fail-closed, Preview/device reset, one preset surface");
