"""Hand-calculated synthetic PCM checks; never firmware or audibility results."""
from fractions import Fraction
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import struct
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("dropout_pcm", Path(__file__).with_name("pcm.py"))
pcm = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pcm)

HZ = 5_760_000_000
PERIOD = 66_846_720  # 512 frames * 544 timer ticks/frame * 240 emulator ticks/timer tick.


def header():
    return {"event": "observer", "schema": 1, "hz": HZ, "cycle_ticks": 16,
            "clock_unit": "emulator_tick", "firmware_source": pcm.FIRMWARE_SOURCE,
            "runner_source": pcm.RUNNER_SOURCE}


def trace(count=3, boot_ticks=0):
    events = [header()]
    for index in range(count):
        scheduled = boot_ticks + index * PERIOD
        events.append({"event": "dma", "seq": index + 1, "scheduled_cycle": scheduled,
                       "observed_cycle": scheduled, "deadline_cycle": scheduled + PERIOD,
                       "half": index % 2, "words": 1024,
                       "address": 0x01C10000 + index % 2 * 4096})
    events.append({"event": "finish", "cycle": boot_ticks + max(count, 1) * PERIOD,
                   "dma_edges": count, "completed": 0, "pending_seq": None, "resets": 0})
    return events


def chunk(left, right=None):
    right = left if right is None else right
    return struct.pack("<hh", left, right) * 512


class PcmTests(unittest.TestCase):
    def analyze(self, data, events=None, start=0, end=30, tail=""):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            pcm_path, trace_path = root / "audio.raw", root / "trace.jsonl"
            pcm_path.write_bytes(data)
            trace_path.write_text("".join(json.dumps(e) + "\n" for e in (trace() if events is None else events)) + tail)
            return pcm.analyze_pcm(pcm_path, trace_path, start, end)

    def test_digital_zero_is_observed_zero_not_audible_or_unobserved(self):
        result = self.analyze(chunk(0) * 3)
        self.assertEqual(result["status"], "OBSERVED")
        self.assertEqual(result["numeric_output"], "DIGITAL_ZERO")
        self.assertEqual(result["samples"]["scalar_samples"], 3072)
        self.assertEqual(result["samples"]["nonzero_samples"], 0)
        self.assertEqual(result["samples"]["peak_absolute_int16"], 0)
        self.assertEqual(result["samples"]["rms_int16"], 0)
        self.assertIsNone(result["samples"]["rms_dbfs"])
        self.assertEqual(result["samples"]["rms_dbfs_kind"], "negative_infinity")
        self.assertEqual(result["human_audibility"], "UNVERIFIED")
        self.assertEqual(result["break_attribution"], "NOT_EVALUATED")
        json.dumps(result, allow_nan=False)

    def test_positive_half_scale_has_hand_calculated_rms_and_dbfs(self):
        #1024 samples *16384**2 =2**38; RMS/full-scale=1/2;20log10(1/2).
        result = self.analyze(chunk(16384), trace(1), end=10)
        stats = result["samples"]
        self.assertEqual(stats["nonzero_samples"], 1024)
        self.assertEqual(stats["peak_absolute_int16"], 16384)
        self.assertEqual(stats["sum_squares_int16"], 1 << 38)
        self.assertEqual(stats["rms_int16"], 16384)
        self.assertEqual(stats["rms_full_scale"], 0.5)
        self.assertAlmostEqual(stats["rms_dbfs"], -6.020599913279624, places=12)

    def test_negative_full_scale_does_not_overflow_abs_or_squares(self):
        # abs(int16 minimum)=32768 in Python's integer domain; RMS=full scale.
        result = self.analyze(chunk(-32768), trace(1), end=10)
        stats = result["samples"]
        self.assertEqual(stats["peak_absolute_int16"], 32768)
        self.assertEqual(stats["sum_squares_int16"], 1 << 40)
        self.assertEqual(stats["rms_int16"], 32768)
        self.assertEqual(stats["rms_dbfs"], 0)

    def test_stereo_channel_statistics_are_independent(self):
        result = self.analyze(chunk(16384, 0), trace(1), end=10)
        self.assertEqual(result["samples"]["nonzero_samples"], 512)
        self.assertAlmostEqual(result["samples"]["rms_int16"], 16384 / math.sqrt(2), places=9)
        self.assertAlmostEqual(result["samples"]["rms_dbfs"], -9.030899869919436, places=12)
        self.assertEqual(result["channels"]["left"]["rms_int16"], 16384)
        self.assertEqual(result["channels"]["right"]["rms_int16"], 0)

    def test_boot_delay_has_no_file_padding_and_end_is_exclusive(self):
        boot = 5 * HZ
        events = trace(3, boot)
        begin = Fraction((boot + PERIOD) * 1000, HZ)
        end = Fraction((boot + 2 * PERIOD) * 1000, HZ)
        data = chunk(1000) + chunk(2000) + chunk(3000)
        result = self.analyze(data, events, begin, end)
        window = result["window"]
        self.assertEqual((window["first_dma_seq"], window["last_dma_seq"]), (2, 2))
        self.assertEqual((window["pcm_byte_first"], window["pcm_byte_end_exclusive"]), (2048, 4096))
        self.assertEqual((window["scalar_sample_first"], window["scalar_sample_end_exclusive"]), (1024, 2048))
        self.assertEqual(result["samples"]["rms_int16"], 2000)
        self.assertEqual(result["pcm_sha256"], hashlib.sha256(data).hexdigest())
        self.assertEqual(result["window_pcm_sha256"], hashlib.sha256(chunk(2000)).hexdigest())
        self.assertEqual(result["clock"]["sample_rate_stereo_frames_per_second"],
                         {"numerator": 24_000_000, "denominator": 544})

    def test_window_before_first_dma_is_no_samples_not_zero_audio(self):
        result = self.analyze(chunk(1234) * 3, trace(3, 5 * HZ), start=0, end=1000)
        self.assertEqual(result["status"], "NO_SAMPLES")
        self.assertEqual(result["numeric_output"], "NOT_OBSERVED")
        self.assertEqual(result["window"]["selected_dma_count"], 0)
        self.assertIsNone(result["samples"]["rms_int16"])
        self.assertIsNone(result["samples"]["peak_absolute_int16"])
        self.assertEqual(result["window_pcm_sha256"], hashlib.sha256(b"").hexdigest())

    def test_selection_uses_scheduled_dma_timestamp_not_irq_or_observed_time(self):
        events = trace(1)
        events[1]["observed_cycle"] = 10_000
        # DMA scheduled at0 is selected even though its host drain is observed
        # after this1us window ends. The returned timestamps expose the difference.
        result = self.analyze(chunk(1), events, start=0, end=0.001)
        self.assertEqual(result["window"]["selected_dma_count"], 1)
        self.assertEqual(result["window"]["first_observed_tick"], 10_000)
        self.assertIn("NOT_CLAIMED", result["same_irq_render_attribution"])

    def test_missing_first_middle_or_last_dma_is_rejected(self):
        for index in (1, 2, 3):
            events = trace(); del events[index]
            with self.subTest(index=index), self.assertRaisesRegex(ValueError, "sequence|count"):
                self.analyze(chunk(1) * 3, events)

    def test_truncated_odd_length_extra_or_boot_padded_pcm_is_rejected(self):
        correct = chunk(1) * 3
        for data in (correct[:-1], correct[:-2048], correct + b"\0", chunk(0) + correct):
            with self.subTest(bytes=len(data)), self.assertRaisesRegex(ValueError, "byte count"):
                self.analyze(data)

    def test_wrong_source_clock_period_words_half_or_sequence_is_rejected(self):
        cases = [(0, "firmware_source", "0" * 40), (0, "runner_source", "0" * 40),
                 (0, "hz", 360_000_000), (0, "cycle_ticks", 1), (1, "words", 512),
                 (1, "half", 1), (2, "seq", 1), (2, "scheduled_cycle", PERIOD + 1),
                 (1, "deadline_cycle", PERIOD + 1)]
        for index, field, value in cases:
            events = trace(); events[index][field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                self.analyze(chunk(1) * 3, events)

    def test_missing_finish_reset_count_reset_event_and_truncated_json_fail_closed(self):
        with self.assertRaisesRegex(ValueError, "finish"):
            self.analyze(chunk(1) * 3, trace()[:-1])
        events = trace(); events[-1]["resets"] = 1
        with self.assertRaisesRegex(ValueError, "reset"):
            self.analyze(chunk(1) * 3, events)
        events = trace(); events.insert(2, {"event": "reset", "cycle": 1})
        with self.assertRaisesRegex(ValueError, "reset"):
            self.analyze(chunk(1) * 3, events)
        with self.assertRaises(ValueError):
            self.analyze(chunk(1) * 3, tail='{"event":')

    def test_window_beyond_finish_and_nonfinite_bounds_are_rejected(self):
        for start, end in ((0, 1000), (0, float("nan")), (True, 30), (30, 30), (-1, 30)):
            with self.subTest(start=start, end=end), self.assertRaises(ValueError):
                self.analyze(chunk(1) * 3, start=start, end=end)

    def test_pcm_mapping_does_not_claim_irq_completeness(self):
        events = trace(1)
        events.insert(2, {"event": "incomplete", "cycle": 10, "reason": "synthetic IRQ incompleteness"})
        result = self.analyze(chunk(16384), events, end=10)
        self.assertEqual(result["numeric_output"], "NONZERO")
        self.assertEqual(result["trace_irq_qualification"], "NOT_EVALUATED")
        self.assertEqual(result["scene_qualification"], "NOT_EVALUATED")


if __name__ == "__main__":
    unittest.main()
