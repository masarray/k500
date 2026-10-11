# Mass Upload Transfer UX — field acceptance

**Scope:** GUI-only. The v1.1.3 K500 hardware/firmware communication engine,
Save, Upload, Mass Upload native 0x41/42/43, and 99→100% ACK/readback modal
remain unchanged. The tested SYNC MASTER→INIT→Save→power-cycle behaviour is a
release-blocking invariant and is not part of this patch.

## Expected interactions

The Mass Upload dialog now behaves like a movable-preset transfer list:
the left collection is the original PC catalogue **minus** entries staged on
the right; the right `targetModel` is the *only* editable planned slot
assignment. No file is moved or deleted from PC storage. The right row's
position maps to the device destination slot (row 1→slot 1, ... row 10→slot 10).
Its existing C++ bridge, `buildTransferUploadEntries()`, preserves this order;
the engine separately sends native Store in hardware-required descending order.

- **Click, Ctrl/Shift, double-click and Add/Remove** remain usable.
- Drag single or Ctrl/Shift-selected presets from left→right at a precise
  insertion position, then see staged items disappear from the left.
- Drag right→left to return an item, or right↕right to reorder by moving the
  *original* ListModel row, not swapping its display label only.
- Drag cancel / drop outside both panels must leave the plan intact.
- All 10 slots remain capped; invalid presets, duplicates and unavailable
  catalogue resources cannot be staged. Ctrl/Shift source selection is cleared
  after a staging list mutation to prevent stale array indices.
- Source collection refresh does not mutate already staged identities; the
  bridge validates every path and every preset checksum again when Upload
  is clicked, before sending any USB command.
- During active operation or upload progress modal, plan manipulation is
  disabled; Mass Upload keeps its original 99% acknowledged/100% verified
  UI and does **not** speculatively rerun a Store on error.

Save/Upload single operations now display a compact status toast with a
spinner and an *indeterminate* loading strip; only actual
`K500PresetManager::operationCompleted` signals yield success. Errors and
disconnect show actionable text and never appear as success. This is not a
second Store progress API, and no simulated percentage is presented.

SYNC MASTER and other System toggles have a smaller 32/34 px face while
retaining their larger existing click targets. `SystemToggleRow` retains the
full-row click area.

## QA matrix on Windows

1. Open Mass Upload offline. Pick several sources, stage by drag/drop and
   Add, return some, reorder first↔last, verify visible slot numbers.
2. Select source items Ctrl/Shift in arbitrary order, drag group into middle;
   verify chronological order from selection rather than source sort order.
3. Repeated Add and drag cannot produce duplicated source paths or 11 slots.
4. Drag outside and cancel: unchanged plan. Folder Sync/Refresh: no stale
   index/plan mutation. Keep original .k500 files present and unmodified.
5. Press Upload while disconnected: existing Connect guidance; no write.
6. Connect K500. Save and Upload a safe test slot: spinner/progress then real
   success/error, not fictitious percent; no double-submit; power-cycle
   still restores SYNC MASTER volume as verified by owner.
7. Mass Upload on a safely backed-up device: preserve former verified overlay,
   10→1 native Store order and final full readback, then re-check all slots.
8. Resize dialog to min 800×520, test scroll wheel, pointer drag, double-click
   and readable text at Windows 100/125/150% scaling.

## Deterministic CI

The existing `Fast Contracts` job executes the **real QML transfer methods**
extracted by `tools/ci/test_mass_transfer_move.js` in Node VM: filtering,
cross-list drops, middle insertion, first/last reorder, return, cancel,
duplicate rejection, 10-slot bound and busy lock. It also keeps the previous
chronological multi-selection VM test. No additional workflow/build job.
