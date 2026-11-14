# Retained initial Chromium failure

Command: `python3 browser.test.py`, cwd `/workspace/smk37-browser-contract`.
Actual exit: 1. The first real Chromium run reached the pointer cancellation assertion and failed:

```
AssertionError: pointer cancellation releases note
```

The fixture originally grouped pointercancel with noteOff(60). It left other locally tracked notes and pedal/bend/transport obligations active and did not disarm. Independent source review separately identified the same contract mismatch. The repaired fixture calls full disarm on pointercancel; normal pointerup/lost capture releases only the local C4 gesture. The broader acceptance script now also requires the disarmed state after cancellation. No physical MIDI, permissions or device operation occurred.
