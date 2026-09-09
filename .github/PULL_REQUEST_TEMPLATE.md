## Summary

<!-- What user-visible or engineering behavior changes? Keep the scope focused. -->

## Why

<!-- What problem/evidence motivates this change? -->

## Change type

- [ ] UI / UX
- [ ] Device protocol / hardware behavior
- [ ] `.k500` codec / preset file workflow
- [ ] Official preset data
- [ ] Preset library / Sync / Mass Upload
- [ ] Packaging / release
- [ ] Documentation / website
- [ ] Tests / CI only

## Evidence and safety boundary

<!-- For hardware-facing changes, link donor/capture evidence. State which fields/bytes/commands are proven and which remain unsupported. -->

- Device-truth authority affected: **no / yes — explain**
- Permanent device storage affected: **no / yes — explain**
- Unknown/reserved bytes affected: **no / yes — explain**
- New hardware packet/offset introduced: **no / yes — evidence required**
- Bluetooth support claim changed: **no / yes — physical evidence required**

## Preset integrity

<!-- Complete when .k500 or official preset data changes. -->

- Donor/source:
- Before SHA-256:
- After SHA-256:
- File size/checksum valid: **N/A / yes**
- Changed-byte audit reviewed: **N/A / yes**
- Mode 01 native golden hash preserved: **N/A / yes / intentionally changed with evidence**

## Verification

- [ ] Existing regression guards remain enabled.
- [ ] Relevant unit/self-tests pass.
- [ ] Runtime section-navigation stress test remains intact when QML/workspace lifecycle changes.
- [ ] Invalid/uncertain destructive operations still fail closed.
- [ ] Documentation/support claims were updated if public behavior changed.
- [ ] No generated build artifacts, local caches, or private test data are committed.

### Physical K500 acceptance

- [ ] Not required for this change.
- [ ] Required and pending.
- [ ] Passed — evidence linked below.

Evidence / issue / capture:

## Screenshots

<!-- Add before/after screenshots for material UI or landing-page changes when useful. -->

## Release impact

- [ ] No release required.
- [ ] Official preset Sync update only.
- [ ] Patch release candidate.
- [ ] Minor/major release candidate.
