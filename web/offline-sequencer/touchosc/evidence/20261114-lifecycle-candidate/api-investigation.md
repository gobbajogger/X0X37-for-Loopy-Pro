# Supported TouchOSC lifecycle API investigation

Read current official Hexler manual on 2026-11-14 before source edits:
- https://hexler.net/touchosc/manual/script-objects-control — documented callbacks include init/update/value/pointer/MIDI/OSC/gamepad/notify. init runs on editor→control-surface transitions, and may also run on some foreground/network-editor changes. No stop/session-end callback is documented. values are script-writable; changing values invokes callbacks, so disarm clears armed/debt before resetting x.
- https://hexler.net/touchosc/manual/script-properties-and-values — tag is a native user-editable string property; x is a button float value, touch reports associated pointers.
- https://hexler.net/touchosc/manual/editor-script — script execution is tied to control-surface mode (independent design reviewer checked).
- https://hexler.net/touchosc/releases — official 1.5.4 build269 Linux archive and checksum links. Inherited official archive pin matches qualification input; no substitute binary used.

Design review agreed on return-only recovery pending native proof. D1 cannot be guaranteed safe during editor mode with the documented callbacks. Require RELEASE before editing/saving/closing, and receiver recovery if already stuck. The safeguard preserves an outstanding local ON's original channel in reserved note tags and releases it at next init; native persistence and init-send remain unverified in this stage. It retires debt on the first existing OFF send attempt; it never retries an attempted OFF. No invented callbacks or transport replay.
