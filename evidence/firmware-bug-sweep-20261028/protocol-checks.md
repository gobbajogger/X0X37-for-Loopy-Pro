# MIDI queue repair evidence

## Scope and source

Baseline: `7e4bbfd95da1d9ad86c80541b9bceae48231d006`.
Producer branch: `fix/midi-overflow-order-20261028`.
This is the bounded protocol unit of the owner's firmware bug sweep. It maps
to LX-REQ-022 transport order and LX-REQ-007 transport continuity. The existing
sole firmware integration owner is unchanged. No remote, device or publication
action was performed by this producer.

## Reproduced defects

1. A saturated MIDI ring replaced its earliest disposable CC in place with an
   incoming protected message. If a pending Note On followed that CC, its Note
   Off was emitted first and the final note state remained held. Start/Continue
   and Stop could likewise reverse. This affected the USB input/output helper
   and the BLE output mirror. Existing retention tests counted the protected
   messages, but did not establish their order.
2. The BLE ring had 56 slots indexed with wrapping `uint32_t` counters. Since
   56 does not divide 2^32, counter rollover aliased live slots even without
   overflow. The same exact-source regression reproduces this independently.

The repair removes the disposable event, preserves the order of all retained
events, and appends the incoming protected event. TIMER5 source masking protects
slot movement and the ingress slot-read/cursor-advance pair. It restores the
previous source enable bit and does not change global interrupt state. The
loader retains its existing single-threaded, IRQ-off path. BLE capacity is now
64 slots, adding 32 pool bytes; native budget acceptance remains a separate gate.

## Executed proof

Environment: Linux x86_64, GCC `13.3.0` (Ubuntu `13.3.0-6ubuntu2~24.04`),
Python `3.12.14`. The new regression compiles production extracts with
`-std=c99 -O1 -g -Wall -Wextra -Werror` into a private temporary directory.

| Check | Observed result |
| --- | --- |
| `python3 tests/host/midi_overflow_order_test.py --source-root` against a pristine baseline checkout | Exit 1; 249,093 checks, 177,950 failures; actual baseline capacities MQ=64, BLE_OQ=56. |
| `python3 tests/host/midi_overflow_order_test.py` on the repair | Exit 0; 264,909 checks, zero failures; MQ=64, BLE_OQ=64. The changed count follows the actual larger BLE capacity. |
| `ASAN_OPTIONS=detect_leaks=0 python3 tests/host/midi_overflow_order_test.py --sanitize` | Exit 0; the same 264,909 checks; ASan and UBSan enabled with fail-fast recovery disabled. |
| Temporarily remove only USB producer source masking in a copied source tree | Exit 1; one failure: TIMER5 sees a partially shifted ring. |
| Temporarily remove only BLE producer source masking in a copied source tree | Exit 1; one failure: TIMER5 sees a partially shifted ring. |
| Temporarily remove only ingress consumer source masking in a copied source tree | Exit 1; 66 failures; an interrupt between slot read and cursor advance loses retained sequence. |
| Compile and execute the production shared queue with `FELUCCA_LOADER=1`, no native IRQ declarations, strict GCC flags | Exit 0; ordered release retention passes without a native IRQ dependency. |

Coverage includes all three queue paths; Note Off and zero-velocity Note On;
Start, Continue and Stop relative to previously queued transport; every pending
event position; ordinary physical wrap and uint32 counter rollover; CC preference;
fallback to another disposable event; full protected queues; dropped ordinary CC
and Clock; previously masked source/global interrupt states; and deterministic
TIMER5 injection throughout producer movement and the actual ingress consume.
Interrupt injection is a host scheduling model, not real-device timing proof.

The first default sanitizer invocation exited 1 because LeakSanitizer could not
read `/proc` under this host. It is not a pass. The explicit retry above disables
only leak detection; leak checking remains NOT_RUN. ASan/UBSan checks pass.

## Existing covering tests

The following run executed once:

```sh
X0X_TEST_JOBS=1 sh tests/run_tests.sh midi-mq-noteoff-overflow midi-mq-rt-overflow midi-out-mq-noteoff-overflow ble-out-noteoff-overflow ble-out-rt-overflow trs uac smk-clip-usb-clear smk-clip-knob
```

Eight selectors passed. `smk-clip-knob` failed at link time because its extracted
autosave code now calls `ui_boot_flash_busy`, which the fixture did not provide.
The exact pristine baseline reproduced the same missing-symbol failure, exit 1.
The fixture now provides the inactive-state implementation used by the project's
other isolated fixtures: this clip/MIDI test has no active boot capture or
maintenance lease. No production boot code, assertion or warning flag changed.
Its focused rerun, `sh tests/host/smk_clip_knob_test.sh`, exited 0 with `PASS 0 fail`.
The eight unaffected passes were retained, not rerun. Existing IRQ stubs were
extended for the newly used source-mask functions; BLE retention fixtures read
the actual production capacity.

The runner's separate SMK package skip is not package proof. Native build,
pool/RAM acceptance, independent Sol review, integration and hardware validation
are pending consumers of the combined candidate, outside this producer receipt.
