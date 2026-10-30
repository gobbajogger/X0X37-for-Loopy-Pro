# Free Play pad opt-out regression (PR #715): host-test-only qualification note

Status: HOST TEST ONLY - NO QUALIFIED HARDWARE CANDIDATE

PR #715 changes only the host regression. It does not change production firmware
and is not, by itself, a device-test candidate. Do not install or hardware-test
PR #715 as though the test-only commit were firmware.

Source: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/715
(`fix/freeplay-pad-optout-regression`). The regression checks both compile-time UI
flag states for Miami Free Play pad handling: rate selection disabled
(`X0X_DEV_WOBBLE=0`) and rate selection enabled (the normal enabled variant).

## Qualification required before any device test

A device checklist becomes runnable only after an integration candidate that contains
the production pad behavior has a separately qualified firmware build for each UI-flag
variant:

1. Build and qualify the disabled variant with `X0X_DEV_WOBBLE=0`.
2. Build and qualify the enabled variant with the rate-selection UI flag enabled.
3. Record the exact source SHA, model, build configuration, and qualification result
   for each variant. One variant's build does not qualify the other.
4. Run the PR #715 host regression against the exact source/configuration used for
   each qualified variant.
5. Only then create or use device steps tied to those exact qualified packages.

Until both variant-specific candidates exist, hardware status is **NOT_RUN**. A host
PASS for PR #715 proves the regression logic only; it is not a firmware build, device
PASS, or authorization to flash hardware.
