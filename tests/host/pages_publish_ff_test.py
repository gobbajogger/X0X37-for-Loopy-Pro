#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Black-box test of safe GitHub Pages publication against local bare Git."""
import hashlib
import os
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
PUBLISH = ROOT / 'tools' / 'publish_pages_ff.sh'


def run(*args, cwd=None, check=True, env=None):
    p = subprocess.run(args, cwd=cwd, capture_output=True, text=True, env=env)
    if check and p.returncode:
        raise AssertionError(f"{args} exited {p.returncode}: {p.stdout} {p.stderr}")
    return p


def fwsc(name):
    # Construct only product header; site safety tests are not firmware-install tests.
    data = bytearray(960)
    for i in range(20):
        data[i * 48 + 47] = 0x7D
    for i, c in enumerate(name.encode('ascii')):
        data[i * 48 + 47] = (c + i + 1) & 0xFF
    return bytes(data)


def item(package, product):
    return {'pkg': package, 'product': product, 'size': len(fwsc(product)),
            'sha256': hashlib.sha256(fwsc(product)).hexdigest()}


class PagesPublish(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='x0x-pages-ff-test-')
        self.addCleanup(self.temp.cleanup)
        self.work = Path(self.temp.name)
        self.remote = self.work / 'remote.git'
        self.repo = self.work / 'client'
        self.site = self.work / 'generated'
        run('git', 'init', '-q', '--bare', str(self.remote))
        run('git', 'init', '-q', str(self.repo))
        run('git', 'config', 'user.name', 'test', cwd=self.repo)
        run('git', 'config', 'user.email', 'test@example.invalid', cwd=self.repo)
        run('git', 'remote', 'add', 'origin', str(self.remote), cwd=self.repo)
        (self.repo / 'README.md').write_text('not a site asset\n')
        run('git', 'add', '-A', cwd=self.repo)
        run('git', 'commit', '-qm', 'source base', cwd=self.repo)
        run('git', 'branch', '-M', 'main', cwd=self.repo)
        run('git', 'push', '-q', 'origin', 'main', cwd=self.repo)
        run('git', 'checkout', '-q', '--orphan', 'gh-pages', cwd=self.repo)
        run('git', 'rm', '-qr', '--cached', '.', cwd=self.repo)
        (self.repo / 'README.md').unlink()
        (self.repo / 'index.html').write_text('<p>old site</p>')
        (self.repo / 'manual').mkdir()
        (self.repo / 'manual/index.html').write_text('must preserve')
        (self.repo / 'install').mkdir()
        (self.repo / 'install/index.html').write_text('old installer')
        (self.repo / 'firmware').mkdir()
        self.old_bytes = fwsc('SMK-37 Elite_900')
        (self.repo / 'firmware/old-elite.fwsc').write_bytes(self.old_bytes)
        self.old_entry = {'version': 'dev-old', 'models': {'elite': item('firmware/old-elite.fwsc', 'SMK-37 Elite_900')}}
        self.old = {'schema': 2, 'latest': 'dev-old', 'versions': [self.old_entry],
                    'history': [{'id': 'previous-audit'}]}
        (self.repo / 'versions.json').write_text(json.dumps(self.old))
        run('git', 'add', '-A', cwd=self.repo)
        run('git', 'commit', '-qm', 'existing Pages version', cwd=self.repo)
        run('git', 'push', '-q', 'origin', 'gh-pages', cwd=self.repo)
        self.base = self.head()
        run('git', 'checkout', '-q', 'main', cwd=self.repo)
        self.site.mkdir()
        (self.site / 'index.html').write_text('<p>new site</p>')
        (self.site / 'install').mkdir()
        (self.site / 'install/index.html').write_text('new installer')
        (self.site / 'firmware').mkdir()
        self.new_bytes = fwsc('SMK-37 Pro_900')
        (self.site / 'firmware/new-pro.fwsc').write_bytes(self.new_bytes)
        self.new_entry = {'version': 'dev-new', 'models': {'pro': item('firmware/new-pro.fwsc', 'SMK-37 Pro_900')}}
        self.expected_catalog = {'schema': 2, 'latest': 'dev-new', 'versions': [self.new_entry, self.old_entry],
                                 'history': [{'id': 'previous-audit'}]}
        self.set_catalog(self.expected_catalog)

    def head(self):
        return run('git', '--git-dir', str(self.remote), 'rev-parse', 'refs/heads/gh-pages').stdout.strip()

    def old_remote_asset(self, path):
        return subprocess.check_output(['git', '--git-dir', str(self.remote), 'show', f'gh-pages:{path}'])

    def set_catalog(self, obj):
        (self.site / 'versions.json').write_text(json.dumps(obj, sort_keys=True))

    def publish(self, base=None):
        return run('sh', str(PUBLISH), str(self.site), base or self.base, cwd=self.repo, check=False)

    def test_preserves_old_firmware_manual_and_site_history(self):
        result = self.publish()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertNotEqual(self.head(), self.base)
        self.assertEqual(self.old_remote_asset('manual/index.html'), b'must preserve')
        self.assertEqual(self.old_remote_asset('firmware/old-elite.fwsc'), self.old_bytes)
        self.assertEqual(self.old_remote_asset('firmware/new-pro.fwsc'), self.new_bytes)
        new = json.loads(self.old_remote_asset('versions.json'))
        self.assertEqual(set(v['version'] for v in new['versions']), {'dev-old', 'dev-new'})
        self.assertEqual(run('git', '--git-dir', str(self.remote), 'rev-list', '--count', 'gh-pages').stdout.strip(), '2')

    def test_rejects_missing_old_model_and_history(self):
        self.set_catalog({'schema': 2, 'latest': 'dev-new', 'versions': [self.new_entry], 'history': []})
        result = self.publish()
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.head(), self.base)

    def test_rejects_replacement_of_old_model_package_identity(self):
        updated = json.loads(json.dumps(self.expected_catalog))
        updated['versions'][1]['models']['elite']['pkg'] = 'firmware/new-pro.fwsc'
        self.set_catalog(updated)
        result = self.publish()
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.head(), self.base)

    def test_rejects_replacing_old_firmware_bytes(self):
        (self.site / 'firmware/old-elite.fwsc').write_bytes(b'tampered')
        result = self.publish()
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.head(), self.base)

    def test_rejects_hash_mismatch_for_new_package(self):
        (self.site / 'firmware/new-pro.fwsc').write_bytes(b'invalid')
        result = self.publish()
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.head(), self.base)

    def test_rejects_duplicate_version_identifiers(self):
        updated = json.loads(json.dumps(self.expected_catalog))
        updated['versions'].append(updated['versions'][0])
        self.set_catalog(updated)
        result = self.publish()
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.head(), self.base)

    def test_rejects_nonexistent_latest_version(self):
        updated = json.loads(json.dumps(self.expected_catalog))
        updated['latest'] = 'dev-does-not-exist'
        self.set_catalog(updated)
        result = self.publish()
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.head(), self.base)

    def test_rejects_wrong_model_even_with_matching_hash(self):
        updated = json.loads(json.dumps(self.expected_catalog))
        updated['versions'][0]['models']['pro']['product'] = 'SMK-37 Elite_900'
        self.set_catalog(updated)
        result = self.publish()
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.head(), self.base)

    def test_rejects_stale_base_after_other_publisher(self):
        first = self.publish()
        self.assertEqual(first.returncode, 0, first.stderr)
        newer_sha = self.head()
        self.assertNotEqual(newer_sha, self.base)
        result = self.publish(self.base)
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.head(), newer_sha)
        self.assertIn('advanced', result.stderr)

    def test_concurrent_remote_commit_causes_non_fast_forward_refusal(self):
        # A second writer advances gh-pages after our fetch, while push is in
        # progress. Git must reject our detached worktree's divergent tip.
        hook = self.repo / '.git/hooks/pre-push'
        hook.write_text("#!/bin/sh\n" +
                        "set -eu\n" +
                        f"REMOTE='{self.remote}'\n" +
                        'BASE=$(git --git-dir="$REMOTE" rev-parse refs/heads/gh-pages)\n' +
                        'TREE=$(git --git-dir="$REMOTE" rev-parse "${BASE}^{tree}")\n' +
                        'TIP=$(GIT_AUTHOR_NAME=test GIT_AUTHOR_EMAIL=test@example.invalid '
                        'GIT_COMMITTER_NAME=test GIT_COMMITTER_EMAIL=test@example.invalid '
                        'git --git-dir="$REMOTE" commit-tree "$TREE" -p "$BASE" -m "concurrent writer")\n' +
                        'git --git-dir="$REMOTE" update-ref refs/heads/gh-pages "$TIP" "$BASE"\n')
        hook.chmod(0o755)
        result = self.publish()
        self.assertNotEqual(result.returncode, 0)
        self.assertNotEqual(self.head(), self.base)  # second writer succeeded
        self.assertEqual(self.old_remote_asset('firmware/old-elite.fwsc'), self.old_bytes)
        self.assertIn('non-fast-forward', result.stderr.lower())

    def test_rejects_private_top_level_source(self):
        (self.site / 'secrets.txt').write_text('must not distribute')
        result = self.publish()
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.head(), self.base)

    def test_rejects_symlink(self):
        (self.site / 'manual').symlink_to('/etc', target_is_directory=True)
        result = self.publish()
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.head(), self.base)

    def test_refuses_changed_existing_history(self):
        updated = json.loads(json.dumps(self.expected_catalog))
        updated['history'][0]['id'] = 'previous-audit'
        updated['history'][0]['title'] = 'unsubstantiated PASS'
        self.set_catalog(updated)
        result = self.publish()
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.head(), self.base)

    def test_refuses_unexpected_nested_source(self):
        (self.site / 'install' / 'private-build-inputs.zip').write_bytes(b'do not publish')
        result = self.publish()
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.head(), self.base)

    def test_refuses_previous_site_symlink(self):
        # Historical shared Pages root must not contain symlinks, even if
        # generated site itself is clean (copying into them can escape).
        run('git', 'checkout', '-q', 'gh-pages', cwd=self.repo)
        (self.repo / 'unsafe').symlink_to('/tmp')
        run('git', 'add', 'unsafe', cwd=self.repo)
        run('git', 'commit', '-qm', 'malformed prior site symlink', cwd=self.repo)
        run('git', 'push', '-q', 'origin', 'gh-pages', cwd=self.repo)
        self.base = self.head()
        run('git', 'checkout', '-q', 'main', cwd=self.repo)
        result = self.publish()
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.head(), self.base)

    def test_shell_wrapper_uses_history_preserving_publisher(self):
        # Isolated orchestration smoke: a minimal stand-in for make_pages.py.
        tools = self.repo / 'tools'
        tools.mkdir()
        shutil.copy2(ROOT / 'tools/publish_pages.sh', tools / 'publish_pages.sh')
        shutil.copy2(PUBLISH, tools / 'publish_pages_ff.sh')
        (tools / 'make_pages.py').write_text("""\
import pathlib, shutil, sys
args = sys.argv[1:]
out = pathlib.Path(args[args.index('--out')+1]); out.mkdir(parents=True, exist_ok=True)
(out / 'index.html').write_text('<p>regenerated</p>')
(out / 'install').mkdir(exist_ok=True)
(out / 'install/index.html').write_text('new code')
merge = pathlib.Path(args[args.index('--merge')+1])
shutil.copy2(merge / 'versions.json', out / 'versions.json')
""")
        # Preview-only invocation cannot accidentally publish. The owner must
        # make a separate affirmative choice after all test/build gates pass.
        denied = run('sh', 'tools/publish_pages.sh', '--shell', cwd=self.repo, check=False)
        self.assertNotEqual(denied.returncode, 0)
        self.assertEqual(self.head(), self.base)
        approved_env = {**os.environ, 'X0X_PAGES_CHECKED_DEV_PUBLISH': 'YES'}
        result = run('sh', 'tools/publish_pages.sh', '--shell', cwd=self.repo, check=False, env=approved_env)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.old_remote_asset('firmware/old-elite.fwsc'), self.old_bytes)
        self.assertEqual(self.old_remote_asset('manual/index.html'), b'must preserve')
        self.assertEqual(self.old_remote_asset('install/index.html'), b'new code')
        self.assertNotEqual(self.head(), self.base)

    def test_ci_workflow_is_read_only_preview(self):
        workflow = (ROOT / '.github/workflows/pages.yml').read_text()
        self.assertIn('contents: read', workflow)
        self.assertNotIn('contents: write', workflow)
        self.assertIn('actions/upload-artifact@v4', workflow)
        self.assertNotIn('actions/deploy-pages@', workflow)
        self.assertNotIn('actions/upload-pages-artifact@', workflow)
        self.assertNotIn('git push', workflow)
        self.assertNotIn('pages: write', workflow)

    def test_actual_generated_shell_publishes_known_nested_images(self):
        # Exercise the real caller, not a hand-written flat-site approximation.
        previous = self.work / 'previous'
        previous.mkdir()
        self.set_catalog(self.old)
        (previous / 'versions.json').write_text(json.dumps(self.old))
        (previous / 'firmware').mkdir()
        (previous / 'firmware/old-elite.fwsc').write_bytes(self.old_bytes)
        shutil.rmtree(self.site)
        run(sys.executable, str(ROOT / 'tools/make_pages.py'), '--shell',
            '--out', str(self.site), '--merge', str(previous), cwd=ROOT)
        for rel in ('manual/img/new/1.0-song-repeat.png',
                    'new/img/new/1.0-song-repeat.png'):
            self.assertTrue((self.site / rel).is_file(), rel)
        result = self.publish()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.old_remote_asset('manual/img/new/1.0-song-repeat.png'),
                         (ROOT / 'docs/img/new/1.0-song-repeat.png').read_bytes())
        self.assertEqual(self.old_remote_asset('firmware/old-elite.fwsc'), self.old_bytes)

    def test_noop_does_not_create_commit(self):
        # An identical generated site needs to be built from the existing branch.
        for child in list(self.site.iterdir()):
            if child.is_dir():
                shutil.rmtree(child)
            else:
                child.unlink()
        self.site.joinpath('install').mkdir()
        self.site.joinpath('index.html').write_bytes(self.old_remote_asset('index.html'))
        self.site.joinpath('install/index.html').write_bytes(self.old_remote_asset('install/index.html'))
        self.site.joinpath('versions.json').write_bytes(self.old_remote_asset('versions.json'))
        result = self.publish()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('unchanged', result.stdout)
        self.assertEqual(self.head(), self.base)


    def test_rejects_promotion_of_previous_device_untested_version(self):
        updated = json.loads(json.dumps(self.expected_catalog))
        updated['versions'][1]['status'] = 'stable'
        updated['versions'][1]['hardware_tested'] = True
        self.set_catalog(updated)
        self.assertNotEqual(self.publish().returncode, 0)
        self.assertEqual(self.head(), self.base)

    def test_rejects_prior_source_or_validation_evidence_rewrite(self):
        updated = json.loads(json.dumps(self.expected_catalog))
        updated['versions'][1]['source_commit'] = 'unproven-commit'
        updated['versions'][1]['validation_record'] = 'fake-record'
        self.set_catalog(updated)
        self.assertNotEqual(self.publish().returncode, 0)
        self.assertEqual(self.head(), self.base)

    def test_refuses_old_version_selected_as_new_latest(self):
        run('git', 'checkout', '-q', 'gh-pages', cwd=self.repo)
        prior = json.loads(json.dumps(self.old))
        prior['versions'].insert(0, self.new_entry)
        prior['latest'] = 'dev-new'
        (self.repo / 'versions.json').write_text(json.dumps(prior))
        (self.repo / 'firmware/new-pro.fwsc').write_bytes(self.new_bytes)
        run('git', 'add', 'versions.json', 'firmware', cwd=self.repo)
        run('git', 'commit', '-qm', 'publish newer DEV prior to rollback test', cwd=self.repo)
        run('git', 'push', '-q', 'origin', 'gh-pages', cwd=self.repo)
        self.base = self.head()
        run('git', 'checkout', '-q', 'main', cwd=self.repo)
        updated = json.loads(json.dumps(prior))
        updated['latest'] = 'dev-old'
        self.set_catalog(updated)
        result = self.publish()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('default', result.stderr)
        self.assertEqual(self.head(), self.base)

    def test_refuses_clearing_existing_latest(self):
        updated = json.loads(json.dumps(self.expected_catalog))
        updated['latest'] = None
        self.set_catalog(updated)
        self.assertNotEqual(self.publish().returncode, 0)
        self.assertEqual(self.head(), self.base)

    def test_rejects_new_stable_or_fabricated_device_pass(self):
        updated = json.loads(json.dumps(self.expected_catalog))
        updated['versions'][0]['status'] = 'stable'
        updated['versions'][0]['hardware_tested'] = True
        self.set_catalog(updated)
        self.assertNotEqual(self.publish().returncode, 0)
        self.assertEqual(self.head(), self.base)

    def test_retains_existing_stable_default_when_new_dev_added(self):
        run('git', 'checkout', '-q', 'gh-pages', cwd=self.repo)
        stable_catalog = json.loads(json.dumps(self.old))
        stable_catalog['versions'][0]['status'] = 'stable'
        stable_catalog['versions'][0]['hardware_tested'] = True
        (self.repo / 'versions.json').write_text(json.dumps(stable_catalog))
        run('git', 'add', 'versions.json', cwd=self.repo)
        run('git', 'commit', '-qm', 'existing stable version attested', cwd=self.repo)
        run('git', 'push', '-q', 'origin', 'gh-pages', cwd=self.repo)
        self.base = self.head()
        run('git', 'checkout', '-q', 'main', cwd=self.repo)
        updated = json.loads(json.dumps(self.expected_catalog))
        updated['versions'][1] = stable_catalog['versions'][0]
        updated['versions'][0]['status'] = 'dev-checked'
        updated['versions'][0]['hardware_tested'] = False
        self.set_catalog(updated)
        result = self.publish()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('stable default', result.stderr)
        self.assertEqual(self.head(), self.base)

    def test_rejects_new_unlisted_firmware(self):
        (self.site / 'firmware/unlisted.fwsc').write_bytes(fwsc('SMK-37 Elite_900'))
        result = self.publish()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('unlisted', result.stderr)
        self.assertEqual(self.head(), self.base)

    def test_rejects_nested_firmware_name(self):
        nested = self.site / 'firmware/nested'
        nested.mkdir()
        (nested / 'lost.fwsc').write_bytes(fwsc('SMK-37 Pro_900'))
        result = self.publish()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('nested', result.stderr)
        self.assertEqual(self.head(), self.base)

    def test_rejects_nested_manual_image(self):
        nested = self.site / 'manual/img/nested'
        nested.mkdir(parents=True)
        (nested / 'secret.png').write_bytes(b'private')
        result = self.publish()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('nested', result.stderr)
        self.assertEqual(self.head(), self.base)

    def test_preserves_old_published_manual_image_bytes(self):
        run('git', 'checkout', '-q', 'gh-pages', cwd=self.repo)
        previous = self.repo / 'manual/img/screen-909.png'
        previous.parent.mkdir(parents=True)
        previous.write_bytes(b'previously-published-documentation-image')
        run('git', 'add', 'manual/img/screen-909.png', cwd=self.repo)
        run('git', 'commit', '-qm', 'publish original manual image', cwd=self.repo)
        run('git', 'push', '-q', 'origin', 'gh-pages', cwd=self.repo)
        self.base = self.head()
        run('git', 'checkout', '-q', 'main', cwd=self.repo)
        image = self.site / 'manual/img/screen-909.png'
        image.parent.mkdir(parents=True)
        image.write_bytes(b'different image bytes')
        result = self.publish()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('published image', result.stderr)
        self.assertEqual(self.head(), self.base)
        self.assertEqual(self.old_remote_asset('manual/img/screen-909.png'),
                         b'previously-published-documentation-image')

    def test_valid_new_image_can_be_added_without_replacing_old(self):
        image = self.site / 'manual/img/new-example.png'
        image.parent.mkdir(parents=True)
        image.write_bytes(b'new-image')
        result = self.publish()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.old_remote_asset('manual/img/new-example.png'), b'new-image')



if __name__ == '__main__':
    unittest.main(verbosity=2)
