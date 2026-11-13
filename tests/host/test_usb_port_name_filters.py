#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Host check: USB/MIDI port filters accept both the old manufacturer (X0X)
and the new one (DnBOX), plus product tokens, and ignore unrelated ports.

No device, no mido backend: the modules are imported with a mido stub.
"""
import importlib.util
import sys
import types
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TOOLS = ROOT / "tools"


def load_tool(name, path, extra_modules=None):
    sys.modules["mido"] = types.SimpleNamespace(
        get_output_names=lambda: [],
        get_input_names=lambda: [],
        open_input=None,
        open_output=None,
        Message=None,
    )
    if extra_modules:
        for key, mod in extra_modules.items():
            sys.modules.setdefault(key, mod)
    if str(TOOLS) not in sys.path:
        sys.path.insert(0, str(TOOLS))
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


fm1_debug = load_tool("fm1_debug", TOOLS / "fm1_debug.py")
ble_autotest = load_tool("ble_autotest", TOOLS / "ble_autotest.py")


# Old firmware (manufacturer X0X) and new firmware (manufacturer DnBOX),
# including hosts that show manufacturer+product or product only.
ACCEPT = (
    "X0X",
    "X0X FM-1",
    "X0X DnBOX 0.0.25 SMK-37 Pro",
    "X0X DnBOX 0.0.25 SMK-37 Elite",
    "DnBOX",
    "DnBOX DnBOX 0.0.26 SMK-37 Pro",
    "DnBOX DnBOX 0.0.26 SMK-37 Elite",
    "DnBOX 0.0.26 SMK-37 Pro",
    "DnBOX 0.0.26 SMK-37 Elite",
    "FM-1",
    "SMK-37 Pro",
    "SMK-37 Elite",
)

# The old fm1_debug filter ("FM-1" or "X0X" only) missed these SMK port names.
FM1_DEBUG_WAS_MISS = (
    "DnBOX",
    "DnBOX DnBOX 0.0.26 SMK-37 Pro",
    "DnBOX DnBOX 0.0.26 SMK-37 Elite",
    "DnBOX 0.0.26 SMK-37 Pro",
    "DnBOX 0.0.26 SMK-37 Elite",
    "SMK-37 Pro",
    "SMK-37 Elite",
)

REJECT = (
    "IAC Driver Bus 1",
    "USB MIDI Interface",
    "Microsoft GS Wavetable Synth",
    "APC40 mkII",
)


def old_fm1_debug_matches(name):
    return "FM-1" in name or "X0X" in name


class PortNameFilters(unittest.TestCase):
    def test_both_tools_accept_old_and_new_manufacturer(self):
        for name in ACCEPT:
            with self.subTest(name=name):
                self.assertTrue(fm1_debug.port_name_matches(name), name)
                self.assertTrue(ble_autotest.port_name_matches(name), name)

    def test_unrelated_ports_are_rejected(self):
        for name in REJECT:
            with self.subTest(name=name):
                self.assertFalse(fm1_debug.port_name_matches(name), name)
                self.assertFalse(ble_autotest.port_name_matches(name), name)

    def test_fm1_debug_covers_names_the_old_filter_missed(self):
        for name in FM1_DEBUG_WAS_MISS:
            with self.subTest(name=name):
                self.assertFalse(old_fm1_debug_matches(name), name)
                self.assertTrue(fm1_debug.port_name_matches(name), name)

    def test_keys_keep_old_and_new_manufacturer(self):
        for keys in (fm1_debug.PORT_KEYS, ble_autotest.PORT_KEYS):
            self.assertIn("X0X", keys)
            self.assertIn("DnBOX", keys)
            self.assertIn("FM-1", keys)

    def test_filter_does_not_choose_elite_vs_pro(self):
        self.assertEqual(
            fm1_debug.port_name_matches("DnBOX 0.0.26 SMK-37 Pro"),
            fm1_debug.port_name_matches("DnBOX 0.0.26 SMK-37 Elite"),
        )
        self.assertEqual(
            ble_autotest.port_name_matches("DnBOX 0.0.26 SMK-37 Pro"),
            ble_autotest.port_name_matches("DnBOX 0.0.26 SMK-37 Elite"),
        )


if __name__ == "__main__":
    unittest.main()
