"""Hand-derived synthetic event tests, never firmware timing measurements."""
import copy
import csv
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

HZ = 5760000000
PERIOD = 66846720  # 512 * 544 * (5760000000 / 24000000)
FW = "94197fc839b00aff47b98ca02f3c6396e4a6eabf"
RUNNER = "31157d25caf7284080cdb5d86ab7e8e438120fd9"


def header():
    watches, address = {}, 0x01C00000
    for name, size in [("x0x_dbg", 52), ("audio_xruns", 4), ("eng_prof", 32),
            ("eng_prof_frames", 4), ("eng_job_cost", 20), ("eng_plan", 1),
            ("eng_c1_mask", 1), ("eng_stage", 4), ("eng_mastered", 4),
            ("eng_do_master", 1), ("eng_job_arg", 4), ("brk_silent_now", 4),
            ("usr_nz", 3), ("nloops", 4), ("pack_n", 4),
            ("kbuf", 1024), ("dry_l", 1024), ("dry_r", 1024)]:
        watches[name] = {"address": address, "bytes": size,
                         "element_bytes": 4 if size % 4 == 0 else 1}
        address += (size + 3) & ~3
    return dict(event="observer", schema=1, hz=HZ, clock_unit="emulator_tick",
                cycle_ticks=16, firmware_source=FW, runner_source=RUNNER,
                core1_time="global_slice_clock",
                eng_prof_scope="guest_counters_with_known_core1_omissions", watches=watches)


def snapshot(halves=0, frames=0, last_us=0, late=0, xruns=0, prof=None):
    return {"x0x_dbg": [0x44424731, halves, last_us, 0, 0, late, 0, 0, last_us, 0, 1, 0, 0],
            "audio_xruns": [xruns], "eng_prof": list(prof or [0] * 8),
            "eng_prof_frames": [frames], "eng_job_cost": [1, 2, 3, 4, 5],
            "eng_plan": [17], "eng_c1_mask": [16], "eng_stage": [4],
            "eng_mastered": [256], "eng_do_master": [1], "eng_job_arg": [256 << 20],
            "brk_silent_now": [0], "usr_nz": [1, 0, 0], "nloops": [1], "pack_n": [0]}


def dma(seq):
    return dict(event="dma", seq=seq, scheduled_cycle=(seq - 1) * PERIOD,
                observed_cycle=(seq - 1) * PERIOD, deadline_cycle=seq * PERIOD,
                half=(seq - 1) % 2, address=0x01C10000 + (seq - 1) % 2 * 4096, words=1024)


def rendered(seq, before, after, complete=None, dma_end=None, mix=True):
    entry = (seq - 1) * PERIOD + 100
    complete = seq * PERIOD - 5000 if complete is None else complete
    events = [dict(event="irq_enter", seq=seq, cycle=entry, depth=1, snapshot=before)]
    for i, value in enumerate(((before["eng_prof_frames"][0] + 256) & 0xFFFFFFFF,
                                after["eng_prof_frames"][0])):
        events.append(dict(event="store", seq=seq, cycle=entry + i + 1, core=0,
                           name="eng_prof_frames", offset=0, width=4, value=value))
    tick = entry + 3
    trailing = []
    for name, values in after.items():
        width = header()["watches"][name]["element_bytes"]
        if name == "eng_prof_frames":
            continue
        for index, value in enumerate(values):
            if value == before[name][index] or (name == "x0x_dbg" and index == 8):
                continue
            record = dict(event="store", seq=seq, cycle=tick,
                          core=1 if name == "eng_prof" and index == 0 else 0,
                          name=name, offset=index * width, width=width, value=value)
            if name == "audio_xruns" or (name == "x0x_dbg" and index == 5):
                record["cycle"] = complete - 1
                trailing.append(record)
            else:
                events.append(record)
                tick += 1
    events.append(dict(event="store", seq=seq, cycle=complete - 2, core=0,
                       name="x0x_dbg", offset=32, width=4, value=after["x0x_dbg"][8]))
    events.extend(trailing)
    events.append(dict(event="half", seq=seq, entry_cycle=entry,
                       completion_cycle=complete, completion_uncertainty_cycles=1,
                       dma_seq_at_completion=seq if dma_end is None else dma_end,
                       last_us_writes=1, last_us=after["x0x_dbg"][8], snapshot=after,
                       break_mix=dict(nonzero_writes=20 if mix else 0,
                                      peak_delta=0.125 if mix else 0.0, invalid=False)))
    return events


def regular_trace():
    a = snapshot()
    b = snapshot(1, 512, 10000, prof=[1, 2, 3, 4, 5, 6, 7, 8])
    c = snapshot(2, 1024, 10000, prof=[2, 4, 6, 8, 10, 12, 14, 16])
    return [header(), dma(1), *rendered(1, a, b), dma(2), *rendered(2, b, c), dma(3),
            dict(event="finish", cycle=HZ * 24 // 1000, completed=2, dma_edges=3, resets=0, pending_seq=None)]


def scene_trace():
    events = regular_trace()
    events[0]["watches"]["scene_flags"] = {"address": 0x01C20001, "bytes": 4, "element_bytes": 1}
    for event in events:
        if "snapshot" in event:
            half = event["snapshot"]["x0x_dbg"][1]
            event["snapshot"]["scene_flags"] = [0, 0, 0, 0] if half == 0 else [1, 2, 9, 4]
    # First a little-endian word, then an overlapping byte store. Metadata uses
    # byte arrays even though this optional field happens to contain four bytes.
    index = next(i for i, event in enumerate(events) if event["event"] == "store" and
                 event["seq"] == 1 and event["name"] == "x0x_dbg" and event["offset"] == 32)
    events[index:index] = [
        dict(event="store", seq=1, cycle=PERIOD - 5004, core=0, name="scene_flags", offset=0, width=4, value=0x04030201),
        dict(event="store", seq=1, cycle=PERIOD - 5003, core=0, name="scene_flags", offset=2, width=1, value=9),
    ]
    return events


class TraceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        path = Path(__file__).with_name("trace.py")
        cls.mod = None
        if path.exists():
            spec = importlib.util.spec_from_file_location("dropout_trace", path)
            cls.mod = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(cls.mod)

    def setUp(self):
        self.assertIsNotNone(self.mod, "trace reducer implementation is missing")

    def analyze(self, events, end_ms=24, tail=""):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp); path = root / "trace.jsonl"
            path.write_text("".join(json.dumps(e) + "\n" for e in events) + tail)
            result = self.mod.analyze_trace(path, 0, end_ms, out_dir=root / "result")
            with (root / "result" / "per_half.csv").open() as stream:
                rows = list(csv.DictReader(stream))
            stages = [json.loads(x) for x in (root / "result" / "stage_events.jsonl").read_text().splitlines()]
            return result, rows, stages

    def test_equal_last_us_values_remain_two_coherent_halves(self):
        result, rows, _ = self.analyze(regular_trace())
        self.assertEqual(result["status"], "OBSERVED")
        self.assertEqual([r["last_us"] for r in rows], ["10000", "10000"])
        self.assertEqual(result["window"]["completed_halves"], 2)
        self.assertEqual(result["deadline"]["unserviced_dma_edges"], 0)
        self.assertEqual(result["break_evidence"]["loaded"], "OBSERVED")
        self.assertEqual(result["break_evidence"]["mix_contribution"], "OBSERVED")
        self.assertEqual(result["break_evidence"]["audibility"], "UNVERIFIED")
        self.assertFalse(result["owner_scene_exact"])
        self.assertFalse(result["representative_scene_qualified"])

    def test_summary_binds_finished_outputs_and_detects_changed_artifact_bytes(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp); path = root / "trace.jsonl"
            path.write_text("".join(json.dumps(event) + "\n" for event in regular_trace()))
            result = self.mod.analyze_trace(path, 0, 24, out_dir=root / "result")
            persisted = json.loads((root / "result" / "summary.json").read_text())
            for filename, key in (("per_half.csv", "per_half_csv_sha256"),
                                  ("stage_events.jsonl", "stage_events_sha256")):
                artifact = root / "result" / filename
                original = artifact.read_bytes()
                self.assertTrue(original, "the binding must cover flushed data, not an empty buffer")
                self.assertEqual(result[key], hashlib.sha256(original).hexdigest())
                self.assertEqual(persisted[key], result[key])
                artifact.write_bytes(original + b"\n")
                self.assertNotEqual(result[key], hashlib.sha256(artifact.read_bytes()).hexdigest(),
                                    "an external checker must reject even a byte-only artifact change")

    def test_summary_without_output_files_does_not_claim_output_hashes(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "trace.jsonl"
            path.write_text("".join(json.dumps(event) + "\n" for event in regular_trace()))
            result = self.mod.analyze_trace(path, 0, 24)
            self.assertNotIn("per_half_csv_sha256", result)
            self.assertNotIn("stage_events_sha256", result)
            self.assertEqual(result["trace_sha256"], hashlib.sha256(path.read_bytes()).hexdigest())

    def test_actual_clock_and_one_tick_completion_uncertainty(self):
        result, rows, _ = self.analyze(regular_trace())
        self.assertAlmostEqual(float(rows[0]["dma_period_us"]), 11605.333333333334)
        self.assertAlmostEqual(float(rows[0]["slack_lower_us"]), 5000 / 5760)
        self.assertAlmostEqual(float(rows[0]["slack_upper_us"]), 5001 / 5760)
        self.assertEqual(result["clock"]["hz"], HZ)

    def test_counter_wrap_and_post_irq_counter_updates(self):
        a = snapshot(0xFFFFFFFF, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF, [0xFFFFFFF0] + [0] * 7)
        b = snapshot(0, 512, 10000, prof=[0x10] + [0] * 7)
        c = snapshot(1, 1024, 10000, prof=[0x20] + [0] * 7)
        events = [header(), dma(1), *rendered(1, a, b), dma(2), *rendered(2, b, c), dma(3),
                  dict(event="finish", cycle=HZ * 24 // 1000, completed=2, dma_edges=3, resets=0, pending_seq=None)]
        result, rows, _ = self.analyze(events)
        self.assertEqual(result["status"], "OBSERVED")
        self.assertEqual(rows[0]["late_delta"], "1")
        self.assertEqual(rows[0]["xruns_delta"], "1")
        self.assertEqual(json.loads(rows[0]["eng_prof_delta"])[0], 32)

    def test_two_dma_transitions_cannot_alias_to_zero_misses(self):
        before, after = snapshot(), snapshot(1, 512, 30000)
        one = rendered(1, before, after, complete=3 * PERIOD - 1000, dma_end=3)
        # The chronological stream has two more DMA edges before the final store/RTI.
        events = [header(), dma(1), *one[:-2], dma(2), dma(3), *one[-2:], dma(4),
                  dict(event="finish", cycle=HZ * 35 // 1000, completed=1, dma_edges=4, resets=0, pending_seq=None)]
        result, rows, _ = self.analyze(events, end_ms=35)
        self.assertEqual(result["status"], "OBSERVED")
        self.assertEqual(rows[0]["late_delta"], "0")
        self.assertEqual(rows[0]["dma_edges_during_irq"], "2")
        self.assertEqual(result["deadline"]["completed_irq_deadline_misses"], 1)
        self.assertEqual(result["deadline"]["unserviced_dma_edges"], 2)

    def test_missing_store_or_wrong_frame_count_is_incomplete(self):
        for mutation in ("last_us", "frames"):
            events = regular_trace()
            index = next(i for i, e in enumerate(events) if e["event"] == "store" and
                         e["name"] == ("x0x_dbg" if mutation == "last_us" else "eng_prof_frames") and
                         (mutation != "last_us" or e["offset"] == 32))
            del events[index]
            result, _, _ = self.analyze(events)
            self.assertEqual(result["status"], "INCOMPLETE")

    def test_dropped_stage_store_cannot_leave_a_coherent_final_snapshot(self):
        events = regular_trace()
        del events[next(i for i, e in enumerate(events) if e["event"] == "store" and e["name"] == "eng_prof")]
        result, _, _ = self.analyze(events)
        self.assertEqual(result["status"], "INCOMPLETE")
        self.assertTrue(any("store replay" in error for error in result["errors"]))

    def test_master_activity_cannot_qualify_a_silent_break_bus(self):
        events = regular_trace()
        for event in events:
            if event["event"] == "half":
                event["break_mix"] = dict(nonzero_writes=0, peak_delta=0.0, invalid=False)
        result, _, _ = self.analyze(events)
        self.assertEqual(result["status"], "OBSERVED")
        self.assertEqual(result["break_evidence"]["mix_contribution"], "NOT_OBSERVED")
        self.assertFalse(result["representative_scene_qualified"])

    def test_missing_finish_truncated_json_and_short_window_fail_closed(self):
        variants = [(regular_trace()[:-1], "", 24),
                    (regular_trace(), '{"event":', 24),
                    (regular_trace(), "", 50)]
        for events, tail, end in variants:
            result, _, _ = self.analyze(events, end_ms=end, tail=tail)
            self.assertEqual(result["status"], "INCOMPLETE")

    def test_pending_irq_outside_window_is_reported_without_losing_complete_rows(self):
        events = regular_trace()
        finish = events.pop()
        events.append(dict(event="irq_enter", seq=3, cycle=2 * PERIOD + 100, depth=1,
                           snapshot=snapshot(2, 1024, 10000)))
        finish["pending_seq"] = 3
        events.append(finish)
        result, rows, _ = self.analyze(events)
        self.assertEqual(result["status"], "OBSERVED")
        self.assertEqual(len(rows), 2)
        self.assertEqual(result["trailing_pending_seq"], 3)

    def test_duplicate_dma_or_reentered_irq_is_incomplete(self):
        repeated = dma(1); repeated["observed_cycle"] = 100
        for extra in (repeated, copy.deepcopy(regular_trace()[2])):
            events = regular_trace(); events.insert(3, extra)
            result, _, _ = self.analyze(events)
            self.assertEqual(result["status"], "INCOMPLETE")

    def test_pending_irq_inside_window_is_incomplete(self):
        events = regular_trace()[:-1]
        events.extend([dict(event="irq_enter", seq=3, cycle=2 * PERIOD + 100, depth=1,
                            snapshot=snapshot(2, 1024, 10000)), dma(4),
                       dict(event="finish", cycle=HZ * 36 // 1000, completed=2, dma_edges=4, resets=0, pending_seq=3)])
        result, _, _ = self.analyze(events, end_ms=36)
        self.assertEqual(result["status"], "INCOMPLETE")
        self.assertTrue(any("no completed IRQ" in error for error in result["errors"]))

    def test_malformed_watch_metadata_returns_incomplete_instead_of_crashing(self):
        events = regular_trace(); events[0]["watches"]["eng_prof"] = None
        result, _, _ = self.analyze(events)
        self.assertEqual(result["status"], "INCOMPLETE")

    def test_core_one_witnesses_are_not_promoted_to_full_stage_timings(self):
        result, rows, stages = self.analyze(regular_trace())
        self.assertEqual(json.loads(rows[0]["eng_job_cost"]), [1, 2, 3, 4, 5])
        self.assertEqual(rows[0]["eng_plan"], "17")
        self.assertEqual(rows[0]["eng_c1_mask"], "16")
        self.assertEqual(result["attribution"]["eng_prof_scope"], "guest_counters_with_known_core1_omissions")
        self.assertTrue(any(e["core"] == 1 for e in stages))
        self.assertNotIn("total_dsp_us", result)

    def test_wrong_source_pin_or_reset_in_window_is_incomplete(self):
        events = regular_trace(); events[0]["firmware_source"] = "0" * 40
        self.assertEqual(self.analyze(events)[0]["status"], "INCOMPLETE")
        events = regular_trace(); events.insert(3, dict(event="reset", cycle=200, pending=True))
        self.assertEqual(self.analyze(events)[0]["status"], "INCOMPLETE")

    def test_optional_scene_bytes_replay_without_claiming_scene_interpretation(self):
        result, rows, stages = self.analyze(scene_trace())
        self.assertEqual(result["status"], "OBSERVED")
        self.assertEqual(json.loads(rows[0]["scene_before"]), {"scene_flags": [0, 0, 0, 0]})
        self.assertEqual(json.loads(rows[0]["scene_after"]), {"scene_flags": [1, 2, 9, 4]})
        self.assertEqual(result["scene_observations"]["interpretation"], "UNVERIFIED")
        self.assertEqual(result["scene_observations"]["fields"]["scene_flags"]["element_bytes"], 1)
        optional = [event for event in stages if event["name"] == "scene_flags"]
        self.assertEqual(len(optional), 2)
        self.assertTrue(all(event["observation_kind"] == "scene_bytes" for event in optional))
        self.assertFalse(result["representative_scene_qualified"])

    def test_missing_optional_scene_store_is_incomplete(self):
        events = scene_trace()
        del events[next(i for i, event in enumerate(events) if event.get("name") == "scene_flags" and event["width"] == 1)]
        result, _, _ = self.analyze(events)
        self.assertEqual(result["status"], "INCOMPLETE")
        self.assertTrue(any("store replay" in error and "scene_flags" in error for error in result["errors"]))

    def test_configured_optional_snapshot_must_contain_bounded_bytes(self):
        for mutation in ("missing", "uint32", "oversize_byte"):
            events = scene_trace()
            state = next(event["snapshot"] for event in events if event["event"] == "irq_enter")
            if mutation == "missing":
                del state["scene_flags"]
            else:
                state["scene_flags"] = [0] if mutation == "uint32" else [0, 256, 0, 0]
            result, _, _ = self.analyze(events)
            self.assertEqual(result["status"], "INCOMPLETE")

    def test_unknown_non_scene_watch_and_malformed_scene_names_are_rejected(self):
        for name in ("unknown", "scene_", "scene_bad-name", "scene_Upper"):
            events = regular_trace()
            events[0]["watches"][name] = {"address": 0x01C20001, "bytes": 1, "element_bytes": 1}
            result, _, _ = self.analyze(events)
            self.assertEqual(result["status"], "INCOMPLETE")
            self.assertTrue(any("unknown observer watch" in error for error in result["errors"]))

    def test_optional_scene_range_limits_and_overlap_are_rejected(self):
        variants = [
            {"scene_large": {"address": 0x01C20000, "bytes": 513, "element_bytes": 1}},
            {"scene_word": {"address": 0x01C20000, "bytes": 4, "element_bytes": 4}},
            {"scene_overlap": {"address": 0x01C00001, "bytes": 1, "element_bytes": 1}},
            {f"scene_{i}": {"address": 0x01C20000 + i, "bytes": 1, "element_bytes": 1} for i in range(65)},
            {f"scene_{i}": {"address": 0x01C20000 + 512 * i, "bytes": 512, "element_bytes": 1} for i in range(9)},
        ]
        for extra in variants:
            events = regular_trace(); events[0]["watches"].update(extra)
            result, _, _ = self.analyze(events)
            self.assertEqual(result["status"], "INCOMPLETE")


if __name__ == "__main__":
    unittest.main()
