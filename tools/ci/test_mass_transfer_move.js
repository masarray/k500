#!/usr/bin/env node
/* MASS_TRANSFER_TRUE_MOVE_REGRESSION_V1
 * Executes the actual QML transfer, drop and reorder functions in a small
 * JS VM. No Qt/USB/build/new CI runner. The right ListModel owns the ONLY
 * upload plan and left is an identity-filtered catalogue projection.
 */
"use strict";
const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");
const qml = fs.readFileSync(path.join(__dirname,
    "../../qml/components/MassUploadTransferWindow.qml"), "utf8");
function extract(name) {
    const start = qml.indexOf("    function " + name + "(");
    assert.ok(start >= 0, "missing real QML function " + name);
    const open = qml.indexOf("{", start);
    let depth = 0;
    for (let i = open; i < qml.length; ++i) {
        if (qml[i] === "{") ++depth;
        else if (qml[i] === "}" && --depth === 0)
            return qml.slice(start, i + 1);
    }
    throw new Error("unterminated " + name);
}
const catalog = Array.from({length: 20}, (_, i) => ({
    path: "official-" + (i + 1), fileName: "preset" + (i + 1) + ".k500",
    displayName: "PRESET " + (i + 1), valid: true, originLabel: "SONKUPIK"
}));
const model = {
    entries: [],
    get count() { return this.entries.length; },
    get(i) { return this.entries[i]; },
    insert(i, e) { this.entries.splice(i, 0, {...e}); },
    remove(i) { this.entries.splice(i, 1); },
    move(from, to, count) { this.entries.splice(to, 0, ...this.entries.splice(from, count)); },
    clear() { this.entries = []; }
};
const ctx = {
    targetModel: model, catalogPresets: catalog, stagedRevision: 0,
    editingLocked: false, transferInFlight: false, targetIndex: -1,
    sourceIndex: -1, sourceAnchor: -1, selectedSourceIndexes: [],
    maxSlots: 10, dragKind: "", dragIndex: -1, dragActive: false,
    dragConsumedClick: false, dragStartX: 0, dragStartY: 0,
    dragX: 0, dragY: 0, dragZone: "", dragInsertIndex: -1,
    dragSourcePaths: [], dragLabel: "", contentItem: {},
    slotCanvas: {
        width: 410, height: 410, slotRowHeight: 36,
        mapFromItem(_item, x, y) { return {x: x - 540, y: y - 60}; }
    },
    sourceList: {
        width: 400, height: 410,
        mapFromItem(_item, x, y) { return {x: x - 20, y: y - 60}; }
    },
    Qt: {ControlModifier: 0x04000000, ShiftModifier: 0x02000000, NoModifier: 0}
};
Object.defineProperty(ctx, "sourcePresets", {
    get() { return catalog.filter(e => !ctx.targetContains(e.path)); }
});
ctx.root = ctx;
vm.createContext(ctx);
vm.runInContext([
    "clearSourceSelection", "selectionPosition", "isSourceSelected",
    "selectSource", "targetContains", "addEntry", "addSelected",
    "addAll", "returnEntry", "removeSelected", "clearTarget",
    "moveTarget", "targetInsertionAt", "overSourceAt",
    "beginTransferGesture", "updateTransferGesture",
    "finishTransferGesture", "cancelTransferGesture"
].map(extract).join("\n"), ctx, {filename:"MassUploadTransferWindow.qml/transfer"});
function slots() { return model.entries.map(e => e.path); }
function available() { return ctx.sourcePresets.map(e => e.path); }
function drag(kind, from, sx, sy, dx, dy) {
    ctx.beginTransferGesture(kind, from, sx, sy);
    ctx.updateTransferGesture(dx, dy);
    ctx.finishTransferGesture(dx, dy);
}
function expectSlots(list, message) {
    assert.deepEqual(slots(), list.map(x => "official-" + x), message);
}
const originalCatalog = catalog.map(e => e.path);
assert.equal(ctx.sourcePresets.length, 20);
// Source card #4 -> right: stage one, hide only from left, no file deletion.
drag("source", 3, 100, 100, 580, 78);
expectSlots([4], "first drag must stage the correct identity");
assert.equal(ctx.sourcePresets.length, 19);
assert.equal(available().includes("official-4"), false);
assert.deepEqual(catalog.map(e => e.path), originalCatalog, "catalogue remains immutable");
// Another source card #8 -> position 2 (adjusted left index after #4 hidden).
drag("source", 6, 100, 132, 620, 140);
expectSlots([4,8], "second drag appends to K500 plan");
// Ctrl/Shift staged selection picks are copied chronologically on drop.
ctx.selectedSourceIndexes = [4, 1]; // currently #6 then #2 (filtered catalogue)
drag("source", 4, 100, 162, 600, 270);
expectSlots([4,8,6,2], "group drop respects click chronology");
// Target lower item -> top; all right rows correspond to slot 1..N.
drag("target", 3, 630, 195, 630, 66);
expectSlots([2,4,8,6], "drag reorder last to first");
assert.deepEqual(model.entries.map((e,i)=>[i+1,e.path]),
  [[1,"official-2"],[2,"official-4"],[3,"official-8"],[4,"official-6"]]);
// Target first item -> end of target list.
drag("target", 0, 630, 72, 630, 400);
expectSlots([4,8,6,2], "drag reorder first to last");
// Drag staged preset back to left: deletes only plan entry, source reappears.
drag("target", 1, 630, 112, 100, 170);
expectSlots([4,6,2], "drag right to left returns preset");
assert.equal(available().includes("official-8"), true);
// Cancel or outside drop must leave plan unchanged.
ctx.beginTransferGesture("target", 0, 630, 70);
ctx.updateTransferGesture(1000, 490);
ctx.finishTransferGesture(1000, 490);
expectSlots([4,6,2], "outside drop cannot mutate plan");
ctx.beginTransferGesture("target", 0, 630, 70);
ctx.cancelTransferGesture();
expectSlots([4,6,2], "cancelled gesture is inert");
// Repeated path is never added twice even through a direct function call.
assert.equal(ctx.addEntry(catalog[3]), false, "duplicate rejected");
// Transfer maximum 10, preserve rank 1..10.
ctx.addAll();
assert.equal(model.count, 10);
assert.equal(new Set(slots()).size, 10, "slot IDs unique");
assert.equal(ctx.addEntry(catalog[19]), false, "11th stage rejected");
// A blocked upload must not mutate the plan.
ctx.editingLocked = true;
const snapshot = slots();
assert.equal(ctx.returnEntry(0), false);
assert.equal(ctx.moveTarget(0, 9), false);
assert.equal(ctx.addEntry(catalog[19]), false);
assert.deepEqual(slots(), snapshot, "in-flight model frozen");
ctx.editingLocked = false;
ctx.clearTarget();
assert.equal(model.count, 0);
assert.equal(ctx.sourcePresets.length, 20);
console.log("PASS: real QML DnD left/right, chronological group, reorder 01..10, cancel, duplicate, lock, catalogue identity");
