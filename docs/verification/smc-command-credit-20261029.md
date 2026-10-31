# SMC startup command-credit repair

## Result and limit

The dual-peer HCI router dropped a valid command-credit event. A synthetic trace
reproduces a startup stall on B `5b934a285d60706343248fc84a1e1065f148d3b0`:
Reset completes with zero command credits; the controller then announces one
credit with `0e 03 01 00 00`. B sends no Event Mask command and reaches FAILED.
The corrected router accepts the announcement and completes all seven startup
commands. This prevents that host-side connection failure without changing the
controller, peer count, settings, project data, samples, calibration or locks.

This event sequence is a test input, **not an observed hardware capture**. It does
not yet establish the cause of the owner's physical B failure or prove a repaired
radio connection. A `401b28d332f6d093963ababe7e7c6f13918abf70` remains the
owner-confirmed first-peer baseline. B differs from A in both its dual-peer host
and controller link configuration/build recipe. Controller initialization and
physical acceptance remain open.

## Protocol and correction

[Bluetooth Core 5.4, Vol 4 Part E, sections 4.4 and 7.7.14](https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/Core-54/out/en/host-controller-interface/host-controller-interface-functional-specification.html)
describes opcode zero as an unsolicited command-credit announcement.
Command Complete has a one-byte credit count, two-byte opcode and command-specific
return parameters. Opcode zero has no command return Status byte.

Previously the pair router required at least four parameter bytes for both
Command Complete and Command Status. It therefore lost the three-parameter
Command Complete credit announcement. Parse those envelopes with their respective
minimum lengths. Still require a Status byte for real command results. An opcode
zero event updates credit only: it cannot release an outstanding command owner,
advance discovery, or claim completion. Command Status keeps its four-byte minimum.

## Executed host evidence

- New `ble_command_credit_event_test.sh`: delayed credit at each of seven startup
  boundaries; outstanding-owner isolation; zero credit; truncated envelopes;
  missing command Status; Command Status credit announcement; actual rejection.
- New test on original B: assertion failure (exit 134), ignored credit event.
- Same production startup trace: exact A reaches SCANNING with seven commands;
  exact B stalls after Reset and reaches FAILED (exit 1); corrected B reaches
  SCANNING with seven commands.
- Dual-peer suite: 167 checks pass. Its credit fixture now uses the real
  three-parameter envelope instead of an invented extra Status byte.
- Handle matrix: dual transport at handles 0, 1 and 64 (167 checks each), complete
  discovery at each base (11 checks each), and 38 zero-handle lifecycle checks pass.
- A single-peer discovery replay at handles 0 and 64: six checks each pass.
- Selected runner: command-credit, dual-peer, first-notification, MIDI-receive and
  capture-replay suites pass. Package checks were not run by this host selection.
- GCC 14.2.0; AddressSanitizer and UndefinedBehaviorSanitizer pass with
  `ASAN_OPTIONS=detect_leaks=0`. LeakSanitizer itself cannot run under this
  environment's ptrace. These fixtures allocate no heap memory.

An initial A replay invocation selected legacy adapter definitions already present
in modern A and failed compilation. The evidence-only harness then omitted only
the duplicate adapters, retaining every discovery assertion; the rerun passed.
An initial direct zero-handle compile omitted its source include directory;
adding `-Ifirmware/src/ble` corrected that invocation. Neither was a product fix.

## Remaining acceptance

Independent correctness disposition, exact native app/controller/package closure,
remote source delivery and owner hardware testing are separate stages. No firmware
binary, SDK/toolchain, device operation, merge or publication is part of this patch.
The earlier R5 controls/query acceptance is not transport acceptance.

## Public-source projection

This narrow public patch is based on existing dual-peer commit
`c647f98d81318ffb4084611683a8231ea0609b6d` (tree
`87a5057e8b1b3e6a94166ba6c90394143fe2c71f`), not on single-peer nightly.
Its production delta is the same parser correction tested on B above. It does
not import B's ancestor chain, later UI/assets, or its handle-zero repair.
Those later changes and the native B successor are separate integration inputs.
The new credit test and existing dual-peer suite are rerun on this public-base
projection; the B handle-zero results above belong to B, not this older base.
