#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Record read-only SMC connection diagnostics over the existing X0X USB MIDI port.

Only ED_DIAG (33) is sent. No reset, Bluetooth toggle, record clearing, pairing,
discovery on the Mac, or control movement is needed. Firmware is not changed.
The JSONL records contain raw reply bytes, decoded words, state/step names,
request start/end timestamps, missing replies, and a final capture record.
A sibling .summary.txt report is written automatically. On trace-capable
firmware it includes sticky first/latest failure evidence and recent events.

Usage: python3 tools/smc_connection_watch.py --output smc-connection.jsonl
Needs mido with python-rtmidi. Existing output files are not overwritten.
--port selects a case-insensitive part of the USB MIDI port name. Exactly one
input/output pair must match; that pair stays pinned throughout collection.
Polling is best effort: ble_autotest.ask opens USB ports and waits 0.3 seconds
before each request, so a 0.25-second interval is not a 4 Hz sample guarantee.
The duration budget reserves this delay and caps each reply timeout; backend
port open/close delays can still extend the final request. Snapshots can miss
fast transitions. Firmware trace records preserve failures between snapshots;
controller timeouts still do not identify the underlying radio/peer cause.
Exit 0 means diagnostic collection succeeded, not that Bluetooth passed.
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
CENTRAL_STATES, CENTRAL_STEPS = ble.SMC_STATES, ble.SMC_STEPS
REQUEST_OVERHEAD = 0.32  # ask()'s 0.3-second settle plus polling/scheduling margin
REPLY_TIMEOUT = 1.0


def utc_now():
    return datetime.now(timezone.utc).isoformat(timespec="milliseconds").replace("+00:00", "Z")


def select_ports(selector=None):
    keys = (selector.casefold(),) if selector else tuple(key.casefold() for key in ble.PORT_KEYS)
    inputs = list(dict.fromkeys(name for name in ble.mido.get_input_names()
                               if any(key in name.casefold() for key in keys)))
    outputs = list(dict.fromkeys(name for name in ble.mido.get_output_names()
                                if any(key in name.casefold() for key in keys)))
    scope = f"matching {selector!r}" if selector else "matching the X0X/SMK USB names"
    if not inputs or not outputs:
        raise RuntimeError(f"No complete USB MIDI input/output pair {scope}; "
                           f"inputs={inputs!r}, outputs={outputs!r}.")
    if len(inputs) != 1 or len(outputs) != 1:
        raise RuntimeError(f"Ambiguous USB MIDI ports {scope}; inputs={inputs!r}, outputs={outputs!r}. "
                           "Use --port with the name of the installed unit.")
    return inputs[0], outputs[0]


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
    if "smc_connection_trace" in d:
        out["connection_trace"] = d["smc_connection_trace"]
    return out


def u32(value):
    return type(value) is int and 0 <= value <= 0xFFFFFFFF


def input_counters(d):
    """Extract SMC receive counters; the SMC1 ACL words are *rejection bytes*, not counters."""
    if (not isinstance(d, dict) or d.get("smc_trace") != 0x534D4301
            or not all(u32(d.get(key)) for key in ("h_step", "rx0", "rx1"))
            or not d["h_step"] & 0x80000000):
        return None
    transport, consumer = d["rx0"], d["rx1"]
    return {
        "acl": transport & 255,
        "notifications": transport >> 8 & 255,
        "midi": transport >> 16 & 255,
        "rejected": transport >> 24 & 255,
        "enqueued": consumer & 255,
        "dropped": consumer >> 8 & 255,
        "dispatched": consumer >> 16 & 255,
        "last_dispatch_reason": consumer >> 24 & 255,
        "last_receive_reason": d["h_step"] >> 24 & 127,
    }


def counter_continuity(before, after):
    """Return an uncertainty reason, or None for a bounded live-uptime interval.

    reset_now contains reset *causes*, not a boot ID. alive_ms is a uint32
    last-tick uptime retained across resets (firmware/src/ble.c). Require an
    active connection and advancing uptime, plus monotonic request windows.
    UTC windows must agree: macOS monotonic time can stop during host sleep.
    Wall time can veto continuity, but never establishes it by itself.
    The later uptime must exceed the entire host interval, with a conservative
    2x clock allowance and 2 ms rounding margin, so a boot within that interval
    cannot explain it. Do not infer uptime wrap or continuity from causes alone.
    """
    if not isinstance(before, dict) or not isinstance(after, dict):
        return "missing_sample_timing"
    first, last = before.get("diag"), after.get("diag")
    if not isinstance(first, dict) or not isinstance(last, dict):
        return "missing_sample_timing"
    times = [row.get(key) for row in (before, after)
             for key in ("request_started_elapsed_seconds", "elapsed_seconds")]
    if not all(type(t) in (int, float) and 0 <= t < math.inf for t in times):
        return "missing_sample_timing"
    try:
        times = [float(t) for t in times]
        wall_times = [datetime.fromisoformat(row[key].replace("Z", "+00:00"))
                      for row in (before, after) for key in ("request_started_utc", "utc")]
        if any(t.utcoffset() is None for t in wall_times):
            return "missing_sample_timing"
        wall_elapsed = [(t - wall_times[0]).total_seconds() for t in wall_times]
    except (KeyError, TypeError, ValueError, OverflowError, AttributeError):
        return "missing_sample_timing"
    # UTC is stored to milliseconds. Allow only rounding/scheduling noise, not
    # suspend or a clock adjustment. Never use wall time alone as a boot ID.
    if (wall_elapsed != sorted(wall_elapsed)
            or any(abs(wall - (mono - times[0])) > 0.01
                   for wall, mono in zip(wall_elapsed, times))):
        return "host_clock_discontinuity"
    start, end, next_start, next_end = times
    if not start <= end <= next_start <= next_end or next_end <= start:
        return "invalid_sample_timing"
    if any(d.get("h_state") != 6 or d.get("plat_state") != 5
           or d.get("blocked", 0) or d.get("c1_gave_up", 0) for d in (first, last)):
        return "not_live_connected"
    if not all(u32(d.get(key)) for d in (first, last) for key in ("alive_ms", "reset_now")):
        return "missing_uptime_or_reset_cause"
    if first["reset_now"] != last["reset_now"]:
        return "reset_cause_changed"
    advance = last["alive_ms"] - first["alive_ms"]
    elapsed_ms = max(next_end - start, wall_elapsed[-1]) * 2000
    if not 0 <= elapsed_ms < math.inf:
        return "invalid_sample_timing"
    bound_ms = math.ceil(elapsed_ms) + 2
    if advance <= 0 or advance > bound_ms or last["alive_ms"] <= bound_ms:
        return "uptime_continuity_unproven"
    return None


def input_activity(snapshots):
    """Sum minimum modulo-256 increments only over comparable sample intervals.

    Pass original JSONL sample rows to retain monotonic timing and missing reads.
    Decoded-only histories remain usable for snapshots, but not interval totals.
    Multiple counter wraps are unobservable. Excluded intervals are indeterminate,
    not zero events or a count of boots.
    """
    keys = ("acl", "notifications", "midi", "rejected", "enqueued", "dropped", "dispatched")
    totals = dict.fromkeys(keys, 0)
    previous = None
    valid = compared = unavailable = 0
    excluded = {}
    for row in snapshots:
        is_sample = isinstance(row, dict) and "diag" in row
        d = row.get("diag") if is_sample else row
        current = input_counters(d)
        if current is None or (is_sample and row.get("outcome") != "reply"):
            unavailable += 1
            previous = None
            continue
        valid += 1
        if previous is not None:
            prior_row, prior = previous
            uncertainty = counter_continuity(prior_row, row)
            if uncertainty:
                excluded[uncertainty] = excluded.get(uncertainty, 0) + 1
            else:
                compared += 1
                for key in keys:
                    totals[key] += (current[key] - prior[key]) & 255
        previous = (row, current)
    return {"samples": valid, "minimum_increments": totals if compared else None,
            "compared_intervals": compared, "indeterminate_intervals": sum(excluded.values()),
            "excluded_reasons": excluded, "unavailable_samples": unavailable} if valid else None


def rejected_smp(d):
    """Return the retained rejected SMP *header*. The two-byte payload was not saved."""
    if (not isinstance(d, dict) or d.get("smc_trace") != 0x534D4301
            or not all(u32(d.get(key)) for key in ("smc_reject", "smc_acl0", "smc_acl1"))):
        return None
    if (d["smc_reject"] >> 16 & 255) != 5 or (d["smc_reject"] & 65535) < 8:
        return None  # BLEC_RX_CID
    head = d["smc_acl0"].to_bytes(4, "little") + d["smc_acl1"].to_bytes(4, "little")
    if int.from_bytes(head[6:8], "little") != 6:
        return None
    return {"acl_length": int.from_bytes(head[2:4], "little"),
            "smp_length": int.from_bytes(head[4:6], "little")}


def human_report(last, counts, observed_connected, reason, input_history=None, controls_operated=None):
    lines = ["SMC CONNECTION TEST REPORT", ""]
    controller_lines = []
    if last is not None:
        platform = last.get("plat_state", -1)
        controller_lines += ["Bluetooth controller/platform: " +
                             (ble.STATE[platform] if 0 <= platform < len(ble.STATE) else f"UNKNOWN_{platform}") + "."]
        mailbox, error = last.get("mb_state_now", -1), last.get("mb_err_now", -1)
        if error > 0 or platform in (0, 6, 7):
            controller_lines += [f"Controller mailbox: {ble.MB[mailbox] if 0 <= mailbox < len(ble.MB) else mailbox}; "
                                 f"reported error: {ble.ERR[error] if 0 <= error < len(ble.ERR) else error}."]
        if last.get("blocked"):
            why = last.get("why", -1)
            controller_lines += ["Bluetooth boot guard is blocking startup: " +
                                 (ble.WHY[why] if 0 <= why < len(ble.WHY) else f"reason {why}") + "."]
        if last.get("c1_gave_up"):
            core_reason = last["c1_gave_up"]
            core_reasons = {1: "unexpected job", 2: "job did not complete",
                            3: "core did not park", 4: "Bluetooth startup did not finish"}
            controller_lines += [f"Core 1 reported: {core_reasons.get(core_reason, f'unknown reason {core_reason}')}."]
    if last is None:
        lines += ["No valid diagnostic replies were received. Connection behavior could not be assessed."]
    elif not last["h_step"] & 0x80000000:
        lines += ["The device did not report SMC central mode. This capture does not establish an SMC connection result."]
    else:
        current = central_state(last)
        state = current["state"]
        connected = state == 6 and last.get("plat_state") == 5
        if connected:
            lines += ["SMC connection established: the latest firmware snapshot is CONNECTED.",
                      "This confirms connection setup; it does not by itself verify control mapping or long-term stability."]
        elif observed_connected:
            lines += ["SMC reached CONNECTED, but the latest snapshot is no longer connected."]
        elif state == 6:
            lines += ["SMC connection is not confirmed: central state is CONNECTED, but the controller/platform does not report a current connection."]
        else:
            lines += ["SMC did not reach CONNECTED in the captured snapshots."]
        lines += [f"Latest phase: {current['state_name']} / {current['step_name']}."]
        trace = last.get("smc_connection_trace")
        if trace:
            lines += [f"Since host initialization: {trace['attempts']} connection attempts, "
                      f"{trace['ready_count']} completed subscriptions, {trace['failure_count']} recorded failures, "
                      f"{trace['disconnect_count']} disconnections."]
            if trace["ready_count"] and not observed_connected:
                lines += ["The firmware records a completed subscription outside the sampled CONNECTED states; it may predate this watch."]
            if trace["latest_failure"]:
                label = "Retained earlier failure (current connection has recovered)" if connected else "Latest recorded failure"
                lines += [label + ": " + ble.smc_failure_text(trace["latest_failure"]),
                          "First recorded failure: " + ble.smc_failure_text(trace["first_failure"])]
            else:
                lines += ["No failure was recorded by the firmware since host initialization."]
            if "last_smp" in trace:
                lines += [ble.smc_smp_history_text(trace)]
            if trace["recent_events"]:
                lines += ["Recent firmware events (oldest first):"]
                for event in trace["recent_events"]:
                    lines += [f"  {event['ms']} ms: {event['kind_name']} at {event['state_name']}/{event['step_name']}; "
                              f"status 0x{event['status']:02X}, opcode 0x{event['opcode']:04X}, "
                              f"handle 0x{event['handle']:04X}, detail 0x{event['detail']:08X}"]
        else:
            lines += ["This firmware has no sticky connection-failure trace. Retained ATT/rejection fields alone do not identify the failed attempt."]
    lines += controller_lines
    counters = input_counters(last)
    if counters is not None:
        lines += ["SMC receive counters (each modulo 256 since host initialization): "
                  f"ACL {counters['acl']}, notifications {counters['notifications']}, decoded MIDI {counters['midi']}, "
                  f"application enqueued {counters['enqueued']}, dispatched {counters['dispatched']}, "
                  f"dropped {counters['dropped']}, rejected ACL {counters['rejected']}."]
        activity = input_activity(input_history) if input_history is not None else None
        if activity:
            if activity["compared_intervals"]:
                changes = activity["minimum_increments"]
                lines += [f"Observed counter increments over {activity['compared_intervals']} comparable intervals "
                          "(minimum modulo-256, not an exact event count): "
                          f"notifications {changes['notifications']}, decoded MIDI {changes['midi']}, "
                          f"application dispatched {changes['dispatched']}, "
                          f"queue dropped {changes['dropped']}, rejected ACL {changes['rejected']}."]
                if not changes["notifications"] and not changes["midi"] and not changes["dispatched"]:
                    lines += ["No receive/dispatch counter changes were observed in comparable samples. "
                              "Complete modulo-256 wraps between reads cannot be excluded."]
            else:
                lines += ["Input counter increments are indeterminate: no comparable sample interval."]
            if activity["indeterminate_intervals"] or activity["unavailable_samples"]:
                lines += [f"Excluded intervals: {activity['indeterminate_intervals']}; "
                          f"unavailable samples: {activity['unavailable_samples']}. "
                          "Their input activity is indeterminate, not zero. Reset-cause bits are not a boot ID."]
        if controls_operated is True:
            lines += ["The owner confirmed fader/button gestures during this capture; flat counters "
                      "must not be described as an idle capture. The missing-input cause remains unconfirmed."]
        else:
            lines += ["Diagnostic counters do not record physical gestures; keep any owner-reported "
                      "control activity separate from these observations."]
        smp = rejected_smp(last)
        if smp is not None:
            lines += ["Retained rejected ACL frame: Bluetooth Security Manager (L2CAP CID 0x0006), "
                      f"{smp['smp_length']}-byte payload. Its SMP command bytes were not captured; "
                      "this is not a MIDI notification and may predate this watch."]
    lines += ["", f"Collection ended: {reason}. Diagnostic replies: {counts['replies']}; "
              f"missing replies: {counts['no_replies']}; read errors: {counts['read_errors']}; "
              f"invalid replies: {counts['invalid_replies']}.",
              "Source checkout metadata identifies the collection tools, not the installed firmware.",
              "Full raw replies and decoded values are retained in the companion JSONL file."]
    return "\n".join(lines) + "\n"


def record(output, seconds, interval, metadata, report_output=None, port_pair=None):
    start = time.monotonic()
    deadline = start + seconds
    counts = {"attempts": 0, "replies": 0, "no_replies": 0, "read_errors": 0, "invalid_replies": 0}
    baseline = None
    last = None
    input_history = []
    observed_connected = False

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
                row["raw_reply"] = ble.ask(ble.ED_DIAG, timeout=timeout, tries=1, port_pair=port_pair)
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
                        last = d
                        observed_connected |= bool(d["h_step"] & 0x80000000 and d["h_state"] == 6 and d.get("plat_state") == 5)
                        row["outcome"] = "reply"
                        counts["replies"] += 1
            write(row)
            input_history.append(row)
            delay = min(requested + interval, deadline) - time.monotonic()
            if delay > 0:
                time.sleep(delay)
    except KeyboardInterrupt:
        reason = "interrupted"
    finally:
        report = human_report(last, counts, observed_connected, reason, input_history,
                              controls_operated=metadata.get("controls_operated"))
        write({"kind": "finish", "reason": reason, "human_summary": report, **counts})
        if report_output is not None:
            report_output.write(report)
            report_output.flush()
    return 130 if reason == "interrupted" else (0 if counts["replies"] else 1)


def failed_start(output, report_output, metadata, seconds, interval, error):
    counts = {"attempts": 0, "replies": 0, "no_replies": 0, "read_errors": 0, "invalid_replies": 0}
    report = human_report(None, counts, False, "port_selection_failed")
    report = report.replace("\n\n", "\n\nAutomatic diagnostic collection could not start: " + error + "\n", 1)
    rows = [{"kind": "metadata", "format": "smc_connection_watch_v1", **metadata,
             "seconds": seconds, "interval": interval, "command": ble.ED_DIAG},
            {"kind": "finish", "reason": "port_selection_failed", "error": error,
             "human_summary": report, **counts}]
    for row in rows:
        row.update(utc=utc_now(), elapsed_seconds=0.0)
        output.write(json.dumps(row, sort_keys=True) + "\n")
    output.flush()
    report_output.write(report)
    report_output.flush()
    return 1


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
    parser.add_argument("--port", metavar="NAME", help="USB MIDI port to use (case-insensitive part of its name)")
    parser.add_argument("--output", type=Path, required=True, help="new JSONL output file")
    args = parser.parse_args(argv)
    try:
        report_path = args.output.with_name(args.output.name + ".summary.txt")
        if report_path.exists():
            raise FileExistsError(f"report already exists: {report_path}")
        with args.output.open("x", encoding="utf-8") as output, report_path.open("x", encoding="utf-8") as report_output:
            metadata = {**source_metadata(), "usb_input": None, "usb_output": None, "port_selector": args.port}
            try:
                ports = select_ports(args.port)
            except (OSError, RuntimeError) as exc:
                result = failed_start(output, report_output, metadata, args.seconds, args.interval, str(exc))
            else:
                metadata.update(usb_input=ports[0], usb_output=ports[1])
                result = record(output, args.seconds, args.interval, metadata, report_output, port_pair=ports)
    except (OSError, RuntimeError) as exc:
        print(f"smc_connection_watch: {exc}", file=sys.stderr)
        return 1
    print(f"smc_connection_watch: capture saved to {args.output}")
    print(f"smc_connection_watch: report saved to {report_path}")
    print(report_path.read_text(encoding="utf-8"), end="")
    return result


if __name__ == "__main__":
    sys.exit(main())
