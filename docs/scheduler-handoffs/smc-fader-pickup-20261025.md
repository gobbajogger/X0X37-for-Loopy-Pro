# SMC fader pickup source repair

Owner report: installed `e75fb2a584aa22978f352697a5e36fc9f886c3c5`, flashed around 10:40 UTC. Intermittent apparent two-stage fader pickup, screen appearing only after the first stage, and jumping values. The owner cannot identify a reliable trigger; mode changes and slow movement are hypotheses, not diagnosed causes. No device access was used in this source stage.

## Confirmed production defects

1. Receive fader 20 with saved target 64; externally change the target to 80, refresh feedback, then receive 100. The installed code clears `seen` in `smc_notice`, rejects this valid 20-to-100 crossing of 80, and requires another pass. Lock/unlock also discards physical crossing history. Target identity/value must invalidate acceptance, while physical history remains valid across that routing transition.
2. With saved target 64 and a real playing motion lane at 20, receive 0 then 64. The installed code marks pickup ACTIVE but skips `pref_set` and `motion_touch` because the incoming value equals the saved value. The motion lane remains effective at 20; another movement to 65 finally requests motion hold. Acquisition must publish a real write and the existing motion hold request even when the saved value is unchanged. Hold follows the existing engine step boundary; this repair does not promise sample-immediate override of ramping motion.

Both core defects also exist in `58e2c52`; the core notice/fader operations are unchanged there. PR168 adds the lock rearm path. These source reproductions do not establish the complete cause of every intermittent hardware symptom.

## Narrow repair and authority

Only `firmware/src/app/smc_mixer.c` changes at runtime: retain physical history across target refresh and lock/unlock, require meaningful movement before a rearmed latch accepts, and publish once on new acquisition even for equal saved values. Unknown history still resets on reconnect/project/blocked-input boundaries. Invalid destinations retain physical history without writes. No new static state, smoothing, protocol changes, generic knob changes or project format changes.

The one SMC latch is `smc.picked`, with current `smc.target`/`smc.value` refreshed by `smc_notice`. Its values are parameter source units normalized by `smc_scaled`. Every mapped main/drum fader descriptor has max127; its source and normalized values are identical. DAW pitch bend decodes `(MSB << 7 | LSB) >> 7`; User CC40–47 supplies that 7-bit value directly. Level dB text uses the existing nonlinear formatter after acceptance; it is not a second pickup target.

Central BLE callback records source4 INLOG then queues each decoded packet. `smc_poll` dispatches directly to `smc_fader`; host User packets are consumed by `smc_midi_host` before generic MIDI dispatch. Keyboard `smk_faders` owns its independent physical inputs and is not called by either SMC route. Relative knobs remain separate and have no absolute pickup latch.

`smc_show` executes on changed received fader values independently of pickup acceptance. UI reads live parameter values and the actual SMC acceptance bit. The installed code already reveals a meaningful received move before pickup; a delayed first overlay therefore requires received-input evidence. No source evidence proves an SMC internal takeover layer. Outgoing feedback remains pitch bend `[0xE0+n, value, value]` after target changes/acquisition plus normal note LED feedback. Whether this affects the physical controller's transmission is unproven; changing feedback blindly would risk compatibility.

## Production-linked evidence

`sh tests/host/smc_pickup_acceptance_test.sh` compiles the actual UI, SMC queues, parameter store, engine, motion and framebuffer. Its write recorder forwards to the real engine; target/pickup are not mocks. New tests fail at runtime on installed source for crossing and equal-value acquisition, independently reproduced by the reviewer.

- 192 DAW/User crossings across main/909/808 mappings, all eight strips, both directions, fine/coarse endpoints. Separate assertions for screen activation, actual rendered status, write count and destination.
- Lock/unlock, part/page/bank changes, parked equal packets, first input and reconnect.
- Real active motion lane; equal-target acquisition emits exactly one write and requests hold without a second movement.
- 262144 fine pitch-bend packets across all 16384 raw codes, both directions on eight main faders; 2048 expected writes, byte-boundary checks, exact feedback values and rendered ACTIVE. Host input-to-parameter path is monotonic. `slow-ramp.csv` is scratch output, not hardware data.
- 4096 deterministic mixed events covering edits, parked switch-away/back, edits while away, kit/bank/part selection, lock/unlock, reconnect, duplicate packets and before/after feedback timing. An independent state model checks writes, targets, acceptance and movement UI. Total835361 assertions pass.

Legacy lock/reachability checks now establish a physical sample genuinely on the non-crossing side before expecting pending pickup, and require retained history after edits. Their behavioral assertions remain. Existing route tests cover five input paths. No broad full gate or Actions dispatch was used.

## Size and integration

Scoped Elite DEV measurement recompiles the changed application translation unit and relinks against exact verified installed-candidate objects with the existing pinned JieLi compiler/image. All generated headers match. Result: image552620B, code free52B, RAM free300B, pool free8324B, delta+16B image and0B static state. An initial host-generated font measurement was discarded; the final run uses the exact pinned Linux-generated headers. This is affected-unit/app-relink evidence, not a new installable package or both-model DEV CHECKED qualification. The combined owner must apply this isolated commit, rerun the affected acceptance/lock/reachability tests, independent combined review and both final native model budgets. Its existing source/budget changes invalidate reuse of the standalone final image budget.

Integration owner: `01a172a2-a5d4-76fd-8327-fd58a62af01f` combines PR168/188/189 on its separate branch. No edits were made to its branch or installed-candidate files/artifacts. No flashing, merge, GUI or public binaries.

## Bounded read-only hardware trace for the parent

A hardware operator may collect existing INLOG while the owner uses the unit normally; no need to force a failure. Preserve current installed identity, SMC preset/routing and initial page/part/bank/lock. Record UTC gesture labels separately: first physical movement; screen appears; pickup first/second stage if observed; mode/page change; edit while away; lock/unlock; symptom absent/present. A timestamped owner phone video can supply physical movement/screen timing if the owner chooses. Do not infer a cause from gesture speed.

Use existing `tools/ble_autotest.py --diag-only` once before/after, always with that flag (the tool's default mode performs restart tests). For a bounded normal-use window, the operator can run this read-only command from the repository, saving output privately:

```python
import sys, time, json, datetime
sys.path.insert(0, 'tools')
import ble_autotest, inlog_fmt
end = time.monotonic() + 60
while time.monotonic() < end:
    before = time.monotonic()
    reply = ble_autotest.ask(inlog_fmt.ED_INLOG, timeout=1, tries=1)
    print(json.dumps({'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
                      'host_monotonic': before, 'raw_reply': reply}), flush=True)
    time.sleep(max(0, .5 - (time.monotonic() - before)))
```

This sends only editor command38 and changes no settings, logging firmware, pickup or sound state. Existing ring is64 entries, shared with other events: retain raw overlapping snapshots, deduplicate by overlap/ordering, align device `fm1_ms now` with each host request bracket, and flag missing overlap/full windows as possible data loss. Do not deduplicate by value: identical parked packets are material evidence. Host User input is not recorded as source4 by the central callback; that route needs its actual host MIDI trace in parallel.

For each SMC pitch-bend event report status/channel, LSB, MSB, raw14-bit, normalizedMSB and device timestamp. Compare first received changed value with owner-labelled physical move and overlay. Received monotonic values with pending screen implicate an app/UI state path; no received movement before overlay supports an upstream/controller hypothesis only when ring coverage and transport counters exclude capture loss. Nonmonotonic input demonstrates an upstream property but does not identify its cause. INLOG contains inbound events, not outgoing fader feedback or accepted writes, so it alone cannot prove a feedback loop or exact engine acceptance. Do not disable feedback or change preset/firmware during this first capture. A clean60-second window does not disprove the intermittent report.
