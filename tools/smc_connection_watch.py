#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Record read-only SMC connection diagnostics over the existing X0X USB MIDI port.

Only ED_DIAG (33) is sent. No reset, Bluetooth toggle, record clearing, pairing,
discovery on the Mac, or control movement is needed. Firmware is not changed.
The JSONL records contain raw reply bytes, decoded words, state/step names,
request start/end timestamps, missing replies, and a final capture record.

Usage: python3 tools/smc_connection_watch.py --output smc-connection.jsonl
Needs mido with python-rtmidi. Existing output files are not overwritten.
Polling is best effort: ble_autotest.ask opens USB ports and waits 0.3 seconds
before each request, so a 0.25-second interval is not a 4 Hz sample guarantee.
The duration budget reserves this delay and caps each reply timeout; backend
port open/close delays can still extend the final request. Snapshots can miss
fast transitions and do not identify the reason for a connection failure.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import subprocess
import sys
import time

import ble_autotest as ble


# firmware/src/ble/ble_midi_central.{h,c}; the source revision is recorded below.
CENTRAL_STATES = ("OFF", "STOPPING", "STARTING", "SCANNING", "CONNECTING",
                  "DISCOVERING", "CONNECTED", "DISCONNECTING", "FAILED")
CENTRAL_STEPS = ("RESET", "SET_EVENT_MASK", "SET_LE_EVENT_MASK", "READ_BUFFER_SIZE",
                 "SET_RANDOM_ADDRESS", "SET_SCAN_PARAMETERS", "SCAN_ON", "SCAN_OFF",
                 "CREATE_CONNECTION", "CANCEL_CONNECTION", "DISCOVER_MIDI_SERVICE",
                 "DISCOVER_MIDI_CHARACTERISTIC", "DISCOVER_CCCD",
                 "SUBSCRIBE_NOTIFICATIONS", "READY")
REQUEST_OVERHEAD = 0.32  # ask()'s 0.3-second settle plus polling/scheduling margin
REPLY_TIMEOUT = 1.0


def utc_now():
    return datetime.now(timezone.utc).isoformat(timespec="milliseconds").replace("+00:00", "Z")


def source_metadata():
    try:
        commit = subprocess.check_output(
            ["git", "rev-parse", "HEAD"], cwd=Path(__file__).resolve().parents[1],
            text=True, stderr=subprocess.DEVNULL, timeout=2).strip()
    except (OSError, subprocess.SubprocessError):
        commit = None
    return {
        "source_checkout_commit": commit,
        "tool_source_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "diag_tool_source_sha256": hashlib.sha256(Path(ble.__file__).read_bytes()).hexdigest(),
        "installed_firmware_identity": None,
        "identity_note": "Source checkout and tool hashes identify this decoder, not installed firmware.",
        "retained_fields_note": "Last ATT opcode, reject details, and receive counters may span attempts; they are not a current-attempt failure reason.",
    }


def central_state(d):
    packed = d["h_step"]
    if not packed & 0x80000000:
        return {"central_marker": False, "state_name": None, "step_name": None}
    state, step = d["h_state"], packed & 255
    out = {
        "central_marker": True, "state": state, "step": step,
        "state_name": CENTRAL_STATES[state] if 0 <= state < len(CENTRAL_STATES) else f"UNKNOWN_{state}",
        "step_name": CENTRAL_STEPS[step] if step < len(CENTRAL_STEPS) else f"UNKNOWN_{step}",
        "hci_command_waiting": (packed >> 8) & 255,
        "acl_credits": (packed >> 16) & 255,
    }
    if d.get("smc_trace") == 0x534D4301:
        out.update(att_response_pending=d["smc_att"] >> 16,
                   midi_value_handle=d["smc_handles"] & 65535,
                   cccd_handle=d["smc_handles"] >> 16,
                   retained_last_att_opcode=d["smc_reject"] >> 24)
    return out


def record(output, seconds, interval, metadata):
    start = time.monotonic()
    deadline = start + seconds
    counts = {"attempts": 0, "replies": 0, "no_replies": 0, "read_errors": 0, "invalid_replies": 0}
    baseline = None

    def write(row):
        row.update(utc=utc_now(), elapsed_seconds=round(time.monotonic() - start, 6))
        output.write(json.dumps(row, sort_keys=True) + "\n")
        output.flush()

    write({"kind": "metadata", "format": "smc_connection_watch_v1", **metadata,
           "seconds": seconds, "interval": interval, "command": ble.ED_DIAG,
           "reply_timeout_max": REPLY_TIMEOUT, "request_overhead_budget": REQUEST_OVERHEAD})
    reason = "duration_complete"
    try:
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= REQUEST_OVERHEAD:
                if remaining > 0:
                    time.sleep(remaining)
                break
            requested = time.monotonic()
            timeout = min(REPLY_TIMEOUT, remaining - REQUEST_OVERHEAD)
            counts["attempts"] += 1
            row = {"kind": "sample", "sequence": counts["attempts"],
                   "request_started_utc": utc_now(),
                   "request_started_elapsed_seconds": round(requested - start, 6),
                   "reply_timeout": timeout, "raw_reply": None, "diag": None,
                   "summary": None, "smc": None}
            try:
                row["raw_reply"] = ble.ask(ble.ED_DIAG, timeout=timeout, tries=1)
            except Exception as exc:
                row.update(outcome="read_error", error=f"{type(exc).__name__}: {exc}")
                counts["read_errors"] += 1
            else:
                if row["raw_reply"] is None:
                    row.update(outcome="no_reply", error="DIAG reply timeout or USB port unavailable")
                    counts["no_replies"] += 1
                else:
                    try:
                        d = ble.decode_diag(row["raw_reply"])
                        if not d:
                            raise ValueError("empty DIAG reply")
                        row["diag"] = d
                        row["smc"] = central_state(d)
                        row["summary"] = ble.summary(d, baseline if baseline is not None else d)
                    except (IndexError, KeyError, TypeError, ValueError, OverflowError) as exc:
                        row.update(outcome="invalid_reply", error=f"{type(exc).__name__}: {exc}")
                        counts["invalid_replies"] += 1
                    else:
                        baseline = d if baseline is None else baseline
                        row["outcome"] = "reply"
                        counts["replies"] += 1
            write(row)
            delay = min(requested + interval, deadline) - time.monotonic()
            if delay > 0:
                time.sleep(delay)
    except KeyboardInterrupt:
        reason = "interrupted"
    finally:
        write({"kind": "finish", "reason": reason, **counts})
    return 130 if reason == "interrupted" else (0 if counts["replies"] else 1)


def bounded_seconds(maximum):
    def parse(value):
        number = float(value)
        if not math.isfinite(number) or not 0 < number <= maximum:
            raise argparse.ArgumentTypeError(f"must be finite and greater than 0, at most {maximum} seconds")
        return number
    return parse


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--seconds", type=bounded_seconds(300), default=45.0,
                        help="capture duration, at most 300 seconds (default 45)")
    parser.add_argument("--interval", type=bounded_seconds(60), default=0.25,
                        help="minimum request-start interval; USB overhead may exceed it (default 0.25)")
    parser.add_argument("--output", type=Path, required=True, help="new JSONL output file")
    args = parser.parse_args(argv)
    try:
        ports = ble.ports()
        if not ports:
            print("smc_connection_watch: no X0X USB MIDI input/output port", file=sys.stderr)
            return 1
        metadata = {**source_metadata(), "usb_input": ports[0], "usb_output": ports[1]}
        with args.output.open("x", encoding="utf-8") as output:
            result = record(output, args.seconds, args.interval, metadata)
    except (OSError, RuntimeError) as exc:
        print(f"smc_connection_watch: {exc}", file=sys.stderr)
        return 1
    print(f"smc_connection_watch: capture saved to {args.output}")
    return result


if __name__ == "__main__":
    sys.exit(main())
