# Independent TouchOSC lifecycle source review

Reviewer: `/root/lifecycle_design_review`, 2026-11-14. Read-only review of implementation and tests; only this review evidence file was written. No native runtime operation, source edit, Git mutation or test rerun. This is not Sol review or a formal GitHub approval.

## Result

The implementation is suitable for an explicitly **UNQUALIFIED draft**. No blocking implementation defect was found in the reviewed delta. F1 below is resolved in the final comment/documentation delta. Native tag survival, init-time transmission and first-tap recovery remain **NOT_RUN**, so neither defect may be reported as natively repaired or accepted. Source draft delivery does not satisfy the assigned native qualification stage.

### F1 — resolved: unverified source-comment assertion

The first reviewed `controller.lua` said `Native properties survive the editor's script-context reset`. That behavior has not been measured on this changed candidate. The final comment now describes recovery of markers retained by native controls and explicitly requires native qualification. The README discloses the official-runtime HTTP 403, native tag/init-send NOT_RUN, editor-time limitation, reserved tags, RELEASE before editing/saving, and no attempted-OFF replay. This resolves the finding. No implementation behavior changed in this final delta.

## Implementation assessment

- `disarm()` sets `armed=false` before releasing notes or resetting native x values. `release()` removes its active entry and clears the native marker before sending OFF. A synchronous x callback therefore cannot duplicate the release or create another ON.
- The local ON records the original human channel. Re-entry accepts only canonical decimal channels 1–16 under the reserved `dnbox-held:` prefix on allowlisted note controls. Normal release retires the marker on the existing send attempt; re-entry does not retry an already attempted OFF.
- `init()` preserves existing active entries, recovers valid retained markers, and invokes disarm. This covers preserved and recreated Lua contexts at the API boundary. Native context/property behavior and successful init-time send are separate unresolved requirements.
- Received notes still change presentation only. They create neither active entries nor markers, preserving the no-echo requirement.
- The packer adds an empty typed tag property to buttons without changing layout geometry or bindings. The new packing test checks both output forms and all 20 buttons.
- `button.lua` remains unchanged. No invented stop callback, MIDI replay, transport change or device command was introduced.

Native tags are user-editable control properties. The companion must reserve note tags for this state and instruct users to RELEASE before editing, saving, closing or changing routes. Saving outstanding markers can preserve an OFF obligation for a later load. Re-entry is only a safeguard when scripts resume; editor-time sound cannot be bounded by the script timeout, and endpoint delivery cannot be guaranteed.

## Test/evidence review

Read `lifecycle-red.log`, `lifecycle-green.log` and `packing-lifecycle.log`. Recorded old-controller sensitivity: **4 of 7 new scenarios FAIL**, including VM re-entry, preserved-context init, stale timeout x and RELEASE/channel reset. Recorded changed-controller result: **7 of 7 PASS**. Recorded new packing case: **1 PASS**. These are producer-executed API-boundary/packing receipts, independently inspected here, not independent native execution.

The new Lua fixture models synchronous root re-entry on x changes and recreates the Lua environment while retaining its native-control stand-ins. It covers original-channel recovery, multiple outstanding notes, idempotence, first-tap recovery after timeout, RELEASE/channel reset, receive-only no-echo, attempted-OFF retirement and invalid markers. It explicitly does not prove native property persistence or delivery. Inherited 8 Lua / 4 packing / 206 TouchOSC / 8 XML results remain historical scoped proof; they do not qualify this changed artifact.

Official checksum and archive access receipts record HTTP 403 / Cloudflare 1010. They establish this executor's current download block, not project-wide runtime absence. No denied operation was retried during this review. Actual OS MIDI reconnect, physical touch/multitouch, mobile/macOS, closing while held and editor-time release remain unqualified.

## Official API basis

- [Control API](https://hexler.net/touchosc/manual/script-objects-control): documented callbacks include init/update/value/pointer/receive handlers; no pre-stop handler is listed. Init is called on editor-to-control-surface transitions and certain resume conditions. Properties and values are script-accessible; value changes can invoke callbacks.
- [Properties and values](https://hexler.net/touchosc/manual/script-properties-and-values): tag is a common string property; button x is a floating value from 0 to 1.
- [Editor control properties](https://hexler.net/touchosc/manual/editor-control-properties): tag is freely editable and properties can be changed by scripts.
- [Script execution](https://hexler.net/touchosc/manual/editor-script): scripts execute when switching to control surface or explicitly running them in the editor. This does not establish a shutdown callback or automatic editor-time timeout.

## Reviewed input SHA-256

All paths are beneath `web/offline-sequencer/touchosc/` in `/workspace/touchosc-lifecycle`. The final six source hashes and both artifact hashes were independently compared with `candidate-inputs.json` and matched. The earlier controller hash was `8e9fca2b890bd306d9ef98c38e3de5d4b086d53112ad1d4ee3d8337ac94badb8`; only its comment changed after the behavior review.

| Path | SHA-256 |
|---|---|
| controller.lua | `82573ce4dd7531d1a9dac6e5071238d0db2e9bdbde65cf866c8a76ace3d8af7d` |
| button.lua (unchanged) | `347db93b4e59eef2b4a189f2a672a4a1b81f48d13dfb8fc949bcdbad92c36068` |
| pack.py | `fad7961e70f2cf5960d6f1dcf991053c8d6c4244b9c14cb1af3eee1b910ad36b` |
| pack.test.py | `4ea0a2f767f1146c1de5da1df5aaaddf2302605b8a1fa13537f7cba21520a56d` |
| lifecycle.test.lua | `4908ec2fb4125ba55c87b9d58b9badf456db852208838e6abe19d5a39d928e19` |
| README.md | `901afa559c6ae7adafee9a6fb92673457297e10719f2dd3473ea8456316db349` |

Generated artifact pins (UNQUALIFIED; no native execution):

- XML: `fae67d328d915512c2bc4fbbf045c6da01e7ffb0d4bb6c9ace4de9e6a8ad236f`.
- TOSC: `efe39a1e2a692ba6b4a352a036e6249b271cd13c024047fbc9bc4548442effe3`.

Next consumer: existing parent/integration owner. Retain the candidate's UNQUALIFIED label and native qualification as pending until a legitimate official runtime is available. No merge or publication follows from this review.
