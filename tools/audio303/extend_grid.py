# SPDX-License-Identifier: GPL-3.0-only
"""Bounded host-only 2D diagnostic grid; existing production controls only."""
import pathlib,subprocess
root=pathlib.Path(__file__).resolve().parents[3]
for note in [48,60]:
 for env in [0,32,64,96,127]:
  for cut in list(range(0,128,16))+[127]:
   args=[44100,note,100,cut,0,env,0,1,0,64,127,0,21,0,0,0,0]
   subprocess.run([str(root/'production303'),str(root/'audio'/f'expanded-n{note}-e{env}-c{cut}.dsp.f32'),*map(str,args)],check=True)
print('90 diagnostic grid renders')
