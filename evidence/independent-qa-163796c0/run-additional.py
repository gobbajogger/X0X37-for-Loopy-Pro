import subprocess,pathlib,json,concurrent.futures,time
r=pathlib.Path('/workspace/work/recut/qa-163796c0');src=r/'source'
common='cc -std=c99 -O2 -ffp-contract=off -fno-fast-math -Wall -Wextra -Werror '
checks=[
('engine-level','python3 tests/host/engine_mix_lut_test.py level'),
('engine-send','python3 tests/host/engine_mix_lut_test.py send'),
('undo-word',common+'-Wno-unused-function tests/host/undo_word_scan_test.c -o ../undo-test && ../undo-test'),
('lz-max',common+'-Wno-unused-function tests/host/lz_max_match_test.c -o ../lz-test && ../lz-test'),
('busfx',common+'-Ifirmware/src/dsp tests/host/busfx_invariant_test.c -lm -o ../busfx-test && ../busfx-test'),
('break-lut',common+'-Ifirmware/src/dsp tests/host/break_retrigger_lut_test.c -lm -o ../break-lut-test && ../break-lut-test'),
('gain-lut',common+'-Iexperiments/original_synth tests/host/original_gain_lut_test.c -lm -o ../gain-lut-test && ../gain-lut-test'),
('project-stream','sh tests/host/run_project_stream_copy.sh'),
('909-emit','python3 tools/gen_drum_samples.py build/gen/x0x_drum_samples.h && sh tests/host/run_drum909_emit_equivalence.sh')]
def run(x):
 name,cmd=x;start=time.monotonic()
 with open(r/(name+'.log'),'w') as out:
  p=subprocess.run(['bash','-c',cmd],cwd=src,stdout=out,stderr=subprocess.STDOUT)
 z={'name':name,'command':cmd,'exit':p.returncode,'seconds':round(time.monotonic()-start,3)};print(json.dumps(z),flush=True);return z
# project/909 header generators are serialized to avoid writes to the same generated header.
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as ex:results=list(ex.map(run,checks[:7]))
results.append(run(checks[7]));results.append(run(checks[8]))
json.dump(results,open(r/'additional-results.json','w'),indent=2)

