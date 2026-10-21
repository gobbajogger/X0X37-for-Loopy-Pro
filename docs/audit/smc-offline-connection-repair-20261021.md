# SMC connection repair and diagnostic test — 21 October 2026

## Scope and evidence boundary

The owner will perform the physical test. Before that test, this lane must repair connection faults that can be demonstrated without hardware and make a remaining failure diagnosable. The owner clarified that the earlier description concerned the historical failure; it was not a failed hardware test of candidate `6eaf01f`.

The prior [normal-connection investigation](smc-normal-connection-20261021.md) did not establish the physical cause. This follow-up found three independent software defects in normal connection paths. They are reproducible in the previously offered candidate, regardless of whether they caused the historical incident. The new failure history records the actual boundary and controller/ATT result on the next device test. A timeout identifies an unanswered or unsent operation; it cannot prove its underlying radio cause.

Base: `d9d7953b34c6e045b14ff08770df78fa9bc2d34f`, whose firmware equals `6eaf01f9d4c5325b1cc66e45e7033179b87affef`. The prior handle-zero repair is retained. Integration base remains `7537a46ab49419920428299db8f3321d01e6fd16`. The last verified install remains `a5cb24dd08b40b80e802e03ff0d9c848315a9751`; no new installation is asserted.

## Reproduced faults and repairs

| Fault | Baseline evidence | Repair and retained check |
| --- | --- | --- |
| A refused HCI submission could remain unsent after the mailbox recovered. | Public startup/advertisement tests fail at Reset, Scan Enable, Scan Disable and Create. | Track refusal separately from an accepted command, retry from the service poll, keep the original deadline and never duplicate an accepted command. `ble_midi_command_retry_test.c` covers every startup command, reconnect, cancellation refusal and clock wrap. |
| Automatic cancellation lost ownership of the original Create outcome. | 13 of 21 event-order scenarios fail. A successful Cancel acknowledgement starts scanning before the required terminal result; some late successful links are ignored. | Track accepted Cancel, its acknowledgement and the original Create result separately. Retain unresolved ownership through local timeout and adopt a winning link. `ble_midi_connection_test.c` covers both connection event formats and late success/failure. |
| Peer ATT requests and indications could abort the host's outstanding discovery request. | MTU request during service discovery, indication during subscription, unrelated error and deferred peer-control cases fail. Ordinary fragmented discovery and a matching error remain controls. | Handle peer requests/indications independently, respond at MTU 23, queue bounded required replies under pressure, and correlate a client response to the outstanding request opcode. `ble_midi_att_setup_test.c` covers these paths and 24 fragmentation variants. |

Baseline logs and hashes are in [the evidence directory](../../evidence/smc-offline-connect-20261021/baseline-evidence.json). These are simulated controller/peer inputs, not recovered SMC connection traffic. The reversed failed-Cancel/result ordering is defensive coverage; it is not evidence that this controller emits that order.

Missing Create Command Status also had an unbounded wait. The repaired host records its deadline and retains ownership of a later result. Independent review caught an interaction between that repair and Stop: transport acceptance alone cannot permit Cancel before Create Command Status. An explicit acknowledgement flag now prevents premature cancellation, including after a local timeout. Ten additional retained cases cover late acknowledgement/refusal, stop cutoffs, clock wrap and late live handles.

Two old tests were corrected to represent actual operations: a matching ATT error now has an outstanding request, and the accepted automatic Cancel case reaches the state through the production service instead of treating a seeded opcode as proof of submission.

## Failure evidence available to the owner's test

The central retains a 96-byte passive history until host initialization:

- First failure and latest failure, including timestamp, state, step, status/reason, opcode, handle and raw context.
- Three recent critical events and saturating counts of accepted Create commands, completed subscriptions, failures and disconnections.
- HCI command errors, connection-establishment failures, unexpected disconnections, matching ATT errors, rejected setup responses, queue failures and deadline boundaries.

Successful local teardown does not replace the ATT/HCI/timeout that caused it. Continued empty scanning does not overwrite an earlier specific cause. History survives retries and explicit host starts. A reboot initializes the host again, so the installer collects automatically after installation and the user should keep the resulting files before another reboot.

`ED_DIAG` exports a coherent 26-word block before the legacy live words. The existing SMC1 suffix remains last, preserving the old decoder's alignment. Maximum reply length is 103 words / 522 bytes within the existing 732-byte output buffer and the one-byte word count. Exported runtime host fields are cleared when the host has not initialized, so retained BLERAM cannot masquerade as this boot's successful connection. The existing bootguard/controller diagnostics cover failure before tracing starts.

`tools/smc_connection_watch.py` sends only read-only `ED_DIAG`. It records raw replies, decoded values, missing replies and a final human summary in JSONL, and automatically writes a companion `.summary.txt`. It selects one unambiguous USB input/output pair, honours `--port`, and pins that pair for every request. Missing or ambiguous ports also leave a readable capture-failure record. Its source commit identifies the tools; the native install record provides the firmware identity.

The report requires current central `CONNECTED` and platform `CONNECTED` to agree. It distinguishes a recovered connection from retained earlier failures, and sent requests from requests blocked before submission. Exit zero means diagnostic collection succeeded; it is not a Bluetooth PASS.

## Validation and delivery

[The development plan](../../evidence/smc-offline-connect-20261021/dev-plan.json) selects the changed transport suites, recorded notification replay, actual SMC application input consumer, DIAG bounds/decoder and automatic watcher. AddressSanitizer and UndefinedBehaviorSanitizer remain enabled. Only LeakSanitizer is disabled because this environment cannot inspect process tasks.

The exact committed Elite development candidate must pass `tools/dev_validate.py`, including target code/RAM/pool budgets and the unchanged selected controller. The kit must carry that exact validation record and artifacts. Verify its manifest and native `--verify-only` path after clean extraction into a path containing spaces. The installer then runs the read-only connection watch automatically and saves its report beside the install record. Packaging and installation are separate from the software regression evidence.

No sample quality, mapping, project/drum feature, sample-slot layout or controller-image change is part of this repair. No release build, external Git push or device installation is performed in this lane. Firmware Main Builder remains the single integration and device-write owner; the owner has elected to perform the hardware test.

## Protocol references

- [Bluetooth Core 5.4, HCI functional specification](https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/Core-54/out/en/host-controller-interface/host-controller-interface-functional-specification.html), Vol. 4 Part E, section 7.8.13: Create acknowledgement and cancellation completion ordering.
- [Bluetooth Core 5.4, Attribute Protocol](https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/Core-54/out/en/host/attribute-protocol--att-.html), Vol. 3 Part F, sections 3.3, 3.4.2 and 3.4.7: independent transactions, MTU exchange and indication confirmation.
