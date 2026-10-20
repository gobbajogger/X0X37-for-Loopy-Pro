#!/usr/bin/env python3
"""Check rule discovery rollout, safe backups, and repeat behavior in scratch repos."""
import json
from pathlib import Path
import shutil
import subprocess
import tempfile


def run(*args):
    return subprocess.check_output(args, text=True)


def main():
    source = Path(__file__).resolve().parents[2]
    with tempfile.TemporaryDirectory(prefix='x0x-rules-rollout-') as temporary:
        workspace = Path(temporary)
        (workspace / 'agent-rules.md').write_text('old workspace rules\n')
        canonical = workspace / 'flash_review_integration'
        for repo in [canonical, workspace / 'x0x37', workspace / 'old', workspace / 'third-party']:
            repo.mkdir()
            run('git', 'init', '-q', str(repo))
            (repo / 'AGENTS.md').write_text('old checkout rules\n')
            if repo.name != 'third-party':
                (repo / 'build.sh').write_text('# build marker\n')
                (repo / 'tools').mkdir()
                (repo / 'tools/fm1_install.py').write_text('# installer marker\n')
        shutil.copytree(source / 'docs/rules', canonical / 'docs/rules')
        shutil.copytree(source / 'skills', canonical / 'skills')
        for name in ['agent-rules.md', 'SKILLS.md']:
            shutil.copy2(source / 'docs' / name, canonical / 'docs' / name)
        shutil.copy2(source / 'AGENTS.md', canonical / 'AGENTS.md')
        shutil.copy2(source / 'tools/rollout_rules.py', canonical / 'tools/rollout_rules.py')
        # A copied entry with missing rules must still use the working wrapper.
        (workspace / 'old/docs').mkdir()
        shutil.copy2(source / 'docs/agent-rules.md', workspace / 'old/docs/agent-rules.md')
        old_hook = ('#!/bin/sh\n# Mac copy of the x0x37 pre-push hook.\n'
                    '# Rollout and repair notes (on the box): /workspace/x0x37-build-and-test-runner/failclosed-hook-rollout.md\n'
                    'FIX="fix the clone (unshallow or repair it; notes on the box: /workspace/x0x37-build-and-test-runner/failclosed-hook-rollout.md)"\n'
                    'echo "$FIX"\nexit 7\n')
        (workspace / 'old/.git/hooks/pre-push').write_text(old_hook)
        (workspace / 'old/.git/hooks/pre-push').chmod(0o755)
        command = ['python3', str(canonical / 'tools/rollout_rules.py'), '--workspace', str(workspace), '--source', str(canonical)]
        archive = Path('archive/rules-2026-10-17/evidence.md')
        (canonical/'docs'/archive).parent.mkdir(parents=True, exist_ok=True)
        (canonical/'docs'/archive).write_text('canonical old evidence')
        (workspace/archive).parent.mkdir(parents=True, exist_ok=True)
        (workspace/archive).write_text('unique historical evidence')
        before = (workspace / 'old/AGENTS.md').read_bytes()
        assert subprocess.run([*command, '--check'], stdout=subprocess.DEVNULL).returncode == 1
        assert json.loads(run(*command))['files'] > 0
        assert (workspace / 'old/AGENTS.md').read_bytes() == before
        result = json.loads(run(*command, '--apply'))
        backup = Path(result['backup'])
        assert (backup / 'old/AGENTS.md').read_bytes() == before
        assert (backup / 'old/.git/hooks/pre-push').read_text() == old_hook
        repaired_hook = (workspace / 'old/.git/hooks/pre-push').read_text()
        assert 'BUILDING.md' in repaired_hook
        assert repaired_hook.endswith('echo "$FIX"\nexit 7\n')
        assert (workspace / 'old/.git/hooks/pre-push').stat().st_mode & 0o111
        assert (workspace / 'third-party/AGENTS.md').read_text() == 'old checkout rules\n'
        assert '(../AGENTS.md)' in (workspace / 'old/AGENTS.md').read_text()
        assert 'classic ChatGPT inside LoopyX0X37' in (workspace / 'AGENTS.md').read_text()
        assert 'references/routing.md' in (workspace / 'old/AGENTS.md').read_text()
        skill = workspace / '.agents/skills/x0x37-task/SKILL.md'
        assert skill.read_bytes() == (canonical / 'skills/x0x37-task/SKILL.md').read_bytes()
        assert (workspace / 'old/.agents/skills/x0x37-install/SKILL.md').resolve() == (canonical / 'skills/x0x37-install/SKILL.md').resolve()
        assert '../.agents/skills/x0x37-task/SKILL.md' in (workspace / 'rules/core.md').read_text()
        assert '.agents/skills/x0x37-task/SKILL.md' in (workspace / 'agent-rules.md').read_text()
        assert not (workspace / 'third-party/.agents').exists()
        assert (workspace/archive).read_text() == 'unique historical evidence'
        assert json.loads(run(*command))['files'] == 0
        assert json.loads(run(*command, '--apply'))['files'] == 0
        assert (source / 'AGENTS.md').read_text() in (canonical / 'AGENTS.md').read_text()
        assert 'old checkout rules' in (workspace / 'old/AGENTS.md').read_text()
        assert subprocess.run([*command, '--check'], stdout=subprocess.DEVNULL).returncode == 0
        # Identical docs with absent or stale skill files must use the wrapper.
        shutil.copytree(canonical / 'docs', workspace / 'x0x37/docs')
        run(*command, '--apply')
        assert '(../AGENTS.md)' in (workspace / 'x0x37/AGENTS.md').read_text()
        shutil.copytree(canonical / 'skills', workspace / 'x0x37/skills')
        run(*command, '--apply')
        assert '(../AGENTS.md)' in (workspace / 'x0x37/AGENTS.md').read_text()
        (workspace / 'x0x37/skills/caveman/SKILL.md').write_text('stale skill\n')
        run(*command, '--apply')
        assert '(../AGENTS.md)' in (workspace / 'x0x37/AGENTS.md').read_text()
        # A stale workflow link is backed up and repaired without touching its target.
        link = workspace / 'old/.agents/skills/x0x37-install'
        link.unlink()
        link.symlink_to('../../../../unrelated')
        result = json.loads(run(*command, '--apply'))
        saved = Path(result['backup']) / 'old/.agents/skills/x0x37-install'
        assert saved.is_symlink() and saved.readlink() == Path('../../../../unrelated')
        assert (link / 'SKILL.md').read_bytes() == (canonical / 'skills/x0x37-install/SKILL.md').read_bytes()
        # Unknown local skill directories are never overwritten.
        link.unlink()
        link.mkdir()
        (link / 'keep').write_text('local work')
        (workspace / 'agent-rules.md').write_text('Must remain unchanged after preflight refusal.\n')
        result = subprocess.run([*command, '--apply'], capture_output=True, text=True)
        assert result.returncode != 0
        assert (link / 'keep').read_text() == 'local work'
        assert (workspace / 'agent-rules.md').read_text() == 'Must remain unchanged after preflight refusal.\n'
        shutil.rmtree(link)
        for relative in ['old/.agents/skills', 'old/.agents']:
            parent = workspace / relative
            saved_parent = parent.with_name(parent.name + '-saved')
            parent.rename(saved_parent)
            parent.write_text('Local file blocks discovery.\n')
            result = subprocess.run([*command, '--apply'], capture_output=True, text=True)
            assert result.returncode != 0
            assert (workspace / 'agent-rules.md').read_text() == 'Must remain unchanged after preflight refusal.\n'
            assert parent.read_text() == 'Local file blocks discovery.\n'
            parent.unlink()
            parent.symlink_to(temporary + '-outside')
            result = subprocess.run([*command, '--apply'], capture_output=True, text=True)
            assert result.returncode != 0
            assert (workspace / 'agent-rules.md').read_text() == 'Must remain unchanged after preflight refusal.\n'
            parent.unlink()
            saved_parent.rename(parent)
    print('PASS rule rollout: dry run, backups, discovery, third-party exclusion, repeat')


if __name__ == '__main__':
    main()
