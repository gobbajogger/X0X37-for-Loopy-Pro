# Proven normal MIDI workflow: preservation and owner handoff

The owner-operated Elite update on 2026-10-24 completed through normal USB-MIDI:
exit 0, transfer 100%, reboot `SMK-37 Elite_900`, editor `X0X DEV`, and a
65,536-byte controller readback matching CRC32 `4a400899`. Three sample-slot
layouts and their bytes matched before/after; calibration exports matched;
MIDI PING passed 10/10. Two Bluetooth observations were READY with zero drops,
increasing controller polls and no new crash.

These observations do **not** establish full hardware validation. SMC stayed
scanning with no connection or traffic. Physical keys, pads, audio and SMC controls
were not tested. Project/settings were not independently compared. This protocol
cannot read back the full installed application: exact pinned transfer and runtime
identity passed; exact installed-app byte readback is **unavailable**, not PASS.
See [the derived nonsecret receipt](execution-summary.json). Original captures,
backups, calibration/sample bytes, controller bytes, SDK and original build logs
remain local. Nothing in this preservation work runs hardware or rebuilds firmware.

## What is preserved

[reviewed-workflow.patch](reviewed-workflow.patch) is the **unaltered** seven-path
reviewed source/handoff patch, SHA256
`584ea440bc642b5e741d12f69c135a0049056f71cb620b900535930e6d4ea01a`.
It applies only to accepted source
`54f2ffbc3124f7449a69067dc8215a2d91a2e675`, root
`3b5cadafba33f4c1b116660bc9d19a27a892337c`. The resulting updater SHA256 is
`7e5056036aeae47cf2ce2175de4eef7c255b897349eff8153ef7753c8f775dfa`,
the same bytes used in the successful execution. This PR preserves a replay and
handoff; it does not replace the repository's active updater or validation code.

The patch contains:

- Exact Elite/Pro model checks, sole compatible MIDI-pair admission, duplicate and
  stale endpoint refusal, bound reconnect/readback/restore, checked sends,
  phase-finish validation, and bounded idle/total transfer time.
- The exact known Elite app-only Felucca head condition. It pins the complete
  logical payload and loader hashes, checks loader headers/CRCs/attributes/entry/
  lengths/bounds and reference-key/decrypted app-header CRC. Other packages retain
  historical head equality/refusal; a marker, `_900`, or `--force` alone cannot
  authorize the exception. Invalid packages with a reference refuse.
- A distinct original-build mapping through the existing `--dev-record` route.
  It requires authentic original log bytes, accepted Git objects/trees and
  independent review evidence. It retains `BUILD_SUCCEEDED_UNREVIEWED` as the
  original status and reports `ORIGINAL BUILD MAPPED`; it does not manufacture a
  schema-2 `DEV CHECKED`, new compilation, release PASS or device qualification.
- Two existing fixture corrections, an offline-default pinned launcher and its
  relative-path descriptor. The patch embeds no firmware/controller/SDK bytes.

The candidate supplies its loader; prior loader installation is not required.
Stage1 validates, stages and reads it back before reboot commit: staging is a
write. The loader checks the actual device chip key and decrypted app-header CRC
before the first application erase, and writes only the app range. The offline
reference check alone does not attest the actual device.

## Existing ownership and exact compatibility

At reconciliation, firmware `nightly` still points to the accepted commit above.
Existing Data Safety [PR #114](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/114)
owns native OTA safety; current head
`757c6e5b168c8c737e758346eb26e23134a5bcbd`, root
`9aa0d0d32a657c3b7a8a6e4150f290954fa19cf6`. Its newer physical-ID/atomic-output
contract deliberately disables ordinary MidoBackend writes. This replay uses the
reviewed singleton/exact-model method. It does not supply cryptographic unit
identity, identify two indistinguishable units exchanged between modes, or close
every endpoint-enumeration/send race.

Consequently this is an owner handoff, not a competing activation of those same
transport fixes. Do not apply the full patch on top of #114 or remove its guards
to make tests pass. Data Safety and independent QA should compare the exact
transport contracts, retain applicable newer backup/extraction guards, and review
any combined updater before adoption. The preserved known-head condition and
original-build adapter are separately identifiable changes; an adapter's updater
hash must never be casually repinned to a newly combined implementation.

[PR #113](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/113) has older
family/model reconnect work;
[PR #121](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/121) owns fixture
reconciliation against #114;
[PR #125](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/125) preserves
future identity/native-discovery work. No branch belonging to those owners is
overwritten here. Browser WebMIDI and its admission rules remain separate: native
CLI results neither prove nor disprove browser behavior. Production promotion,
firmware publication and source integration remain separate owner actions.

## Offline reproduction

From a complete clone containing the accepted Git objects:

```sh
python3 -B docs/native-midi/replay.py --result /tmp/native-midi-replay-new.json
```

This creates a disposable local clone at the exact accepted tree, checks and
applies the exact patch there, verifies the updater hash, then runs the affected
host suites and 17 transport boundary tests. It does not fetch, import the real
MIDI backend, invoke the hardware launcher, or compile firmware. A new result path
is required. Temporary source changes disappear on completion.

To repeat the additional 27 head and 31 original-build mapping checks, an owner
with the **original qualified local preparation directory** can add:

```sh
python3 -B docs/native-midi/replay.py \
  --private-evidence "$QUALIFIED_PREPARATION" \
  --result /tmp/native-midi-private-replay-new.json
```

That directory must contain the original `inputs/`, `review/` and
`prep-evidence/accepted-independent-qa.json`. The tests read those files and
mutate only temporary copies. Do not reconstruct original log bytes, publish
them, or substitute a new script/result for a hash-pinned original review.
The portable test scripts differ only in input locations and removal of writes
to the original review result files. Missing private fixtures leave those checks
NOT RUN in the public-only invocation. Darwin skips Linux-specific validator
runner cases; the package/Node parity fixture is optional and absent here.

## Repeating the normal MIDI command in a future authorized execution

First reconstruct a **fresh isolated owner-local** preparation at the exact
accepted commit and apply the preserved patch. Restore only the verified original
local inputs/evidence at the descriptor's relative paths. Preserve the original
successful preparation and its existing backup; do not delete or overwrite it.
The launcher pins the source tree, helper files, mapping descriptor and exact
package/controller/reference hashes. Its default command is offline preflight:

```sh
cd "$QUALIFIED_PREPARATION"
/usr/bin/python3 -B flash-handoff.py
```

For a separately authorized hardware session, in a foreground terminal with only
the intended compatible Elite pair attached, the normal command is:

```sh
/usr/bin/python3 -B flash-handoff.py --execute
```

The fixed backup destination must be fresh. The launcher delegates to the native
updater with `--port 'X0X FM-1'`, matching controller, accepted commit,
`--config dev`, original-build descriptor, explicit head reference, fresh backup
and `--yes`. It uses no force, sample opt-out or controller omission. The updater
holds the exclusive hardware lock and verifies backup before UPGRADE, then model,
reboot, samples and controller readback. Stop on refusal; interrupted staging or
write has an uncertain device outcome until checked, never an automatic PASS.

### Observed OS/Codex permission boundary

In the preceding preparation history the sandboxed CoreMIDI client failed with
OSStatus `-10833`; an ordinary supported `require_escalated` nonmutating query
succeeded. The recorded successful flash also used per-command
`require_escalated`, exited 0, and left no denial or approval pending. These are
observations of those commands, not proof of a particular macOS TCC entitlement
or a prompt-free guarantee for later sessions.

When an authorized ordinary MIDI command needs sandbox escalation, request that
same command through the supported command escalation mechanism and monitor its
terminal result. Do not use a ROM launcher, permanent root, security-setting
changes, GUI automation or alternate execution to bypass a denial. This
preservation task performs no permission changes or hardware commands.
