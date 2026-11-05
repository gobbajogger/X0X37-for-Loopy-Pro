# x0x37 project instructions

**Current rules:** [RULES.md](RULES.md). Older handoffs, archived plans, and prior AGENTS text are evidence, not executable rules.

**Inline protections** (full text in RULES.md):
- No force-push; no merge to `main` outside the sole integrator/merger/publisher; no publish outside the chain.
- No secrets, raw captures, `refs-oscuro`, or proprietary SDK/toolchain/DSP bytes in git. Preserve LICENSE/notices. Existing `*_private.h` / #684 synth headers stay.
- Commit identity `gobbajogger <255244738+gobbajogger@users.noreply.github.com>`; no commit trailers.
- Cloud only; never operate the owner's Mac or devices. Current Elite recovery and flash remain withdrawn; read-only Pro investigation through the assigned integrator does not grant a write, install, flash, or recovery.
- Honest PASS/FAIL/NOT_RUN with exact SHAs. D6 for new installable NIGHTLY: 0 FAIL / 0 SKIP / 0 NOT_RUN. Failed/unrun required gates stay out of the flashable picker.
- Do not edit NIGHTLY catalogs (existing 0.0.24.5 entry untouched).
- Stuck locks: contact the holder (no timeouts); escalate unreachable holders to coordinator and owner.
- Do not bypass a denial by switching transport, encoding, or worker; do not bypass required checks/reviews or silently change auto-merge.

Build pins: [BUILDING.md](BUILDING.md). Licensing: [LICENSING.md](LICENSING.md). History shelf: [docs/rules/workflow-history.md](docs/rules/workflow-history.md).
