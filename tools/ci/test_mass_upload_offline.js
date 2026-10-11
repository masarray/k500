#!/usr/bin/env node
/* OFFLINE_MASS_UPLOAD_CONNECT_GUIDANCE_TEST_V1
 * Execute the real QML uploadTransfer function with mocked collaborators.
 * Never touches hardware, files, Qt or USB; no separate CI workflow.
 */
"use strict";
const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");
const file = fs.readFileSync(path.join(__dirname, "../../qml/components/MassUploadTransferWindow.qml"), "utf8");
const start = file.indexOf("    function uploadTransfer()");
assert.ok(start >= 0);
const open = file.indexOf("{", start);
let close = -1, depth = 0;
for (let i = open; i < file.length; i++) {
    if (file[i] === "{") depth++;
    if (file[i] === "}" && --depth === 0) { close = i + 1; break; }
}
assert.ok(close > open, "real uploadTransfer function body");
const source = file.slice(start, close);
let stagedCalls = 0, uploadCalls = 0, toastCalls = 0;
const target = [{path: "one.k500"}, {path: "two.k500"}];
const context = {
    root: null,
    targetModel: { count: target.length, get(i) { return target[i]; } },
    fileBridge: { buildTransferUploadEntries(paths) {
        stagedCalls++;
        assert.deepEqual(Array.from(paths), ["one.k500", "two.k500"]);
        return [{slot: 2, image: new Uint8Array(656)}];
    }},
    presetManager: {
        usbStoreAvailable: false, busy: false,
        massUploadSlotImages(entries) { uploadCalls++; assert.equal(entries.length, 1); }
    },
    connectNoticeTimer: { restart() { toastCalls++; }, stop() {} },
    transferSucceeded: false,
    transferError: "",
    transferInFlight: false,
    connectNotice: ""
};
context.root = context;
vm.createContext(context);
vm.runInContext(source, context, { filename: "MassUploadTransferWindow.qml/uploadTransfer", timeout: 1200 });
context.uploadTransfer();
assert.equal(stagedCalls, 0, "offline must not even stage images");
assert.equal(uploadCalls, 0, "offline must never send Store writes");
assert.match(context.connectNotice, /Connect/);
assert.equal(toastCalls, 1);
assert.equal(context.transferInFlight, false, "offline cannot open fake progress overlay");
context.presetManager.busy = true;
context.presetManager.usbStoreAvailable = true;
context.uploadTransfer();
assert.equal(stagedCalls, 0, "busy must never start a second mass upload");
context.presetManager.busy = false;
context.uploadTransfer();
assert.equal(stagedCalls, 1);
assert.equal(uploadCalls, 1);
assert.equal(context.transferInFlight, true);
assert.equal(context.connectNotice, "");
assert.ok(file.includes("OFFLINE_MASS_UPLOAD_CONNECT_GUIDANCE_V1"));
assert.ok(file.includes("id: connectToastText"));
console.log("PASS: Mass Upload offline Connect guidance, no writes, busy lock, online upload");
