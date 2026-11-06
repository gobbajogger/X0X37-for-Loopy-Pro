import os, subprocess, pathlib, json, time, datetime, hashlib, statistics, csv
ROOT=pathlib.Path(os.environ['PERF_WORK_ROOT']).resolve()
OUT=ROOT/'evidence/diagnostic'
OUT.mkdir(exist_ok=True)
EXE={'A':ROOT/'evidence/baseline-build/test','B':ROOT/'evidence/candidate-build/test'}
cwd={'A':ROOT/'integration-baseline','B':ROOT/'candidate'}
meta={'baseline_commit':'48e92b76c2f476cf40771e00a04e1838136bb55d','baseline_tree':'00370c8cf385577395f616e9df3cbc45a0d23041','candidate_tree':'0260cd3f78a1b3717ef988d54aca767d65b6a2bb','status':'EXPLORATORY; provisional baseline; not release qualification','matrix':'unchanged tests/host/night_ui_bench.c: 15 cases, full_ms and steady_ms, 200 draws per measurement','core':2,'rounds':15,'clock':'CLOCK_PROCESS_CPUTIME_ID','same_binary_control':'AAAA using exact A executable; outer pair versus inner pair','classification':'Exploratory rule defined for this diagnostic, NOT recovered project rule: SAME if abs(median paired ABBA percent delta) <= max absolute per-round AAAA percent delta; else SLOWER if positive, FASTER if negative. SAME means unresolved within observed control envelope, not equivalence proof.','binary_sha256':{k:hashlib.sha256(v.read_bytes()).hexdigest() for k,v in EXE.items()}}
(OUT/'metadata.json').write_text(json.dumps(meta,indent=2)+'\n')
rows=[]
def run(label,phase,r,pos):
 name=f'{r:02}-{phase}-{pos}-{label}'
 start=datetime.datetime.now(datetime.timezone.utc).isoformat()
 p=subprocess.run(['taskset','-c','2',str(EXE[label])],cwd=cwd[label],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env={**os.environ,'SMC_UI_SHOTS':str(OUT)})
 (OUT/(name+'.log')).write_text(p.stdout)
 assert p.returncode==0,(name,p.returncode,p.stdout)
 vals={}
 for line in p.stdout.splitlines():
  if line.startswith('DRAW_COST '):
   fields=line.split(); case=fields[1]; data=dict(x.split('=') for x in fields[2:])
   assert data['static_blits']=='0' and data['frames']=='200'
   for metric in ('full_ms','steady_ms'):
    vals[case+'/'+metric]=float(data[metric])
 assert len(vals)==30,(name,len(vals))
 row={'round':r,'phase':phase,'position':pos,'binary':label,'start_utc':start,'exit':p.returncode,'values':vals}
 rows.append(row)
 with (OUT/'raw.jsonl').open('a') as f:f.write(json.dumps(row)+'\n')
 return vals
assert not (OUT/'raw.jsonl').exists(),'Refuse to mix prior raw run'
for label in 'AB':run(label,'warmup',0,0)
for r in range(1,16):
 for phase,seq in ([('ABBA','ABBA'),('AA','AAAA')] if r%2 else [('AA','AAAA'),('ABBA','ABBA')]):
  while os.getloadavg()[0]>=5:
   print('Waiting for load <5',os.getloadavg(),flush=True);time.sleep(5)
  gate={'round':r,'phase':phase,'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'loadavg':os.getloadavg()}
  with (OUT/'round-load.jsonl').open('a') as f:f.write(json.dumps(gate)+'\n')
  for pos,label in enumerate(seq):run(label,phase,r,pos)
 print('Completed round',r,flush=True)
keys=rows[0]['values'].keys(); summary=[]
for key in keys:
 aa=[];delta=[];a=[];b=[]
 for r in range(1,16):
  ab=[x['values'][key] for x in rows if x['round']==r and x['phase']=='ABBA']
  ctl=[x['values'][key] for x in rows if x['round']==r and x['phase']=='AA']
  av=(ab[0]+ab[3])/2; bv=(ab[1]+ab[2])/2
  a.append(av);b.append(bv);delta.append((bv/av-1)*100)
  aa.append(((ctl[1]+ctl[2])/(ctl[0]+ctl[3])-1)*100)
 def spread(v):return {'median':statistics.median(v),'min':min(v),'max':max(v),'iqr':statistics.quantiles(v,n=4,method='inclusive')[2]-statistics.quantiles(v,n=4,method='inclusive')[0]}
 d=statistics.median(delta);noise=max(map(abs,aa)); cls='SAME' if abs(d)<=noise else ('SLOWER' if d>0 else 'FASTER')
 summary.append({'case':key,'baseline_ms':spread(a),'candidate_ms':spread(b),'paired_delta_percent':spread(delta),'aa_delta_percent':spread(aa),'aa_max_abs_percent':noise,'diagnostic_class':cls,'paired_deltas':delta,'aa_deltas':aa})
(OUT/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
with (OUT/'summary.csv').open('w') as f:
 w=csv.writer(f);w.writerow(['case','A_median_ms','A_min_ms','A_max_ms','A_IQR_ms','B_median_ms','B_min_ms','B_max_ms','B_IQR_ms','median_paired_delta_pct','AA_max_abs_pct','diagnostic_class'])
 for s in summary:w.writerow([s['case'],*[s['baseline_ms'][k] for k in ('median','min','max','iqr')],*[s['candidate_ms'][k] for k in ('median','min','max','iqr')],s['paired_delta_percent']['median'],s['aa_max_abs_percent'],s['diagnostic_class']])
print(json.dumps({c:sum(s['diagnostic_class']==c for s in summary) for c in ['FASTER','SAME','SLOWER']}),flush=True)
