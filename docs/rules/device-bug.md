# Rules v3: device bug

Read when: after a device bug report or a failed device test.

### Rule 8 [v2: 11, sentences 2 to 6, in time order]
Step: P1 record bug, P2 find cause.
Rule owner: Firmware Main Builder.

For a device bug, the owner first shows the bug once with INLOG on. The builder then makes a host replay test from that log. Make a fix only when data and a host test prove the cause. If a fix fails on the device, do not guess again: name the wrong assumption and get the missing data. If 2 device tests fail one after the other, go back to last-good-hw and bisect.

To log a device result, see docs.md (rule 12).

## Owner facts for a fix

- Optimize without cutting features. Font size is not a feature. (owner, 2026-10-16 18:41)
