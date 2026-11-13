#!/usr/bin/env python3
# A stand-in for tools/prefs_lint.py --git REPO REV in the status.sh test: one failed entry.
print("PREFS FAIL 1 of 3 entries")
print("  NO STEP line 5 2026-10-17 02:00: an entry with no rule")
raise SystemExit(1)
