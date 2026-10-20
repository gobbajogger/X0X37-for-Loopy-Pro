#!/usr/bin/env python3
"""Current decision shapes must be checked; historical text remains inert."""
import sys
from pathlib import Path
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[2]/'tools'))
import prefs_lint as lint

class Decisions(unittest.TestCase):
    def check(self, text):
        return lint.lint(text, {'integration.md': '### Rule 11: validation\nMaintain Elite and Pro target builds and shared tests.'})

    def test_actual_missed_shapes(self):
        for title in ('Build target scope', 'On-demand rules'):
            rows = self.check(f'## {title} — 2026-10-20\n\nSource: owner.\n\nCurrent requirements. Active rules: rules/core.md.\n')
            self.assertTrue(any(r['status'] == 'UNRECOGNIZED' for r in rows))

    def test_new_heading_still_checks_dated_entry(self):
        rows = self.check('## New decision area\n\n- 2026-10-20 Maintain Elite and Pro target builds. Active rule: integration.md rule 11.\n')
        self.assertEqual([r['status'] for r in rows], ['OK'])

    def test_undated_and_prose_fail(self):
        for line in ('- Maintain Elite.', 'Maintain Elite.', '* 2026-10-20 Maintain Elite.'):
            self.assertEqual(self.check('## Team rules\n'+line)[0]['status'], 'UNRECOGNIZED')

    def test_decisions_before_heading_and_source_only_section(self):
        rows = self.check('# Current decisions\n\n- 2026-10-20 New mandatory workflow.\n')
        self.assertEqual([r['status'] for r in rows], ['NO STEP'])
        rows = self.check('## Team rules\nSource: new mandatory workflow.\n')
        self.assertEqual([r['status'] for r in rows], ['UNRECOGNIZED'])

    def test_history_unchanged_and_invalid_pointer_fails(self):
        rows = self.check('## Team rules\n- 2026-10-20 Maintain Elite.\n## Historical preference record\n## Old — 2020-01-01\nold text')
        self.assertEqual([r['status'] for r in rows], ['NO STEP'])

if __name__ == '__main__':
    unittest.main()
