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

Production auto-prompting requires this exact verified static merchant image:

```text
resources/support/qris-sonkupik.png
```

When that file is absent, CMake does not package a QRIS resource and the
production donation prompt remains disabled. The rest of SonKuPik K500 works
normally.

Before adding or replacing the PNG:

1. obtain the official static QRIS directly from the authorized merchant/provider;
2. scan it using at least two independent banking/e-wallet applications;
3. verify the displayed merchant identity;
4. place the verified PNG at the exact path above;
5. configure and rebuild so CMake embeds `:/support/qris-sonkupik.png`;
6. run the prompt in QA mode with `--donation-prompt-preview`;
7. scan the QR from the built application before release.

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
