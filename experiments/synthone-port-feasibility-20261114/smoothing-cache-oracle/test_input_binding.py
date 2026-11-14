#!/usr/bin/env python3
"""The source identity verifier must bind the actual compiled subtree too."""
import os,subprocess,sys,tempfile
from pathlib import Path
here=Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='synthone-wrong-soundpipe-') as wrong:
    env=dict(os.environ,SOUNDPIPE_ROOT=wrong)
    p=subprocess.run([sys.executable,str(here/'verify_inputs.py')],env=env,capture_output=True,text=True)
    assert p.returncode!=0 and 'SOUNDPIPE_ROOT must resolve to the pinned AUDIOKIT_ROOT subtree' in p.stderr,(p.returncode,p.stdout,p.stderr)
print('INPUT_BINDING_NEGATIVE_PASS mismatched_compile_subtree_rejected=1')
