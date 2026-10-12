#!/usr/bin/env node
/* OPERATION_FEEDBACK_SINGLE_AUTHORITY_V2 — run actual SystemWorkspaceImpl QML
 * functions in a deterministic VM, without hardware, QML or new CI workflows.
 */
"use strict";
const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");
const impl = fs.readFileSync(path.join(__dirname,
    "../../qml/components/SystemWorkspaceImpl.qml"), "utf8");
const popup = fs.readFileSync(path.join(__dirname,
    "../../qml/components/OperationFeedback.qml"), "utf8");
const rack = fs.readFileSync(path.join(__dirname,
    "../../qml/components/RackFaderPanel.qml"), "utf8");
function extract(name) {
    const begin = impl.indexOf("    function " + name + "(");
    assert.ok(begin >= 0, "Missing actual QML function " + name);
    let start = impl.indexOf("{", begin), depth = 0;
    for (let i = start; i < impl.length; i++) {
        if (impl[i] === "{") depth++;
        if (impl[i] === "}" && --depth === 0) return impl.slice(begin, i + 1);
    }
    throw Error("Unclosed QML function " + name);
}
const timer = { stops:0, restarts:0,
    stop() { this.stops++; }, restart() { this.restarts++; } };
const ctx = {
    singleOpKind:"", singleOpSlot:0, singleOpState:"", singleOpMessage:"",
    deviceConnected:true, presetManager:{busy:false}, singleOpDismiss:timer
};
ctx.root = ctx;
vm.createContext(ctx);
vm.runInContext(["beginSingleOperation", "finishSingleOperation"]
    .map(extract).join("\n"), ctx);
assert.equal(ctx.beginSingleOperation("Save", 3), true);
assert.equal(ctx.singleOpState, "busy");
assert.equal(ctx.singleOpSlot, 3);
assert.equal(ctx.beginSingleOperation("Save", 4), false, "no second submission");
assert.equal(ctx.singleOpSlot, 3, "second click cannot switch active slot");
ctx.finishSingleOperation("Upload", true, "wrong kind", 3);
ctx.finishSingleOperation("Save", true, "wrong slot", 4);
assert.equal(ctx.singleOpState, "busy", "stale completion cannot fake success");
ctx.finishSingleOperation("Save", true, "hardware verified", 3);
assert.equal(ctx.singleOpState, "success");
assert.equal(ctx.singleOpMessage, "hardware verified");
assert.equal(timer.restarts, 1);
ctx.finishSingleOperation("Save", false, "late failure", 0);
assert.equal(ctx.singleOpState, "success", "late failure cannot replace completion");
ctx.presetManager.busy = true;
assert.equal(ctx.beginSingleOperation("Recall", 1), false, "native busy gate");
ctx.presetManager.busy = false;
assert.equal(ctx.beginSingleOperation("Recall", 5), true);
ctx.finishSingleOperation("Recall", false, "Timeout menunggu RSP 0xC0", 0);
assert.equal(ctx.singleOpState, "error");
assert.match(ctx.singleOpMessage, /0xC0/, "failure reason must be retained");
ctx.deviceConnected = false;
assert.equal(ctx.beginSingleOperation("Upload", 5), false, "disconnected gate");
assert.match(impl, /kind === "Save" \|\| kind === "Upload" \|\| kind === "Recall"/);
assert.match(impl, /root\.finishSingleOperation\(kind, true,[\s\S]*?slot\)/);
assert.match(impl, /root\.beginSingleOperation\("Recall", slot\)/);
assert.match(impl, /root\.beginSingleOperation\("Save", slot\)/);
assert.match(impl, /root\.beginSingleOperation\("Upload", slot\)/);
assert.match(impl, /root\.presetManager\.progress/);
assert.match(popup, /parent: Overlay\.overlay/);
assert.match(popup, /x: parent \? Math\.round\(\(parent\.width - width\) \/ 2\)/);
assert.match(popup, /y: parent \? Math\.round\(\(parent\.height - height\) \/ 2\)/);
assert.match(popup, /visible: phase !== ""/);
assert.match(popup, /Popup\.NoAutoClose/);
assert.doesNotMatch(popup, /saveCurrentToSlot|uploadSlotImage|recallMode|Timer\s*\{/);
assert.match(rack, /SYNC_MASTER_OPTICAL_FACE_V2/);
assert.match(rack, /trackHeight: Math\.max\(11, Math\.round\(syncMasterLabel\.contentHeight\)\)/);
assert.match(rack, /width: 38\s*height: 30/);
console.log("PASS operation UI: slot/kind ownership, duplicate gate, native result, Recall timeout, full-window overlay, optical toggle");
