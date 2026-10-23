#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Guard direct POSIX CLI entrypoint executable bits after Git checkout."""
import os
from pathlib import Path
import stat
import unittest

ROOT = Path(os.environ.get("X0X_TEST_REPO_ROOT", Path(__file__).resolve().parents[1]))


@unittest.skipUnless(os.name == "posix", "Git POSIX executable-mode contract")
class CliModes(unittest.TestCase):
    def test_sample_backup_cli_is_executable(self):
        p = ROOT / "tools" / "x0x_slots.py"
        self.assertTrue(p.stat().st_mode & stat.S_IXUSR, f"{p} must be Git mode 100755")

    def test_firmware_installer_cli_is_executable(self):
        p = ROOT / "tools" / "fm1_install.py"
        self.assertTrue(p.stat().st_mode & stat.S_IXUSR, f"{p} must be Git mode 100755")


if __name__ == "__main__":
    unittest.main(verbosity=2)
