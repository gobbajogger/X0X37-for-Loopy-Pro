# Independent host regression review

Reviewed 2026-10-25 in the isolated selectable-engine experiment. This accepts
only the tested host foundation. It is not release, device, full native resource,
real alternate-DSP, or production persistence/UI qualification.

## Reviewed source identity

- Original nightly: `08d3164a265123cb4c84e5104642fd93257d63d7`.
- `firmware/src/app/engine.c` SHA256:
  `92eb33f8e400bb77935bf662698ca52d624b1475572e7fceef6d118db8c774bb`.
- `firmware/src/dsp/synth_engine.h` SHA256:
  `7542f676faf8c59a791b5bc7eb3f524ff0ca25a5a18233dfd96f0bfdf8347181`.

## Findings and disposition

The original note, render, parameter and overload operations retain their303
backend calls through the adapter. Legacy `T_303` commands construct303/schema1
keys; those keys reject another engine identity/schema before touching its setter.
The code adds no public alternate registry, PRESETS overload or fake-engine menu.

Initial review found that switching before draining existing queued notes could
retrigger the alternate from old untagged commands; old MIDI held-state could also
release a new voice. The final implementation drains commands into the old engine,
commits the request before motion/sequencing, panics the old voice, and clears both
source-held bits and the part-held note. The same-core ISR excludes the main-loop
producer during this boundary. Project-held rendering returns before the boundary.
Null-safe activity queries preserve the old zero-initialized query behavior.

Production transport observation uses the backend callback without introducing
new303 panic/tail changes. Generic host transport-stop cleanup is independently
tested. Multithreaded host producer/consumer use is explicitly unsupported by the
same-core mailbox; a future threaded port needs its own synchronized queue.

## Observed tests

- `sh tests/host/run_synth_engine_review.sh`: PASS. Independently written exhaustive
  original303/adapter pot sweep, note/accent/slide/velocity, variable block lengths,
  overload/lite transitions, identity/schema rejection, boundary acknowledgements,
  cleanup, retained settings and 600 ticket wraps.
- The same runner builds the entire host app twice, using original nightly
  `engine.c` and candidate `engine.c` against unchanged current dependencies.
  Its scenario exercises sound edits, overlapping notes, PLAY, overload changes
  and releases. The resulting WAVs match byte-for-byte: 1,267,756 bytes,
  SHA256 `a75af3fc060631d1140b3970ae7a7a492e9937a60f88f852e4e4f85f8d7f7030`.
  WAV output is local test output, not a committed/public binary.
- `X0X_HOST_DEFS=-D_DARWIN_C_SOURCE sh tests/host/project_runtime_test.sh`: PASS
  existing update guard, reset/load/cancel/invalid/transfer/save/sample preservation,
  queued routes, locks, abort, solo and pickup checks.
- `sh tests/host/run_synth_engine.sh`: PASS, 526,336 bit-exact303 samples and host
  alternate lifecycle/identity assertions. The added production-registry test
  rejects unknown IDs, wrong schemas and incomplete callbacks without a pending
  switch; it runs without the host-alternate macro.
- `sh tests/host/run_synth_state.sh`: PASS, 70,509 checks; legacy raw project ABI
  remains 35,052 bytes and the explicit envelope adds 56 bytes before settings.

The independent C regression also passes AddressSanitizer and UndefinedBehaviorSanitizer
(`-O1 -g -fsanitize=address,undefined`), recorded in `independent-sanitizers.log`.

Logs accompany this review. The stock project303 stream runner cannot compile its
`.pool` section on macOS without host defines; this portability limitation is not
a passing stream-migration result. The complete runtime suite needed Darwin
feature definitions because the stock POSIX flags hide system typedefs on macOS.

## Remaining gates

A real alternate engine requires native CPU/code/RAM fit, lifecycle and same-core
command integration tests before registration. No existing resource budget is
relaxed. Full linked Elite/Pro native resource qualification and hardware remain
separate. The new envelope is an isolated explicit codec; existing raw project
serialization is unchanged. Wiring a new file/export route requires the reviewed
schema/UI decision and old-file fixtures through that route. The codec's opaque
unknown-engine settings do not authorize fallback activation or alternate locks.
Task72 and task84 were subsequently supplied and read. Task72's feasibility
snapshot SHA256 is `4df9c3aff26474a214360561293f5575e39e5170def2de3e5be7e798e8ddb9e8`;
task84's preset proposal SHA256 is
`d2c5668af6d25cde6a81daac747bcee840de68b1f0238f766bbc643c23ea3e43`.
Task72's earlier suggestion engine0=303 is superseded for this experimental
foundation by task84's explicit engine1=303, engine0=invalid namespace. Schema1
continues the existing16-pot303 identity. No UI-order or vendor-program identity
is substituted.

The mono callback is deliberately the smallest existing303 boundary. Task72
identifies stereo spread/send downmix as a separate unresolved contract; an
original wobble backend needing stereo must settle and test that extension before
registration. There is no stereo, fade or click-free transition claim here.
Task84's native preset browser, eight-snapshot capacity and step overrides remain
proposals, and this foundation exposes no preset UI or PRESETS remapping.

Task84 calls for unknown required engine/schema rejection before live import;
the codec's opaque retention alone is not that activation/import path. Retaining
unknown bytes while preventing activation is compatible with preservation at this
foundation stage. Full export/editor/storage integration remains explicitly
pending the version/capacity decision, isolated by codec tests without changing
legacy raw bytes. Native evidence covers affected-object compilation only; it is
not full-link RAM/pool/code or runtime deadline fit proof.

Persistence review addendum: the persistence owner independently accepted the
baremetal portability correction (`__builtin_memset`/`__builtin_memcpy` in the
codec, explicit string.h in its host test) and reran the supported
`SYNTH_STATE_CFLAGS='-O1 -g -fsanitize=address,undefined'` path:70509 checks PASS.
Final synth_state.h SHA256:
`3359854aafc26278d65c54cd9a63f34a536f363a2f7a20e776bb647c0de6d9e6`.
The separate native compile probe covers decode/migrate/encode; it does not link
or activate a production file route.
