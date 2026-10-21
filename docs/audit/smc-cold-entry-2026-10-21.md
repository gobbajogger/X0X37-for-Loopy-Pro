# First installation and cold entry — unresolved delivery gates

Software package checkpoint 54ede7a builds and validates both the forward vendor
profile and exact 816 logical rollback. Actual package/loader transactions pass
native and sanitized replay. This does not by itself make the package usable
from the owner's existing firmware: per-unit head acquisition and a reachable
never-started-radio update route still need implementation/review.

## Exact historical 53d source

Examined commit 53d5654c9bbaec10b64c55fb60d0e7d857ba8156, tree
7a7c1da22181ff9a52f3bf27a3db4660c22af420, not an assumed current device image.

`main_fm1.c` loads the project, applies it, then calls
`ble_prepare(proj.set.bt & 1)` and `ble_start`, before core1/audio startup.
With saved Bluetooth OFF, `ble_guard_boot` returns zero and `ble_boot_on`
remains zero. `ble_start` returns before host/controller initialization;
`ble_attach` returns because `ble_c0_ready` remains zero. Later UI Bluetooth ON
only requests restart; it does not start the controller during that boot.
Normal engine/editor initialization still completes, including sample reads.

ED_DIAG includes the live `ble_boot_on`, `ble_up`, `ble_want`, `ble_host_on`
fields and raw reset causes. A warm OFF toggle after startup retains
`ble_boot_on=1`, so it must not pass a never-started check. Persistent crash
mailbox/guard words alone are not current-boot proof. Raw reset bit meanings
have not been established sufficiently to replace explicit physical power-off/on.

## Remaining boundaries

The existing native updater holds CPU1 before reset (`fm1_enter_update` calls
`fm1_core_reset`). It does not explicitly stop ALNK DMA on this normal path.
The exact active DMA address/direction versus SPL decompression must be checked;
loader-entry quiescence cannot by itself prove a preceding interval was safe.
This is inherited updater behavior, not evidence of a newly observed failure.

There is no existing exposed Elite head-read endpoint in this firmware. Debug
PEEK excludes the flash head; sample and controller reads are fixed elsewhere.
FM1-V15 rescue is not an Elite acquisition route. A proposed RAM-only head-export
loader must retain existing owned staging ranges, prove cleanup/restart, and
never write main/head/samples. No head reference may be fabricated from a build
template. Raw head data/key material stays on the owner's machine, not Git or
shared diagnostic uploads.

The candidate's current `safe_main` is crash-triggered and lacks editor service,
so it cannot be presented as a manual sample-backup entry. No invented button
sequence or deliberately induced crash is part of this plan.
