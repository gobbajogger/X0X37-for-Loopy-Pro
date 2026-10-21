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


def reply(state, step, waiting, trace=None):
    fields = watch.ble.CRASH + watch.ble.BOOT + watch.ble.GUARD + watch.ble.LIVE
    words = [0] * len(fields)
    values = {"plat_state": 5 if state == 6 else 4, "h_state": state,
              "h_step": 0x80050000 | waiting << 8 | step, "mb_state_now": 2,
              "polls": 100, "rx0": 0x04030201}
    for key, value in values.items():
        words[fields.index(key)] = value
    if trace:
        boundary = len(watch.ble.CRASH) + len(watch.ble.BOOT) + len(watch.ble.GUARD)
        words[boundary:boundary] = trace
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


def ask(cmd, args=(), timeout=3.0, tries=2, port_pair=None):
    assert cmd == watch.ble.ED_DIAG == 33, f"unexpected device command: {cmd}"
    assert not args and tries == 1
    assert port_pair == ("X0X USB input", "X0X USB output")
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
         patch.object(watch, "select_ports", return_value=("X0X USB input", "X0X USB output")), \
         patch.object(watch.ble, "t5w", side_effect=AssertionError("BT_TEST is forbidden")), \
         patch.object(watch.time, "monotonic", side_effect=lambda: clock[0]), \
         patch.object(watch.time, "sleep", side_effect=sleep), \
         patch.object(sys, "stdout", io.StringIO()):
        result = watch.main(["--seconds", str(duration), "--output", str(output)])
    rows = [json.loads(line) for line in output.read_text().splitlines()]
    report = output.with_name(output.name + '.summary.txt').read_text()

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
assert report == finish['human_summary']
assert 'SMC connection established: the latest firmware snapshot is CONNECTED.' in report
assert 'no sticky connection-failure trace' in report
print("PASS: DIAG-only retry timeline, transient timeout, raw/decoded JSONL, timestamps, source identity, bounded finish")

# Firmware records survive missed polls: capture only a later ATT failure, while
# still obtaining the original controller establishment failure from the device.
first = [100, 2 | 4 << 8 | 8 << 16 | 0x3e << 24, 0x200d | 0xffff << 16, 0]
latest = [200, 4 | 5 << 8 | 13 << 16 | 3 << 24, 0x12 | 0x73 << 16, 0x50]
trace = [watch.ble.SMC_TRACE_MAGIC, 5, 2, 2, 2 | 2 << 8] + first + latest + first + latest + [0] * 4 + [26]
assert len(trace) == 26
clock[0] = 0


def traced_ask(cmd, args=(), timeout=3.0, tries=2, port_pair=None):
    assert cmd == 33 and not args and tries == 1
    assert port_pair == ('X0X in', 'X0X out')
    clock[0] += 0.4
    return reply(5, 13, 0, trace)


with tempfile.TemporaryDirectory(prefix='smc-watch-trace-') as tmp:
    output = Path(tmp) / 'failure.jsonl'
    with patch.object(watch.ble, 'ask', traced_ask), \
         patch.object(watch, 'select_ports', return_value=('X0X in', 'X0X out')), \
         patch.object(watch.time, 'monotonic', side_effect=lambda: clock[0]), \
         patch.object(watch.time, 'sleep', side_effect=sleep), \
         patch.object(sys, 'stdout', io.StringIO()):
        assert watch.main(['--seconds', '0.6', '--output', str(output)]) == 0
    rows = [json.loads(line) for line in output.read_text().splitlines()]
    report = output.with_name(output.name + '.summary.txt').read_text()
assert len(rows) == 3 and rows[1]['smc']['connection_trace']['failure_count'] == 2
assert 'SMC did not reach CONNECTED' in report
assert 'Latest recorded failure: Peer returned ATT error 0x03 (write not permitted)' in report
assert 'for request 0x12, attribute 0x0073' in report
assert 'First recorded failure: Connection attempt failed: 0x3E' in report
assert 'underlying radio/peer cause is not identified' in report
assert report == rows[-1]['human_summary']

# A completed current subscription must override a retained earlier failure in
# the headline, while preserving that earlier evidence and the source boundary.
recovered = watch.ble.decode_diag(reply(6, 14, 0, trace))
report = watch.human_report(recovered, rows[-1], True, 'duration_complete')
assert report.startswith('SMC CONNECTION TEST REPORT\n\nSMC connection established:')
assert 'Retained earlier failure (current connection has recovered)' in report
assert 'not the installed firmware' in report
assert watch.ble.summary(recovered, None)['smc_failure'].startswith('Currently CONNECTED; retained earlier failure:')
blocked = recovered.copy()
blocked.update(h_state=0, plat_state=6, mb_state_now=3, mb_err_now=1, blocked=1, why=1, c1_gave_up=2)
report = watch.human_report(blocked, rows[-1], False, 'duration_complete')
assert 'Bluetooth controller/platform: FAILED.' in report
assert 'Controller mailbox: FAILED; reported error: BOOT.' in report
assert 'Bluetooth boot guard is blocking startup: TIMEOUT.' in report
assert 'Core 1 reported: job did not complete.' in report
print('PASS: automatic human report, sticky first/latest cause across missed polls, ATT error meaning, recovered current connection, boot/controller failure, and tool/firmware identity separation')

# Retained .ble_ram can contain CONNECTED from an older boot. The platform's
# current state must agree before either the report or a later sample records
# a successful connection. Exercise the raw recorder as well as its formatter.
stale = recovered.copy()
stale.update(plat_state=6, blocked=1, why=4, mb_state_now=2, mb_err_now=0)
report = watch.human_report(stale, rows[-1], False, 'duration_complete')
assert 'SMC connection is not confirmed' in report
assert 'SMC connection established' not in report and 'current connection has recovered' not in report
assert 'Bluetooth boot guard is blocking startup: RESET.' in report
assert not watch.ble.summary(stale, None)['smc_failure'].startswith('Currently CONNECTED')
clock[0] = 0
stale_sequence = iter((stale, watch.ble.decode_diag(reply(3, 6, 0))))


def stale_ask(*args, **kwargs):
    clock[0] += 0.4
    return [1]


with patch.object(watch.ble, 'ask', stale_ask), \
     patch.object(watch.ble, 'decode_diag', side_effect=lambda raw: next(stale_sequence)), \
     patch.object(watch.time, 'monotonic', side_effect=lambda: clock[0]), \
     patch.object(watch.time, 'sleep', side_effect=sleep):
    output, summary = io.StringIO(), io.StringIO()
    assert watch.record(output, 0.9, 0.25, {}, summary, port_pair=('X0X in', 'X0X out')) == 0
assert 'SMC did not reach CONNECTED' in summary.getvalue()
assert 'SMC reached CONNECTED, but' not in summary.getvalue()
print('PASS: stale central CONNECTED with failed/blocked platform never records or reports connection success')

# Port names are enumerated once, selected unambiguously, and pinned for every
# request. A disappeared selected port must never fall back to another unit.
with patch.object(watch.ble.mido, 'get_input_names', return_value=['SMK A', 'smk B', 'smk B'], create=True), \
     patch.object(watch.ble.mido, 'get_output_names', return_value=['SMK A', 'smk B'], create=True):
    assert watch.select_ports('SMK b') == ('smk B', 'smk B')
    try:
        watch.select_ports()
        raise AssertionError('ambiguous ports accepted')
    except RuntimeError as exc:
        assert 'Ambiguous' in str(exc)


class FakePort:
    def __init__(self):
        self.drained = False
        self.sent = []

    def __enter__(self):
        return self

    def __exit__(self, *args):
        pass

    def iter_pending(self):
        if not self.drained:
            self.drained = True
            return []
        return [types.SimpleNamespace(type='sysex', data=watch.ble.HDR + [33, 9])]

    def send(self, message):
        self.sent.append(message)


input_port, output_port = FakePort(), FakePort()
with patch.object(watch.ble, 'ports', side_effect=AssertionError('must not reselect USB device')), \
     patch.object(watch.ble.mido, 'open_input', return_value=input_port, create=True) as open_input, \
     patch.object(watch.ble.mido, 'open_output', return_value=output_port, create=True) as open_output, \
     patch.object(watch.ble.mido, 'Message', side_effect=lambda kind, data: (kind, data), create=True), \
     patch.object(watch.ble.time, 'sleep'):
    assert watch.ble.ask(33, tries=1, port_pair=('smk B', 'smk B')) == [9]
    open_input.assert_called_once_with('smk B')
    open_output.assert_called_once_with('smk B')
assert output_port.sent == [('sysex', watch.ble.HDR + [33])]
with patch.object(watch.ble, 'ports', side_effect=AssertionError('must not fall back to another device')), \
     patch.object(watch.ble.mido, 'open_input', side_effect=OSError('selected port disappeared'), create=True):
    try:
        watch.ble.ask(33, tries=1, port_pair=('smk B', 'smk B'))
        raise AssertionError('disappeared pinned port accepted')
    except OSError as exc:
        assert 'disappeared' in str(exc)

# Both early selection failures leave complete, automatically printed evidence
# files, without issuing a DIAG request or opening any USB port.
for inputs, outputs, expected in (([], [], 'No complete USB MIDI'),
                                 (['SMK A', 'SMK B'], ['SMK A', 'SMK B'], 'Ambiguous USB MIDI')):
    with tempfile.TemporaryDirectory(prefix='smc-watch-no-port-') as tmp:
        output = Path(tmp) / 'failure.jsonl'
        with patch.object(watch.ble.mido, 'get_input_names', return_value=inputs, create=True), \
             patch.object(watch.ble.mido, 'get_output_names', return_value=outputs, create=True), \
             patch.object(watch.ble, 'ask', side_effect=AssertionError('must not query ambiguous ports')), \
             patch.object(sys, 'stdout', io.StringIO()) as printed:
            assert watch.main(['--port', 'smk', '--output', str(output)]) == 1
        failed_rows = [json.loads(line) for line in output.read_text().splitlines()]
        report = output.with_name(output.name + '.summary.txt').read_text()
    assert len(failed_rows) == 2 and failed_rows[-1]['reason'] == 'port_selection_failed'
    assert failed_rows[0]['usb_input'] is None and failed_rows[0]['port_selector'] == 'smk'
    assert failed_rows[-1]['attempts'] == 0 and failed_rows[-1]['human_summary'] == report
    assert expected in report and report in printed.getvalue()
print('PASS: explicit case-insensitive USB selection, pinned requests, no fallback, and saved early-failure reports')
