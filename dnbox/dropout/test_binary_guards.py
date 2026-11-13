"""Optional real fm1-live entry-point checks; no firmware is executed."""
import os
from pathlib import Path
import subprocess
import unittest


@unittest.skipUnless(os.environ.get("DNBOX_TEST_EMULATOR"), "set DNBOX_TEST_EMULATOR to the built host executable")
class BinaryGuardTests(unittest.TestCase):
    def test_timing_and_flash_fault_overrides_fail_before_loading_firmware(self):
        for name in ("FM1_USB_SIE_NS", "FM1_FLASH_CUT"):
            with self.subTest(variable=name):
                env = {k: v for k, v in os.environ.items() if not k.startswith("FM1_")}
                env.update(FM1_STRICT="1", FM1_DNBOX_MAP="/unopened.map", FM1_DNBOX_TRACE="/unopened.trace")
                env[name] = "1"
                result = subprocess.run([str(Path(os.environ["DNBOX_TEST_EMULATOR"]).resolve()),
                    "--fwsc", "/intentionally-missing-private-input.fwsc", "--mhz", "360", "--shot", "0"],
                    env=env, capture_output=True, text=True)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("DnBOX observer requires", result.stderr)


if __name__ == "__main__":
    unittest.main()
