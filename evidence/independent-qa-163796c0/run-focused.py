import subprocess,pathlib,concurrent.futures,json,time
root=pathlib.Path('/workspace/work/recut/qa-163796c0');src=root/'source'
checks=[
('ima-delta', ['bash','tests/host/run_ima_delta_lut.sh']),
('vca-sustain',['sh','tests/host/vca_sustain_test.sh',str(root/'build-vca')]),
('insert-idle',['sh','tests/host/chicago_insert_idle_identity_test.sh',str(root/'build-insert-idle')]),
('fx2-zero',['sh','tests/host/chicago_fx2_zero_skip_test.sh',str(root/'build-fx2-zero')]),
('master-idle',['sh','tests/host/master_idle_identity_test.sh',str(root/'build-master-idle')]),
('fxbus-idle',['sh','tests/host/fxbus_idle_identity_test.sh',str(root/'build-fxbus-idle')]),
('fxbus-cache',['sh','tests/host/fxbus_coef_cache_test.sh']),
('peaking-cache',['sh','tests/host/chicago_peaking_coef_cache_test.sh']),
('bpm-unchanged',['sh','tests/host/chicago_bpm_unchanged_tempo_test.sh']),
('modulation-hoist',['python3','tests/host/run_chicago_modulation_hoist.py']),
('ensemble-hoist',['python3','tests/host/run_chicago_ens_hoist.py']),
('a3-master',['python3','tests/host/run_a3_dsp_equivalence.py','master']),
('a3-drum808',['python3','tests/host/run_a3_dsp_equivalence.py','drum808']),
('a3-fxbus',['python3','tests/host/run_a3_dsp_equivalence.py','fxbus']),
('break-silent',['sh','tests/host/breaks_silent_skip_test.sh']),
('clip-latency',['sh','tests/host/clip_latency_equivalence_test.sh'])]
def run(item):
 name,cmd=item;start=time.monotonic()
 with open(root/(name+'.log'),'w') as out:
  try:r=subprocess.run(cmd,cwd=src,stdout=out,stderr=subprocess.STDOUT,timeout=180);exit=r.returncode
  except subprocess.TimeoutExpired:exit='TIMEOUT'
 result={'name':name,'command':cmd,'exit':exit,'seconds':round(time.monotonic()-start,3)}
 print(json.dumps(result),flush=True);return result
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as ex:results=list(ex.map(run,checks))
json.dump(results,open(root/'focused-results.json','w'),indent=2)

