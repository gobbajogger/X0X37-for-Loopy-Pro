#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Hardware-free retry timeline: preserve replies/gaps and send only ED_DIAG.

Run: python3 tests/smc_connection_watch_test.py
The fixture simulates scan/create/cancel/retry, discovery/subscription, another
retry, and connection. It verifies recorder behavior, not a physical SMC link.
"""
import io
import json
from pathlib import Path
import sys
import tempfile
import types
from unittest.mock import patch

sys.modules["mido"] = types.ModuleType("mido")
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import smc_connection_watch as watch


def reply(state, step, waiting):
    fields = watch.ble.CRASH + watch.ble.BOOT + watch.ble.GUARD + watch.ble.LIVE
    words = [0] * len(fields)
    values = {"plat_state": 5 if state == 6 else 4, "h_state": state,
              "h_step": 0x80050000 | waiting << 8 | step, "mb_state_now": 2,
              "polls": 100, "rx0": 0x04030201}
    for key, value in values.items():
        words[fields.index(key)] = value
    # A retained ATT opcode must not be promoted to this attempt's result.
    words += [0x534D4301, 113 | 114 << 16, 0, 0, 0x13 << 24,
              (1 if state == 5 else 0) << 16]
    wire = [len(words)]
    for word in words:
        wire += [(word >> shift) & 127 for shift in (0, 7, 14, 21, 28)]
    return wire


progression = [(3, 6, 0), (4, 8, 1), None, (4, 9, 1), (3, 6, 0),
               (4, 8, 1), (5, 10, 0), (5, 11, 0), (5, 12, 0), (5, 13, 0),
               (7, 13, 1), (3, 6, 0), (4, 8, 1), (5, 10, 0), (5, 11, 0),
               (5, 12, 0), (5, 13, 0), (6, 14, 0)]
wire_replies = [reply(*entry) if entry else None for entry in progression]
calls = []
clock = [0.0]
duration = (len(progression) - 1) * 0.4 + 1.3 + 0.2


def ask(cmd, args=(), timeout=3.0, tries=2):
    assert cmd == watch.ble.ED_DIAG == 33, f"unexpected device command: {cmd}"
    assert not args and tries == 1
    assert 0 < timeout <= 1.0
    assert clock[0] + timeout + watch.REQUEST_OVERHEAD <= duration + 1e-9
    value = wire_replies[len(calls)]
    calls.append(cmd)
    clock[0] += 0.3 + (timeout if value is None else 0.1)
    return value


def sleep(seconds):
    clock[0] += seconds


with tempfile.TemporaryDirectory(prefix="smc-watch-test-") as tmp:
    output = Path(tmp) / "watch.jsonl"
    with patch.object(watch.ble, "ask", ask), \
         patch.object(watch.ble, "ports", return_value=("X0X USB input", "X0X USB output")), \
         patch.object(watch.ble, "t5w", side_effect=AssertionError("BT_TEST is forbidden")), \
         patch.object(watch.time, "monotonic", side_effect=lambda: clock[0]), \
         patch.object(watch.time, "sleep", side_effect=sleep), \
         patch.object(sys, "stdout", io.StringIO()):
        result = watch.main(["--seconds", str(duration), "--output", str(output)])
    rows = [json.loads(line) for line in output.read_text().splitlines()]

assert result == 0 and calls == [33] * len(progression)
metadata, finish = rows[0], rows[-1]
assert metadata["kind"] == "metadata" and metadata["installed_firmware_identity"] is None
assert len(metadata["tool_source_sha256"]) == len(metadata["diag_tool_source_sha256"]) == 64
assert "not installed firmware" in metadata["identity_note"]
assert "may span attempts" in metadata["retained_fields_note"]
assert metadata["interval"] == 0.25 and metadata["seconds"] == duration
samples = rows[1:-1]
assert len(samples) == len(progression)
for index, (sample, expected, raw) in enumerate(zip(samples, progression, wire_replies)):
    assert sample["kind"] == "sample" and sample["sequence"] == index + 1
    assert sample["request_started_utc"].endswith("Z") and sample["utc"].endswith("Z")
    assert sample["request_started_elapsed_seconds"] < sample["elapsed_seconds"] <= duration
    assert sample["raw_reply"] == raw
    if expected is None:
        assert sample["outcome"] == "no_reply" and "timeout" in sample["error"]
        assert sample["diag"] is sample["summary"] is sample["smc"] is None
    else:
        state, step, waiting = expected
        assert sample["outcome"] == "reply"
        assert sample["diag"]["h_state"] == state and sample["diag"]["h_step"] & 255 == step
        assert sample["smc"]["hci_command_waiting"] == waiting
        assert sample["smc"]["retained_last_att_opcode"] == 0x13
        assert "failure_reason" not in sample["smc"]
assert [samples[i]["smc"]["state_name"] for i in (0, 1, 4, 6, 10, 11, 17)] == [
    "SCANNING", "CONNECTING", "SCANNING", "DISCOVERING", "DISCONNECTING", "SCANNING", "CONNECTED"]
assert [samples[i]["smc"]["step_name"] for i in (3, 6, 7, 8, 9, 17)] == [
    "CANCEL_CONNECTION", "DISCOVER_MIDI_SERVICE", "DISCOVER_MIDI_CHARACTERISTIC",
    "DISCOVER_CCCD", "SUBSCRIBE_NOTIFICATIONS", "READY"]
assert samples[-1]["summary"]["status"] == "CONNECTED"
assert finish["kind"] == "finish" and finish["reason"] == "duration_complete"
assert finish["attempts"] == len(progression) and finish["replies"] == len(progression) - 1
assert finish["no_replies"] == 1 and finish["read_errors"] == finish["invalid_replies"] == 0
assert finish["elapsed_seconds"] == round(duration, 6)
print("PASS: DIAG-only retry timeline, transient timeout, raw/decoded JSONL, timestamps, source identity, bounded finish")
