# Device bug

Read when: after a device bug report or a failed device test.

### Rule 8
Step: P1 record bug, P2 find cause. Rule owner: Firmware Main Builder.

Trust the reports and the setup of the owner. Search our code for the cause first.

For a device bug, the owner first shows the bug once with INLOG on. The owner can skip the INLOG step for one bug. The builder then makes a host replay test from that log.

Make a fix only when data and a host test prove the cause. If a fix fails on the device, do not guess again: name the wrong assumption and get the missing data. If 2 device tests fail one after the other, go back to last-good-hw and bisect. Log the result with docs.md.

Test target for each fix: X0X does not drop notes, pad colour updates or the Bluetooth connection (BEHAVIOR.md).

Owner fact for a fix: "Optimize without cutting features. Font size is not a feature." (2026-10-16 18:41)
