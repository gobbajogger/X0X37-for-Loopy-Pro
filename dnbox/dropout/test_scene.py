"""Host scene checks. Synthetic matches are never an emulator qualification.

One rejection fixture replays selected measured final-RAM values from the first
short run (RAM SHA256 ed68df77cb9b39f90401bb8513bdcfda98739e208b59825c6ac55936f5400e98).
It verifies rejection logic only; this test does not execute firmware.
"""
import csv
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("dropout_scene", Path(__file__).with_name("scene.py"))
scene_check = importlib.util.module_from_spec(spec)
spec.loader.exec_module(scene_check)


def declaration(bpm=140):
    return {"firmware_source": scene_check.FIRMWARE_SOURCE, "kind": "representative",
            "owner_scene_exact": False, "start_ms": 20000, "end_ms": 320000, "intended": {
                "tempo_bpm": bpm,
                "synth_a": {"engine": "Chicago", "internal_arp": "on", "usb_channel": 2,
                            "held_notes": [48, 52, 55]},
                "synth_b": {"engine": "Juno", "usb_channel": 3, "held_notes": [60, 64, 67]},
                "break": {"slot": 1, "loop_index": 0, "steps": "0xffff", "b_chance": 0},
                "909": {"voice": "BD", "steps": "0x1111"},
                "808": {"all_steps": "empty", "external_notes": "none"}}}


def u32(value):
    return list(struct.pack("<I", value))


def matching(bpm=140):
    snapshot = {name: [0] * size for name, _, _, size in scene_check.WATCHES}
    snapshot.update(scene_a_kind=[0], scene_a_status=[1], scene_a_arp_enable=[1],
                    scene_b_kind=[3], scene_b_status=[1],
                    scene_seq_pat=u32(0x01C0C3A4), scene_seq_playing=[1],
                    scene_seq_ch=[9, 10, 1, 2, 3], scene_seq_bpm=list(struct.pack("<f", bpm)),
                    scene_909_hit=list(struct.pack("<22I", 0x1111, *([0] * 21))),
                    scene_909_len=[16], scene_808_len=[16], scene_brk_steps=u32(0xffff),
                    scene_brk_set=[25, 60, 20, 50, 10, 0, 5, 0, 2, 0, 2, 2])
    for name, notes in (("scene_a_usb_keys", [48, 52, 55]), ("scene_b_usb_keys", [60, 64, 67])):
        for key in notes:
            snapshot[name][key] = 1
    return snapshot


def summary(count, edges=None, start=20000, end=320000):
    return {"status": "OBSERVED", "firmware_source": scene_check.FIRMWARE_SOURCE,
            "window": {"completed_halves": count, "coherent_halves": count,
                       "dma_edges": count if edges is None else edges,
                       "start_ms": start, "end_ms": end},
            "scene_observations": {"fields": scene_check.scene_metadata(
                scene_check.SYMBOLS, scene_check.ELF_SHA256["elite"])}}


class SceneTests(unittest.TestCase):
    def check(self, snapshot=None, declared=None):
        return scene_check.check_snapshot(matching() if snapshot is None else snapshot,
                                          declaration() if declared is None else declared)

    def csv_check(self, rows, trace_summary, *, bind_digest=True, **window):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "per_half.csv"
            with path.open("w", newline="") as output:
                writer = csv.DictWriter(output, fieldnames=["dma_seq", "coherent", "scene_before", "scene_after"])
                writer.writeheader()
                for sequence, coherent, before, after in rows:
                    writer.writerow(dict(dma_seq=sequence, coherent=coherent,
                                         scene_before=json.dumps(before), scene_after=json.dumps(after)))
            if trace_summary is not None and bind_digest:
                trace_summary = dict(trace_summary)
                trace_summary.setdefault("per_half_csv_sha256", hashlib.sha256(path.read_bytes()).hexdigest())
            return scene_check.check_window(path, declaration(), trace_summary, **window)

    def test_map_accepts_both_exact_images_and_uses_raw_nonoverlapping_ranges(self):
        maps = [scene_check.scene_map(scene_check.SYMBOLS, value) for value in scene_check.ELF_SHA256.values()]
        self.assertEqual(maps[0], maps[1])
        self.assertIn("scene_a_kind 0x01c3d7c0 1\n", maps[0])
        self.assertIn("scene_b_kind 0x01c3ebc8 1\n", maps[0])
        metadata = scene_check.scene_metadata(scene_check.SYMBOLS, scene_check.ELF_SHA256["elite"])
        self.assertEqual(len(metadata), 26)
        self.assertEqual(sum(w["bytes"] for w in metadata.values()), 503)
        self.assertTrue(all(w["element_bytes"] == 1 for w in metadata.values()))
        self.assertNotIn("scene_brk_bchance", metadata)  # Already contained in set[12].

    def test_map_rejects_unpinned_or_moved_or_resized_or_missing_aggregates(self):
        with self.assertRaisesRegex(ValueError, "SHA256"):
            scene_check.scene_map(scene_check.SYMBOLS, "0" * 64)
        for replacement in ((0x01C3C3C0, 10256), (0x01C3C3BC, 10260), None):
            symbols = dict(scene_check.SYMBOLS)
            symbols["original_bass"] = replacement
            with self.subTest(replacement=replacement), self.assertRaises(ValueError):
                scene_check.scene_map(symbols, scene_check.ELF_SHA256["elite"])

    def test_final_ram_extraction_uses_the_verified_layout(self):
        ram = bytearray(scene_check.RAM_BYTES)
        snapshot = matching()
        metadata = scene_check.scene_metadata(scene_check.SYMBOLS, scene_check.ELF_SHA256["elite"])
        for name, watch in metadata.items():
            offset = watch["address"] - scene_check.RAM_BASE
            ram[offset:offset + watch["bytes"]] = bytes(snapshot[name])
        extracted = scene_check.snapshot_from_ram(ram, scene_check.SYMBOLS, scene_check.ELF_SHA256["elite"])
        self.assertEqual(extracted, snapshot)
        with self.assertRaisesRegex(ValueError, "1 MiB"):
            scene_check.snapshot_from_ram(ram[:-1], scene_check.SYMBOLS, scene_check.ELF_SHA256["elite"])

    def test_matching_values_have_only_boundary_scope_and_no_audibility_claim(self):
        result = self.check()
        self.assertEqual(result["status"], "OBSERVED_MATCH")
        self.assertTrue(result["required_state_matches"])
        self.assertFalse(result["owner_scene_exact"])
        self.assertEqual(result["audibility"], "UNVERIFIED")
        self.assertIn("boundaries only", result["evidence_scope"])

    def test_measured_wrong_short_run_is_rejected_without_false_slot_error(self):
        # Measured final snapshot values; checks replay only, no firmware execution.
        snapshot = matching(139)
        snapshot.update(scene_a_kind=[1], scene_a_arp_enable=[0], scene_b_kind=[0],
                        scene_808_hit=list(struct.pack("<22I", 0xffff, *([0] * 21))),
                        scene_brk_steps=u32(0), scene_brk_slot_a=[0])
        result = self.check(snapshot)
        self.assertEqual(result["status"], "REJECTED")
        fields = {v["field"] for v in result["violations"]}
        self.assertTrue({"scene_a_kind", "scene_a_arp_enable", "scene_b_kind", "scene_seq_bpm",
                         "scene_808_hit", "scene_brk_steps"}.issubset(fields))
        self.assertNotIn("scene_brk_slot_a", fields)

    def test_bpm_is_taken_from_scene_and_compared_as_target_binary32(self):
        self.assertTrue(self.check(matching(139), declaration(139))["required_state_matches"])
        self.assertFalse(self.check(matching(139), declaration(140))["required_state_matches"])
        malformed = declaration(float("nan"))
        with self.assertRaises(ValueError):
            self.check(declared=malformed)

    def test_physical_slot_one_is_logical_loop_zero_and_bchance_is_checked(self):
        snapshot = matching()
        self.assertTrue(self.check(snapshot)["required_state_matches"])
        snapshot["scene_brk_slot_a"] = [1]
        self.assertIn("scene_brk_slot_a", {v["field"] for v in self.check(snapshot)["violations"]})
        snapshot["scene_brk_slot_a"] = [0]
        snapshot["scene_brk_set"][9] = 100
        self.assertEqual(self.check(snapshot)["violations"][0]["field"], "scene_brk_set[BRK_BCHANCE]")

    def test_pattern_routing_status_muting_and_any_808_row_are_required(self):
        cases = {"scene_seq_pat": u32(0x01C0C634), "scene_seq_ppat": [0, 0, 0, 0, 1],
                 "scene_a_status": [0], "scene_b_status": [255], "scene_seq_ch": [9, 10, 2, 1, 3],
                 "scene_seq_playing": [0], "scene_seq_part_off": [16], "scene_seq_mute": u32(1 << 24),
                 "scene_seq_vmute": u32(1), "scene_808_hit": [0] * 87 + [128],
                 "scene_909_hit": list(struct.pack("<22I", 0x1111, 0, 1, *([0] * 19)))}
        for name, value in cases.items():
            snapshot = matching(); snapshot[name] = value
            with self.subTest(name=name):
                self.assertFalse(self.check(snapshot)["required_state_matches"])

    def test_extra_missing_or_duplicated_held_keys_do_not_pass(self):
        for key, count in ((48, 0), (48, 2), (49, 1)):
            snapshot = matching(); snapshot["scene_a_usb_keys"][key] = count
            with self.subTest(key=key, count=count):
                self.assertFalse(self.check(snapshot)["required_state_matches"])

    def test_incomplete_malformed_nonfinite_or_word_encoded_snapshots_rejected(self):
        variants = []
        missing = matching(); missing.pop("scene_a_kind"); variants.append(missing)
        words = matching(); words["scene_seq_pat"] = [0x01C0C3A4]; variants.append(words)
        boolean = matching(); boolean["scene_seq_playing"] = [True]; variants.append(boolean)
        nonfinite = matching(); nonfinite["scene_seq_bpm"] = list(struct.pack("<f", float("nan"))); variants.append(nonfinite)
        other = matching(); other["scene_unreviewed"] = [0]; variants.append(other)
        for snapshot in variants:
            with self.subTest(fields=len(snapshot)):
                self.assertFalse(self.check(snapshot)["required_state_matches"])

    def test_window_checks_both_boundaries_of_every_coherent_half(self):
        result = self.csv_check([(100, 1, matching(), matching()), (101, 1, matching(), matching())], summary(2))
        self.assertEqual(result["status"], "OBSERVED_MATCH")
        self.assertEqual(result["checked_boundaries"], 4)
        wrong = matching(); wrong["scene_b_kind"] = [0]
        result = self.csv_check([(100, 1, matching(), matching()), (101, 1, matching(), wrong)], summary(2))
        self.assertEqual(result["status"], "REJECTED")
        self.assertEqual(result["failing_boundaries"], 1)
        self.assertEqual(result["failures"][0]["boundary"], "after")

    def test_window_needs_trace_coverage_correct_addresses_and_coherent_rows(self):
        row = (100, 1, matching(), matching())
        foreign = summary(1)
        foreign["scene_observations"]["fields"]["scene_a_kind"]["address"] += 1
        for evidence in (None, summary(2), foreign):
            with self.subTest(evidence=evidence):
                result = self.csv_check([row], evidence)
                self.assertEqual(result["status"], "INCOMPLETE")
                self.assertFalse(result["required_state_matches_at_all_observed_boundaries"])
        result = self.csv_check([(100, 0, matching(), matching())], summary(1))
        self.assertEqual(result["status"], "INCOMPLETE")
        result = self.csv_check([], summary(0))
        self.assertEqual(result["status"], "INCOMPLETE")

    def test_unserviced_edges_are_counted_without_inventing_scene_samples(self):
        result = self.csv_check([(100, 1, matching(), matching()), (102, 1, matching(), matching())], summary(2, edges=3))
        self.assertEqual(result["status"], "OBSERVED_MATCH")
        self.assertEqual(result["unobserved_dma_edges"], 1)
        self.assertEqual(result["checked_boundaries"], 4)
        self.assertEqual(result["continuous_state_between_boundaries"], "UNVERIFIED")

    def test_window_requires_the_exact_csv_digest_from_its_reducer(self):
        row = (100, 1, matching(), matching())
        wrong = summary(1); wrong["per_half_csv_sha256"] = "0" * 64
        for evidence in (summary(1), wrong):
            with self.subTest(digest=evidence.get("per_half_csv_sha256")):
                result = self.csv_check([row], evidence, bind_digest=False)
                self.assertEqual(result["status"], "INCOMPLETE")
                self.assertFalse(result["required_state_matches_at_all_observed_boundaries"])
                self.assertTrue(any("CSV SHA256" in reason for reason in result["incomplete_reasons"]))

    def test_window_default_requires_the_declared_interval(self):
        row = (100, 1, matching(), matching())
        missing = summary(1); del missing["window"]["start_ms"]
        for evidence in (summary(1, start=20000, end=29000),
                         summary(1, start=21000, end=321000), missing):
            with self.subTest(window=evidence["window"]):
                result = self.csv_check([row], evidence)
                self.assertEqual(result["status"], "INCOMPLETE")
                self.assertFalse(result["required_state_matches_at_all_observed_boundaries"])
        result = self.csv_check([row], summary(1))
        self.assertEqual(result["declared_window"], {"start_ms": 20000, "end_ms": 320000})
        self.assertEqual(result["observed_window"], result["declared_window"])
        self.assertFalse(result["observation_window_override"])
        self.assertFalse(result["partial_duration"])
        self.assertFalse(result["representative_scene_qualified"])

    def test_explicit_short_interval_is_bound_and_remains_partial(self):
        row = (100, 1, matching(), matching())
        arguments = dict(observed_start_ms=20000, observed_end_ms=29000)
        result = self.csv_check([row], summary(1, start=20000, end=29000), **arguments)
        self.assertEqual(result["status"], "OBSERVED_MATCH")
        self.assertTrue(result["required_state_matches_at_all_observed_boundaries"])
        self.assertEqual(result["declared_window"], {"start_ms": 20000, "end_ms": 320000})
        self.assertEqual(result["observed_window"], {"start_ms": 20000, "end_ms": 29000})
        self.assertTrue(result["observation_window_override"])
        self.assertTrue(result["partial_duration"])
        self.assertFalse(result["representative_scene_qualified"])
        wrong = self.csv_check([row], summary(1), **arguments)
        self.assertEqual(wrong["status"], "INCOMPLETE")

    def test_window_override_must_be_paired_finite_and_increasing(self):
        row = (100, 1, matching(), matching())
        for arguments in (dict(observed_start_ms=20000), dict(observed_end_ms=29000),
                          dict(observed_start_ms=True, observed_end_ms=29000),
                          dict(observed_start_ms=-1, observed_end_ms=29000),
                          dict(observed_start_ms=20000, observed_end_ms=float("inf")),
                          dict(observed_start_ms=20000, observed_end_ms=20000)):
            with self.subTest(arguments=arguments), self.assertRaises(ValueError):
                self.csv_check([row], summary(1), **arguments)

    def test_known_optional_808_predicate_is_sampled_only_and_can_be_absent(self):
        absent = self.check()
        self.assertEqual(absent["status"], "OBSERVED_MATCH")
        self.assertEqual(absent["808_live_inactivity"], "UNVERIFIED")
        self.assertEqual(absent["sampled_808_activity"]["status"], "NOT_OBSERVED")
        snapshot = matching(); snapshot["scene_808_active"] = [0]
        present = self.check(snapshot)
        self.assertEqual(present["status"], "OBSERVED_MATCH")
        self.assertEqual(present["808_live_inactivity"], "UNVERIFIED")
        self.assertEqual(present["sampled_808_activity"]["value"], 0)
        self.assertIn("sampled", present["sampled_808_activity"]["scope"])

    def test_optional_808_active_or_malformed_predicate_cannot_match(self):
        for value in ([1], [255], [-1], [True], [0, 0]):
            snapshot = matching(); snapshot["scene_808_active"] = value
            with self.subTest(value=value):
                result = self.check(snapshot)
                self.assertEqual(result["status"], "REJECTED")
                self.assertFalse(result["required_state_matches"])
        snapshot = matching(); snapshot["scene_808_active"] = [1]
        self.assertEqual(self.check(snapshot)["violations"][0]["field"], "scene_808_active")

    def test_window_optional_808_metadata_and_snapshot_fields_must_agree(self):
        snapshot = matching(); snapshot["scene_808_active"] = [0]
        row = (100, 1, snapshot, snapshot)
        metadata = {"address": 29393605, "bytes": 1, "element_bytes": 1}
        evidence = summary(1)
        evidence["scene_observations"]["fields"]["scene_808_active"] = dict(metadata)
        result = self.csv_check([row], evidence)
        self.assertEqual(result["status"], "OBSERVED_MATCH")
        self.assertEqual(result["808_live_inactivity"], "UNVERIFIED")
        for key, wrong in (("address", 29393606), ("bytes", 2), ("element_bytes", 4)):
            foreign = summary(1)
            foreign["scene_observations"]["fields"]["scene_808_active"] = dict(metadata, **{key: wrong})
            with self.subTest(key=key):
                self.assertEqual(self.csv_check([row], foreign)["status"], "INCOMPLETE")
        self.assertEqual(self.csv_check([row], summary(1))["status"], "INCOMPLETE")
        self.assertEqual(self.csv_check([(100, 1, matching(), matching())], evidence)["status"], "INCOMPLETE")

    def test_declared_other_scene_cannot_weaken_the_required_engine_checks(self):
        declared = declaration(); declared["intended"]["synth_a"]["engine"] = "Miami"
        with self.assertRaises(ValueError): self.check(declared=declared)
        declared = declaration(); declared["owner_scene_exact"] = True
        with self.assertRaises(ValueError): self.check(declared=declared)


if __name__ == "__main__":
    unittest.main()
