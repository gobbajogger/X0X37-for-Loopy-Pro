# TouchOSC D1/D2 lifecycle source candidate

State: source candidate reviewed; exact native qualification BLOCKED by official runtime HTTP403. This stage does not establish a natively repaired or safe companion. Parent retains integration. PR910 is untouched.

D1 had an outstanding ON when native Ctrl+E stopped scripts; init discarded tracking on return. The supported return-time safeguard records a local note's original channel in its native tag before ON. At init, valid allowlisted markers recover outstanding releases; disarm emits OFF and clears marker/x. No documented pre-stop callback exists, so editor-time sound remains unbounded. RELEASE before editing/saving/closing; receiver recovery remains necessary if already stuck while scripts are stopped. Tags are editable serializable document state; no close/crash/document replacement guarantee.

D2 had a timeout OFF but x remained1, consuming the first re-armed tap. Disarm now resets note x after clearing active debt, with armed=false before callbacks. No reentrant ON/duplicate OFF is permitted. Existing button script is unchanged. Received MIDI remains presentation-only. Each normal OFF attempt retires its marker and is never replayed for endpoint recovery.

Measured proof: baseline Lua5.1.5 exit1,4/7 new regressions fail; changed source exit0,7/7 pass. New packing test exit0,1 pass verifies empty typed tags in XML/TOSC. Native-derived tests cover fresh/preserved Lua-context init, original channels, multiple notes, idempotence, timeout first tap, RELEASE/channel reset, received-note no-echo, invalid markers and attempted-OFF retirement. These use an API model and cannot prove native persistence or delivery. Independent source reviewer approved only an explicitly UNQUALIFIED draft; final source/artifact hashes match candidate-inputs.json.

Inherited8 Lua/4 packing/206 native/8 XML passing proof remains historical at679a2c9; no unchanged suite was rerun or relabelled current-artifact PASS. Original71-member lifecycle packet and embedded47-member repair packet hashes verified. Original failed exploration remains unchanged. New intermediate Lua-pattern failure is documented in development-failures.md.

No TouchOSC/native process started here. Official runtime checksum and archive returned403 Cloudflare1010. Library title search found prior evidence, not an official runtime archive. No archive retry, CDN workaround, licence purchase or substitute binary. Debian Lua5.1.5 ran task-locally; Xvfb was extracted but never started. No physical MIDI, hardware, firmware, protected-ref merge, publication, active-session cleanup or competing writer.

Next consumer: existing creator assigns legitimate native executor on exact artifacts using successor-handoff.md; current source owner releases scope at final return. Stop-reason receipt and validation give the concrete resume condition. Standard thread creation/read tools are absent; successor creation/consumption PENDING_PARENT.
