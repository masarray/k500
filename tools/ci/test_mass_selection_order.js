#!/usr/bin/env node
/* MASS_UPLOAD_SELECTION_ORDER_REGRESSION_V1
 * Executes the REAL selection and staging functions extracted from their QML
 * owner, instead of testing a rewritten approximation. No Qt/app/window/USB,
 * network, dependency install, or separate GitHub workflow is required.
 */
"use strict";
const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");

const qml = fs.readFileSync(
    path.join(__dirname, "../../qml/components/MassUploadTransferWindow.qml"), "utf8"
);
function qmlFunction(name) {
    const marker = "    function " + name + "(";
    const start = qml.indexOf(marker);
    assert.ok(start >= 0, "missing real QML function " + name);
    const open = qml.indexOf("{", start);
    assert.ok(open >= 0, "missing body for " + name);
    let depth = 0;
    for (let i = open; i < qml.length; ++i) {
        if (qml[i] === "{") depth += 1;
        if (qml[i] === "}") {
            depth -= 1;
            if (depth === 0)
                return qml.slice(start, i + 1);
        }
    }
    throw new Error("unterminated QML function " + name);
}

const ctx = {
    Qt: { ControlModifier: 0x04000000, ShiftModifier: 0x02000000, NoModifier: 0 },
    sourceAnchor: -1,
    sourceIndex: -1,
    selectedSourceIndexes: [],
    targetIndex: -1,
    maxSlots: 10,
    sourcePresets: Array.from({length: 20}, (_, i) => ({
        valid: true,
        path: "official-" + String(i + 1),
        fileName: "official-" + String(i + 1) + ".k500",
        displayName: "PRESET " + String(i + 1),
        source: "official",
        originLabel: "SONKUPIK"
    })),
    targetModel: {
        entries: [],
        get count() { return this.entries.length; },
        get(i) { return this.entries[i]; },
        append(entry) { this.entries.push({...entry}); },
        clear() { this.entries.length = 0; }
    }
};
ctx.root = ctx;
vm.createContext(ctx);
vm.runInContext(
    ["clearSourceSelection", "selectionPosition", "isSourceSelected",
     "selectSource", "targetContains", "addEntry", "addSelected", "addAll"]
        .map(qmlFunction).join("\n"),
    ctx,
    {timeout: 1000, filename: "MassUploadTransferWindow.qml/selection"}
);
const CTRL = ctx.Qt.ControlModifier;
const SHIFT = ctx.Qt.ShiftModifier;
function assertPicks(indices, title) {
    assert.deepEqual(Array.from(ctx.selectedSourceIndexes), indices, title);
    indices.forEach((index, position) => assert.equal(
        ctx.selectionPosition(index), position + 1,
        "visible pick rank for source #" + index
    ));
}
function assertSlots(numbers, title) {
    assert.deepEqual(ctx.targetModel.entries.map(e => e.path),
        numbers.map(n => "official-" + n), title);
}

// User picks BLUES CLUB (#08) first, then POP ROCK (#04), then UAUDIO (#18).
ctx.selectSource(7, 0);
ctx.selectSource(3, CTRL);
ctx.selectSource(17, CTRL);
assertPicks([7, 3, 17], "Ctrl picks must retain click chronology");
ctx.addSelected();
assertSlots([8, 4, 18], "Add must map click order to physical slots");
ctx.addSelected();
assertSlots([8, 4, 18], "repeated Add must not duplicate entries");

// Cancelling then choosing the same source again moves it to the last pick.
ctx.selectSource(3, CTRL);
assertPicks([7, 17], "Ctrl deselect must preserve other pick order");
ctx.selectSource(3, CTRL);
assertPicks([7, 17, 3], "Ctrl re-pick must append, never re-sort");
ctx.targetModel.clear();
ctx.addSelected();
assertSlots([8, 18, 4], "Ctrl re-pick moves the staged source last");

// Shift range must follow direction from first picked anchor to clicked end.
ctx.clearSourceSelection();
ctx.selectSource(7, 0);
ctx.selectSource(3, SHIFT);
assertPicks([7, 6, 5, 4, 3], "reverse Shift range preserves pick direction");
ctx.targetModel.clear();
ctx.addSelected();
assertSlots([8, 7, 6, 5, 4], "reverse Shift Add mirrors range direction");
ctx.clearSourceSelection();
ctx.selectSource(3, 0);
ctx.selectSource(7, SHIFT);
assertPicks([3, 4, 5, 6, 7], "forward Shift range preserves pick direction");

// Ctrl+Shift extends, rather than replacing existing earlier selections.
ctx.clearSourceSelection();
ctx.selectSource(7, 0);
ctx.selectSource(3, CTRL);
ctx.selectSource(6, CTRL | SHIFT);
assertPicks([7, 3, 4, 5, 6], "Ctrl+Shift appends unselected range in direction");

// Invalid source inside Shift range is skipped, not staged as corrupt preset.
ctx.sourcePresets[5].valid = false;
ctx.clearSourceSelection();
ctx.selectSource(3, 0);
ctx.selectSource(7, SHIFT);
assertPicks([3, 4, 6, 7], "invalid preset skipped during range selection");
ctx.targetModel.clear();
ctx.addSelected();
assertSlots([4, 5, 7, 8], "invalid preset omitted from stage");
ctx.sourcePresets[5].valid = true;

// Even a full-bank selection may stage no more than 10 physical K500 slots.
ctx.clearSourceSelection();
ctx.selectSource(0, 0);
ctx.selectSource(19, SHIFT);
assert.equal(ctx.selectedSourceIndexes.length, 20, "selection order includes all 20 PC choices");
ctx.targetModel.clear();
ctx.addSelected();
assertSlots([1,2,3,4,5,6,7,8,9,10], "physical 10-slot bound preserved");
assert.equal(ctx.targetModel.count, 10, "no overflow to 11th device slot");

console.log("PASS: real MassUpload selection order, badge ranks, Shift direction, Ctrl re-pick, duplicates, invalid items and 10-slot cap");
