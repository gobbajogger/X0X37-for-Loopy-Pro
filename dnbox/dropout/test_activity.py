"""Hand-derived synthetic store traces; no firmware execution or timing claims."""
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

FW = "94197fc839b00aff47b98ca02f3c6396e4a6eabf"
RUNNER = "31157d25caf7284080cdb5d86ab7e8e438120fd9"
ELFS = ("720f851c83a62fbef71fe52ad84e669409a69fde1d1fb313427d7942ed3ca1ea",
        "7e67c3b33fe2e690e5f913b5d048727a715c70daca06ddd326a10e37eacb9535")
WATCH = "scene_808_active"
META = {"address": 29393605, "bytes": 1, "element_bytes": 1}


def store(seq, name, value, core=0, cycle=100):
    event = dict(event="store", seq=seq, cycle=cycle, core=core, name=name,
                 offset=0, width=1 if name == WATCH else 4, value=value,
                 ticks_since_irq_entry=0)
    if name == WATCH:
        event["observation_kind"] = "scene_bytes"
    return event


def half(seq=7, values=(0, 0)):
    events = []
    for value in values:
        # All timestamps tie intentionally: actual append order, including core1,
        # defines the store ordering. A core-based sort would break this trace.
        events += [store(seq, "eng_stage", 0), store(seq, WATCH, value, core=1),
                   store(seq, "eng_stage", 1), store(seq, "eng_stage", 2, core=1),
                   store(seq, "eng_stage", 4)]
    return events


def summary(raw, halves=1, edges=None):
    return {"schema": 1, "status": "OBSERVED", "firmware_source": FW,
            "runner_source": RUNNER, "error_count": 0,
            "stage_events_sha256": hashlib.sha256(raw).hexdigest(),
            "window": {"start_ms": 20000, "end_ms": 20024,
                       "completed_halves": halves, "coherent_halves": halves,
                       "dma_edges": halves if edges is None else edges},
            "scene_observations": {"fields": {WATCH: dict(META)}}}


class ActivityTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        path = Path(__file__).with_name("activity.py")
        cls.mod = None
        if path.exists():
            spec = importlib.util.spec_from_file_location("dropout_activity", path)
            cls.mod = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(cls.mod)

    def setUp(self):
        self.assertIsNotNone(self.mod, "per-stretch 808 activity checker is missing")

    def check(self, events, *, halves=1, edges=None, mutate_summary=None, suffix=b"", strip_newline=False):
        raw = b"".join(json.dumps(event).encode() + b"\n" for event in events) + suffix
        if strip_newline:
            raw = raw.rstrip(b"\n")
        report = summary(raw, halves, edges)
        if mutate_summary:
            mutate_summary(report)
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "stage_events.jsonl"
            path.write_bytes(raw)
            return self.mod.check_808_inactive(path, report)

    def test_map_requires_actual_symbol_address_size_and_retained_elf(self):
        for digest in ELFS:
            self.assertEqual(self.mod.activity_map({"d808_was_active": (29393605, 1)}, digest),
                             "scene_808_active 0x01c082c5 1\n")
        for symbols, digest in (({}, ELFS[0]), ({"d808_was_active": (29393606, 1)}, ELFS[0]),
                                ({"d808_was_active": (29393605, 4)}, ELFS[0]),
                                ({"d808_was_active": (29393605, True)}, ELFS[0]),
                                ({"d808_was_active": (29393605, 1)}, "0" * 64)):
            with self.subTest(symbols=symbols, digest=digest), self.assertRaises(ValueError):
                self.mod.activity_map(symbols, digest)

    def test_two_fresh_equal_zero_stores_qualify_observed_stretches_only(self):
        result = self.check(half())
        self.assertEqual(result["status"], "INACTIVE_OBSERVED")
        self.assertTrue(result["all_observed_stretches_inactive"])
        self.assertTrue(result["mix_guard_excluded_at_all_observed_stretches"])
        self.assertTrue(result["coverage_complete"])
        self.assertEqual(result["total_stretches"], 2)
        self.assertEqual(result["observed_dma_sequences"], [7])
        self.assertEqual(result["per_half"][0]["stretch_resets"], 2)
        self.assertEqual(result["per_half"][0]["stage2_completions"], 2)
        self.assertEqual(result["per_half"][0]["activity_stores"], 2)
        self.assertFalse(result["owner_scene_exact"])
        self.assertFalse(result["representative_scene_qualified"])
        self.assertEqual(result["zero_808_cpu_cost"], "UNVERIFIED")

    def test_transient_active_store_is_seen_even_when_final_store_is_zero(self):
        events = half(values=(1, 0))
        self.assertEqual([e["value"] for e in events if e["name"] == WATCH], [1, 0])
        result = self.check(events)
        self.assertEqual(result["status"], "ACTIVE_OBSERVED")
        self.assertTrue(result["coverage_complete"])
        self.assertEqual(result["active_stores"], 1)
        self.assertFalse(result["all_observed_stretches_inactive"])
        self.assertFalse(result["mix_guard_excluded_at_all_observed_stretches"])

    def test_missing_or_duplicate_fresh_zero_cannot_reuse_previous_value(self):
        missing = half(); del missing[1]
        duplicate = half(); duplicate.insert(2, copy.deepcopy(duplicate[1]))
        for events in (missing, duplicate):
            with self.subTest(stores=len(events)):
                result = self.check(events)
                self.assertEqual(result["status"], "UNVERIFIED")
                self.assertFalse(result["all_observed_stretches_inactive"])
                self.assertTrue(result["incomplete_reasons"])

    def test_missing_reset_completion_or_second_stretch_rejects_coverage(self):
        for index in (0, 3, 5, 8):
            events = half(); del events[index]
            with self.subTest(omitted=index):
                self.assertEqual(self.check(events)["status"], "UNVERIFIED")
        self.assertEqual(self.check(half(values=(0,)))["status"], "UNVERIFIED")

    def test_store_outside_reset_completion_interval_is_not_coverage(self):
        events = half(); moved = events.pop(1); events.insert(0, moved)
        self.assertEqual(self.check(events)["status"], "UNVERIFIED")

    def test_hash_mismatch_does_not_bind_an_active_or_inactive_observation(self):
        for value in (0, 1):
            with self.subTest(value=value):
                result = self.check(half(values=(value, 0)),
                                    mutate_summary=lambda report: report.update(stage_events_sha256="0" * 64))
                self.assertEqual(result["status"], "UNVERIFIED")
                self.assertFalse(result["provenance_verified"])
                self.assertFalse(result["all_observed_stretches_inactive"])

    def test_missing_digest_watch_or_wrong_watch_address_rejects_inactive_claim(self):
        def no_digest(report):
            del report["stage_events_sha256"]
        def no_watch(report):
            report["scene_observations"]["fields"] = {}
        def wrong_watch(report):
            report["scene_observations"]["fields"][WATCH]["address"] += 1
        for mutation in (no_digest, no_watch, wrong_watch):
            with self.subTest(mutation=mutation.__name__):
                self.assertEqual(self.check(half(), mutate_summary=mutation)["status"], "UNVERIFIED")

    def test_incomplete_or_wrong_source_summary_cannot_qualify_inactivity(self):
        mutations = (lambda report: report.update(status="INCOMPLETE"),
                     lambda report: report.update(firmware_source="0" * 40),
                     lambda report: report.update(runner_source="0" * 40),
                     lambda report: report["window"].update(coherent_halves=0),
                     lambda report: report.update(error_count=1))
        for index, mutation in enumerate(mutations):
            with self.subTest(case=index):
                self.assertEqual(self.check(half(), mutate_summary=mutation)["status"], "UNVERIFIED")

    def test_truncation_malformed_record_or_partial_store_rejects_coverage(self):
        self.assertEqual(self.check(half(), suffix=b'{"event":')["status"], "UNVERIFIED")
        self.assertEqual(self.check(half(), strip_newline=True)["status"], "UNVERIFIED")
        events = half(); events[1]["width"] = 2
        self.assertEqual(self.check(events)["status"], "UNVERIFIED")
        events = half(); events[0]["width"] = 1
        self.assertEqual(self.check(events)["status"], "UNVERIFIED")

    def test_missing_entire_half_or_nonmonotonic_group_rejects_coverage(self):
        self.assertEqual(self.check(half(), halves=2)["status"], "UNVERIFIED")
        self.assertEqual(self.check(half(8) + half(7), halves=2)["status"], "UNVERIFIED")

    def test_unobserved_dma_edges_are_explicit_without_inventing_state(self):
        result = self.check(half(7) + half(9), halves=2, edges=3)
        self.assertEqual(result["status"], "INACTIVE_OBSERVED")
        self.assertEqual(result["total_stretches"], 4)
        self.assertEqual(result["unobserved_dma_edges"], 1)
        self.assertEqual(result["observed_dma_sequences"], [7, 9])
        self.assertEqual(result["inactivity_on_unobserved_dma_edges"], "UNVERIFIED")


if __name__ == "__main__":
    unittest.main()
