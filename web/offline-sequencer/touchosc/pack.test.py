# SPDX-License-Identifier: GPL-3.0-only
"""Run on the Orchestrator's box, never in the Codex authoring session."""
import importlib.util
from pathlib import Path
import tempfile
import unittest
import xml.etree.ElementTree as ET
import zlib

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("dnbox_touchosc_pack", HERE / "pack.py")
pack = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pack)


class LayoutTests(unittest.TestCase):
    def test_packed_layout_contains_real_scripts_and_distinct_interactive_controls(self):
        # Removing script embedding or reusing an ID must make this fail.
        data = pack.make_tosc(HERE)
        xml = zlib.decompress(data)
        tree = ET.fromstring(xml)
        self.assertEqual(tree.tag, "lexml")
        nodes = tree.findall(".//node")
        self.assertEqual(len({n.attrib["ID"] for n in nodes}), len(nodes))
        named = {}
        for node in nodes:
            properties = {p.findtext("key"): p.findtext("value") for p in node.findall("./properties/property")}
            named[properties["name"]] = (node, properties)
        self.assertIn("note_60", named)
        self.assertIn("note_71", named)
        self.assertIn("note_36", named)
        self.assertIn("note_46", named)
        self.assertEqual(named["note_60"][1]["script"], (HERE / "button.lua").read_text())
        self.assertEqual(named["dnbox_companion"][1]["script"], (HERE / "controller.lua").read_text())
        self.assertEqual(named["feedback"][0].findtext("./values/value/default"), "No incoming MIDI observed")
        # MIDI messages are emitted exclusively by the guarded root Lua controller.
        self.assertEqual(tree.findall(".//messages"), [])

    def test_all_controls_fit_portrait_canvas_with_finger_sized_keys(self):
        tree = ET.fromstring(pack.make_xml(HERE))
        for node in tree.findall(".//node"):
            frame = next(p.find("value") for p in node.findall("./properties/property") if p.findtext("key") == "frame")
            x, y, w, h = (float(frame.findtext(k)) for k in ("x", "y", "w", "h"))
            self.assertGreaterEqual(x, 0)
            self.assertGreaterEqual(y, 0)
            self.assertLessEqual(x + w, 480)
            self.assertLessEqual(y + h, 800)
            if node.attrib["type"] == "BUTTON":
                self.assertGreaterEqual(w, 44)
                self.assertGreaterEqual(h, 44)

    def test_missing_script_does_not_emit_a_partly_working_layout(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaises(FileNotFoundError):
                pack.make_tosc(Path(directory))

    def test_packing_is_repeatable_and_leaves_source_scripts_unchanged(self):
        originals = [(p, p.read_bytes()) for p in (HERE / "button.lua", HERE / "controller.lua")]
        first = pack.make_tosc(HERE)
        self.assertEqual(first, pack.make_tosc(HERE))
        for path, data in originals:
            self.assertEqual(path.read_bytes(), data)


if __name__ == "__main__":
    unittest.main()
