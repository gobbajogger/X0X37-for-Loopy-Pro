import sys,pathlib,json,hashlib,importlib,datetime,os
sys.path.insert(0,'tools');import build as b
root=pathlib.Path.cwd();out=root/'build/cost-recovery';result={}
def sha(p):return hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()
for model,directory in [('elite','smk37-elite'),('pro','smk37')]:
 b=importlib.reload(b);b.BOARD='smk37';b.PRODUCT='SMK-37 '+('Elite' if model=='elite' else 'Pro')+'_900';b.BOARD_DEFS['smk37']=['-DBOARD_SMK37=1']+(['-DSMK_MODEL="SMK-37 Elite"'] if model=='elite' else []);b.fm1pkg_make.set_board('smk37');b.APP_SLOT=b.fm1pkg_make.APP_SLOT;b.APP_CODE_MAX=b.fm1pkg_make.APP_CODE_MAX;b.OUT=root/'build'/directory
 original_tc=b.tc;original_all=b.tc_all;record={'start':datetime.datetime.now(datetime.timezone.utc).isoformat(),'reuse':{},'commands':[]}
 before=sha(b.OUT/'o2/app_engine.o')
 def traced(tool,*args):
  if tool=='pi32v2/bin/ld':args=(*args,'-Map',out/(model+'.map'))
  record['commands'].append([tool,*map(str,args)])
  return original_tc(tool,*args)
 b.tc=traced
 def scoped(*cmds):
  if cmds and cmds[0][0]=='cc':
   for c in cmds:
    if any(str(x).endswith('app/engine.c') for x in c):b.tc(*c)
    else:record['reuse'][str(pathlib.Path(c[-1]).relative_to(root))]=sha(c[-1])
   return ['']*len(cmds)
  return original_all(*cmds)
 b.tc_all=scoped
 img,syms,dis,rt=b.build_app()
 assert sha(b.OUT/'o2/app_engine.o')==before,'spelling-only fence fix changed object unexpectedly'
 (out/(model+'-symbols.txt')).write_text(syms)
 hdr=b.tc('common/bin/objdump','-h',b.OUT/'x0x.elf');(out/(model+'-sections.txt')).write_text(hdr)
 errors,notes=b.check(img,syms,dis,rt);mmio=b.mmio_check();errors+=mmio
 record.update({'end':datetime.datetime.now(datetime.timezone.utc).isoformat(),'errors':errors,'notes':notes,'linkedChecks':'PASS' if not errors else 'FAIL','mmioErrors':mmio,'fenceObjectEquivalence':True,'sourceSha256':{n:sha(root/n) for n in ['firmware/src/app/engine.c','firmware/src/dsp/synth_engine.h','tools/build.py','experiments/wobble/wobble.c']},'outputs':{str(p.relative_to(root)):{'sha256':sha(p),'size':p.stat().st_size} for p in [b.OUT/'x0x.elf',b.OUT/'x0x.bin',b.OUT/'o2/app_engine.o',out/(model+'.map')]}})
 result[model]=record;(out/'final-native.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({model:{'errors':errors,'notes':notes,'fenceObjectEquivalence':True}},indent=2),flush=True)
 if errors:raise SystemExit(1)
