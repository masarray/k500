# Voluntary QRIS Donation Prompt

This feature is intentionally isolated from K500 hardware and preset state.

## Runtime behavior

- Count one launch day per unique local calendar date.
- Reopening the app multiple times on the same date does not increment the counter.
- The automatic prompt becomes eligible on the third distinct date.
- The OK action is disabled for three seconds and shows a visible countdown.
- Acknowledgement is persisted once; donation does not unlock or restrict any feature.
- App-update prompts are arbitrated so they never stack on top of the donation prompt.

## QRIS safety gate

The repository never fabricates a QRIS payload from merchant labels or NMID text.

Production auto-prompting requires the owner-supplied static merchant image:

```text
resources/support/qris-sonkupik.png
```

The current accepted asset is a 1240 × 1748 official QRIS merchant poster
showing:

- Merchant: `SONKUPIK, AUDIO DEVELOPER, DIGITAL & KREATIF`
- NMID: `ID1026551401775`

The compact popup crops only the QR region from that original poster; it never
regenerates or rewrites the payment payload. Merchant identity and NMID are
rendered separately below the QR for a clear visual cross-check.

CMake remains fail-closed: if the PNG is ever removed, production auto-prompting
disables itself while the rest of SonKuPik K500 continues normally.

For any future QRIS replacement:

1. obtain the official static QRIS directly from the authorized merchant/provider;
2. scan it using at least two independent banking/e-wallet applications;
3. verify the displayed merchant identity and NMID;
4. replace the PNG at the exact path above;
5. review/update the crop rectangle if the poster geometry changed;
6. configure and rebuild so CMake embeds `:/support/qris-sonkupik.png`;
7. run `--donation-prompt-preview` and scan the QR from the built application before release.

## Deterministic validation

`k500_donation_prompt_selftest` validates distinct-day counting, same-day
reopen behavior, the three-day cap, fail-closed QRIS behavior, and QA preview
isolation without network or K500 hardware.

The QA-only command-line switch:

```text
--donation-prompt-preview
```

forces the modal to open without modifying the usage-day counter or consuming
the user's one-time acknowledgement state.
