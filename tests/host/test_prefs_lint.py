#!/usr/bin/env python3
"""Current decision shapes must be checked; historical text remains inert."""
import sys
from pathlib import Path
import unittest
import subprocess
import tempfile
sys.path.insert(0, str(Path(__file__).resolve().parents[2]/'tools'))
import prefs_lint as lint

class Decisions(unittest.TestCase):
    def test_workspace_rule_mirrors_use_discovery(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root/'rules').mkdir()
            source = root/'maintained/skills/x0x37-install'
            source.mkdir(parents=True)
            (source/'SKILL.md').write_text('### Rule 6: install\nInstall the complete validated development set.\n')
            (root/'.agents/skills').mkdir(parents=True)
            (root/'.agents/skills/x0x37-install').symlink_to(source)
            (root/'rules/install.md').write_text('<!-- x0x37-skill: skills/x0x37-install/SKILL.md -->\n')
            prefs = root/'USER_PREFERENCES.md'
            prefs.write_text('## Installs\n- 2026-10-21 Install the complete validated development set. Active rule: install.md rule 6.\n')
            result = subprocess.run([sys.executable,lint.__file__,str(prefs)],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)

    def test_skill_redirects_check_actual_body_and_missing_sources(self):
        files = {'install.md': '<!-- x0x37-skill: skills/x0x37-install/SKILL.md -->'}
        body = '### Rule 6: complete install\nInstall the complete validated development set.'
        resolved = lint.resolve_skill_entries(files, lambda path: body)
        prefs = '## Installs\n- 2026-10-21 Install the complete validated development set. Active rule: install.md rule 6.\n'
        self.assertEqual(lint.lint(prefs, resolved)[0]['status'], 'OK')
        resolved = lint.resolve_skill_entries(files, lambda path: 'No rule remains.')
        self.assertEqual(lint.lint(prefs, resolved)[0]['status'], 'NO STEP')
        with self.assertRaises(FileNotFoundError):
            lint.resolve_skill_entries(files, lambda path: (_ for _ in ()).throw(FileNotFoundError(path)))
        with self.assertRaises(ValueError):
            lint.resolve_skill_entries({'core.md': '<!-- x0x37-skill: ../outside -->'}, lambda path: body)

    def test_skill_redirects_in_saved_git_revision(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root/'docs/rules').mkdir(parents=True)
            (root/'skills/x0x37-install').mkdir(parents=True)
            (root/'docs/rules/install.md').write_text('<!-- x0x37-skill: skills/x0x37-install/SKILL.md -->\n')
            (root/'skills/x0x37-install/SKILL.md').write_text('### Rule 6: install\nInstall the complete validated development set.\n')
            (root/'docs/USER_PREFERENCES.md').write_text('## Installs\n- 2026-10-21 Install the complete validated development set. Active rule: install.md rule 6.\n')
            subprocess.run(['git','init','-q',str(root)],check=True)
            subprocess.run(['git','-C',str(root),'add','.'],check=True)
            subprocess.run(['git','-C',str(root),'-c','user.name=test','-c','user.email=test@example.com','commit','-qm','fixture'],check=True)
            result = subprocess.run([sys.executable,lint.__file__,'--git',str(root),'HEAD'],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
            (root/'skills/x0x37-install/SKILL.md').unlink()
            result = subprocess.run([sys.executable,lint.__file__,str(root/'docs/USER_PREFERENCES.md')],capture_output=True,text=True)
            self.assertEqual(result.returncode,2,result.stdout+result.stderr)

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
