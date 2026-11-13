"""Focused pure-host tests; source clip checksum/ffprobe execution is separate."""
from __future__ import annotations

import importlib.util
from pathlib import Path
import tempfile
import unittest

TOOL = Path(__file__).resolve().parents[1] / "tools/box_entry/validate_asset.py"
spec = importlib.util.spec_from_file_location("dnbox_box_entry_asset_gate", TOOL)
asset = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(asset)


class BoxEntryAssetGateTests(unittest.TestCase):
    def setUp(self) -> None:
        self.probe = {"streams": [{"codec_type": "video", "codec_name": "h264",
                                   "width": 480, "height": 480, "avg_frame_rate": "70/1",
                                   "nb_read_frames": "36", "duration": "0.514286"}]}

    def test_valid_expected_geometry(self) -> None:
        info = asset.verify_probe(self.probe)
        self.assertEqual((info["frames"], info["fps"]), (36, 70))
        self.assertAlmostEqual(info["duration_seconds"], 36 / 70)

    def test_reject_wrong_endpoint_timing_or_audio(self) -> None:
        bad = {"streams": [{**self.probe["streams"][0], "nb_read_frames": "192"}]}
        with self.assertRaises(ValueError):
            asset.verify_probe(bad)
        bad = {"streams": [self.probe["streams"][0], {"codec_type": "audio"}]}
        with self.assertRaises(ValueError):
            asset.verify_probe(bad)
        bad = {"streams": [{**self.probe["streams"][0], "avg_frame_rate": "60/1"}]}
        with self.assertRaises(ValueError):
            asset.verify_probe(bad)
        bad = {"streams": [{**self.probe["streams"][0], "duration": "1.314286"}]}
        with self.assertRaises(ValueError):
            asset.verify_probe(bad)

    def test_fail_closed_on_corrupt_bytes_or_dimensions(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            f = Path(td) / "fake.png"
            f.write_bytes(b"\x89PNG\r\n\x1a\n" + b"\0" * 16)
            with self.assertRaises(ValueError):
                asset.png_dimensions(f)
            with self.assertRaises(ValueError):
                asset.verify_bytes(f, 24, "0" * 64)
            f.write_bytes(b"\x89PNG\r\n\x1a\n" + b"\0\0\0\0IHDR" + (480).to_bytes(4,"big") + (480).to_bytes(4,"big"))
            self.assertEqual(asset.png_dimensions(f), (480, 480))


if __name__ == "__main__":
    unittest.main()
