#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The manual HTML generator uses DnBOX as the page title and home link.

A markdown H1 of X0X is the old brand title and is rendered as DnBOX.
Body text that names older firmware is left as written.
"""
import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def load_manual_html():
    spec = importlib.util.spec_from_file_location(
        "manual_html", ROOT / "tools" / "manual_html.py"
    )
    mod = importlib.util.module_from_spec(spec)
    # Avoid running main(); the module only defines helpers and main.
    sys.modules["manual_html"] = mod
    spec.loader.exec_module(mod)
    return mod


class ManualHtmlTitle(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mod = load_manual_html()

    def render(self, markdown):
        work = Path(tempfile.mkdtemp())
        src = work / "in.md"
        dst = work / "out.html"
        src.write_text(markdown)
        argv = sys.argv
        try:
            sys.argv = ["manual_html.py", str(src), str(dst)]
            self.mod.main()
        finally:
            sys.argv = argv
        return dst.read_text()

    def test_old_brand_h1_becomes_dnbox_title(self):
        html = self.render("# X0X\n\nA unit on older firmware still shows X0X.\n")
        self.assertIn("<title>DnBOX Manual</title>", html)
        self.assertIn('<h1><a class="home" href="../">DnBOX</a></h1>', html)
        self.assertIn("for DnBOX, groovebox firmware", html)
        self.assertNotIn("<title>X0X Manual</title>", html)
        self.assertNotIn('href="../">X0X</a>', html)
        # Body text that names old firmware is kept.
        self.assertIn("older firmware still shows X0X", html)

    def test_section_page_is_prefixed_dnbox(self):
        html = self.render("# What's new\n\nNotes.\n")
        self.assertIn("<title>DnBOX: What&#x27;s new</title>", html)
        self.assertIn('<a class="home" href="../">DnBOX</a>', html)
        self.assertIn("<h1>What's new</h1>", html)

    def test_committed_snapshot_title_matches(self):
        html = (ROOT / "docs" / "manual.html").read_text()
        self.assertIn("<title>DnBOX Manual</title>", html)
        self.assertIn('<h1><a class="home" href="../">DnBOX</a></h1>', html)
        self.assertNotIn("<title>X0X Manual</title>", html)


if __name__ == "__main__":
    unittest.main()
