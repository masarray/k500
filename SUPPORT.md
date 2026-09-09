# Support

For SonKuPik K500 support, first confirm you are using the latest stable release and the expected support scope.

## Before opening an issue

1. Reproduce the problem on the latest stable build when practical.
2. Record the SonKuPik K500 version and Windows version.
3. Record the transport: USB HID or Bluetooth SPP.
4. If the problem involves an Official/Local preset, record the preset filename/source and SHA-256 when possible.
5. Export a **Support Report** from the application when the UI is still usable.
6. Describe the minimum reproducible sequence and expected vs actual behavior.

## Support scope

The v1.0 hardware-qualified support scope is **Windows 10/11 x64 + K500 over USB HID**.

Bluetooth SPP is implemented but experimental. Bluetooth reports are welcome, but they should not be described as regressions from a qualified Bluetooth v1.0 baseline unless new acceptance evidence exists.

## Good bug reports

Useful reports include:

- exact version/commit when known;
- connected/disconnected state;
- exact click/control sequence;
- source/destination slot for Upload/Mass Upload;
- whether the issue survives reconnect;
- whether permanent storage was involved;
- Support Report / trace where appropriate;
- screenshots for visual/layout issues.

Do not upload private credentials or unrelated personal data.

## Preset reports

Distinguish:

- **SONKUPIK** official preset;
- **LOCAL** user preset.

If an official preset behaves incorrectly, use Sync first, then record its hash. Official presets can update independently from the application binary.

Mode 01 stable rollback authority is `CONCERT HIFI V4`, SHA-256 `9aebeb908295abda1182ddbadc3aa537ea16b4cfea241b64b5a5180e66670e74`.

## Security issues

Do not disclose a security-sensitive vulnerability publicly before a fix if private vulnerability reporting is available. See [SECURITY.md](SECURITY.md).

## Hardware acceptance evidence

Protocol/hardware qualification belongs in the hardware acceptance issue template and should include exact commit, transport, firmware, procedure, reconnect/power-cycle evidence where relevant, and Support Report/trace references.

## Documentation

Start with [docs/README.md](docs/README.md) and the [User Guide](docs/USER_GUIDE.md).
