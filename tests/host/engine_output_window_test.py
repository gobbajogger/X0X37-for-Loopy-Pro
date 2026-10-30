# SPDX-License-Identifier: GPL-3.0-only
from pathlib import Path
import json,subprocess,hashlib,shutil,os,argparse
parser=argparse.ArgumentParser(description="Compare actual engine output and force a raw-alias negative.")
parser.add_argument('--baseline-ref',default='7e4bbfd95da1d9ad86c80541b9bceae48231d006')
parser.add_argument('--output',required=True)
args=parser.parse_args()
S=Path(__file__).resolve().parents[2];D=Path(args.output).resolve();D.mkdir(parents=True,exist_ok=True);E=D;cmds=[]
baseline=subprocess.check_output(['git','show',args.baseline_ref+':firmware/src/app/engine.c'],cwd=S,text=True)
assert '    static float out_l[256], out_r[256];' in baseline
(E/'engine-baseline.c').write_text(baseline)
(E/'engine-raw-alias.c').write_text(baseline.replace('    static float out_l[256], out_r[256];', '#if X0X_DEV_WOBBLE\n    float *out_l=t0,*out_r=tr;\n#else\n    static float out_l[256], out_r[256];\n#endif'))
assert (S/'build/gen/x0x_drum_samples.h').exists(), 'Prepare unchanged assets with sh host/build_host.sh --generate-only'
def run(a,log,check=True):
 p=subprocess.run(list(map(str,a)),cwd=S,capture_output=True,text=True,env=os.environ|{"ASAN_OPTIONS":"detect_leaks=0"});(D/log).write_text(p.stdout+p.stderr);cmds.append({'argv':list(map(str,a)),'cwd':str(S),'exit':p.returncode,'log':log});(D/'COMMANDS.json').write_text(json.dumps(cmds,indent=2));assert not check or p.returncode==0,(log,p.stdout[-700:]+p.stderr[-700:]);return p.returncode
units=['dsp/drum909','dsp/drum808','dsp/x0x_param','dsp/bass303','dsp/breaks','dsp/fxbus','dsp/master','seq/sequencer','seq/tb3po','seq/pattern','seq/motion','app/factory_banks']
for wobble in [1,0]:
 for mode in ['normal','sanitized']:
  flags=['-O1','-g','-ffp-contract=off','-std=c99','-DX0X_HOST',f'-DX0X_WOBBLE={wobble}',f'-DX0X_DEV_WOBBLE={wobble}','-D_POSIX_C_SOURCE=200809L','-D_DARWIN_C_SOURCE','-Ifirmware/src','-Ifirmware/src/app','-Ifirmware/src/dsp','-Ibuild/gen']
  if mode=='sanitized':flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
  common=[]
  for unit in units+['wobble']:
   src='experiments/wobble/wobble.c' if unit=='wobble' else 'firmware/src/'+unit+'.c';obj=D/f'{wobble}-{mode}-{unit.replace("/","_")}.o';run(['cc',*flags,'-c',src,'-o',obj],obj.stem+'.log');common.append(obj)
  for variant,src in [('baseline',E/'engine-baseline.c'),('raw',E/'engine-raw-alias.c'),('fixed',S/'firmware/src/app/engine.c')]:
   if variant=='raw' and (wobble==0 or mode=='sanitized'):continue
   tag=f'{wobble}-{mode}-{variant}';obj=D/(tag+'.o');exe=D/tag
   run(['cc',*flags,'-Dseq_until_event=output_window_until_event','-Dplat_cpu2_run=output_window_cpu2_run','-Dplat_cpu2_wait=output_window_cpu2_wait','-c',src,'-o',obj],tag+'-compile.log')
   run(['cc',*flags,'tests/host/engine_output_window_test.c',*common,obj,'-lm','-o',exe],tag+'-link.log')
   run([exe,D/(tag+'.pcm')],tag+'-run.log')
   print(tag,'PASS',flush=True)
  b=(D/f'{wobble}-{mode}-baseline.pcm').read_bytes();f=(D/f'{wobble}-{mode}-fixed.pcm').read_bytes();assert b==f,(wobble,mode,'DIFF')
  if wobble==1 and mode=='normal':assert b!=(D/f'{wobble}-{mode}-raw.pcm').read_bytes(),'raw alias negative failed to reproduce'
print('ALL_DIFFERENTIALS_PASS_RAW_ALIAS_FAILS',flush=True)
