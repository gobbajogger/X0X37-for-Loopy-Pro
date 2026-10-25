# SPDX-License-Identifier: GPL-3.0-only
"""Derive the original held-out octave sequence without executing the AU."""
import pathlib,json
root=pathlib.Path(__file__).resolve().parents[3]
trials=json.loads((root/'evidence/manifest.json').read_text());held=[]
for t in trials:
 if t['name']=='overlap_glide_mid':
  t['id']=t['id'].replace('overlap_glide_mid','slide-holdout-n60');t['name']='slide-holdout-n60';t['note']=60
  for event in t['events']:event[2]+=12
  held.append(t)
assert len(held)==2
(root/'evidence/slide-holdout-manifest.json').write_text(json.dumps(held,indent=2))
