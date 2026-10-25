# SPDX-License-Identifier: GPL-3.0-only
"""EXPERIMENTAL host-only spectral anchor. Does not change firmware or presets.
Refuses unmeasured notes, interpolation and rejected lookup entries.
Output is native float32 mono at44100Hz. Gain correction is explicit in sidecar.
"""
import argparse,array,json,math,pathlib,subprocess,tempfile

def main():
 root=pathlib.Path(__file__).resolve().parents[3]
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--cutoff',type=float,required=True)
 p.add_argument('--note',type=int,choices=[48,60],required=True)
 p.add_argument('--lookup',type=pathlib.Path,default=root/'evidence/experimental-cutoff-lookup.json')
 p.add_argument('--renderer',type=pathlib.Path,default=root/'production303')
 p.add_argument('output',type=pathlib.Path)
 a=p.parse_args();lookup=json.loads(a.lookup.read_text())
 entries=[x for x in lookup['entries'] if x['chicago_cutoff']==a.cutoff and x['usable_exploratory_anchor']]
 if len(entries)!=1:p.error('No accepted measured anchor; interpolation and failed entries are unsupported')
 if a.output.exists() or a.output.with_suffix('.json').exists():p.error('Output or sidecar already exists')
 e=entries[0]
 with tempfile.TemporaryDirectory() as td:
  raw=pathlib.Path(td)/'render.f32'
  args=[44100,a.note,100,e['production_cutoff'],0,e['production_envmod'],e['production_wave'],1,0,64,127,0,21,0,0,0,0]
  subprocess.run([str(a.renderer),str(raw),*map(str,args)],check=True)
  x=array.array('f');x.frombytes(raw.read_bytes());gain=10**(e['gain_db']/20)
  out=array.array('f',(v*gain for v in x))
  if not all(math.isfinite(v) for v in out):raise ValueError('Nonfinite output')
  with a.output.open('xb') as f:out.tofile(f)
  meta=dict(name=lookup['name'],note=a.note,velocity=100,rate=44100,frames=len(out),output='native float32 mono',source_arguments=args,explicit_gain_db=e['gain_db'],measurement=e,limitations='Stationary spectral anchor only; envelope, phase, resonance, slide and accent not calibrated; no firmware edits')
  with a.output.with_suffix('.json').open('x') as f:json.dump(meta,f,indent=2)
 return 0
if __name__=='__main__':raise SystemExit(main())
