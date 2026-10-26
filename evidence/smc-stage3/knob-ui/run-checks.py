from pathlib import Path
import subprocess,json,sys,platform
out=Path('evidence/smc-stage3/knob-ui')
checks=[
('knob-mapping',['python3','tests/host/smc_mapping_test.py'],'three intended knob modes, eleven tracks, bank bounds, unique fields and menu priority'),
('pickup',['python3','tests/host/smc_fader_test.py'],'fader pickup, crossing, motion and inactive input preservation'),
('buttons',['python3','tests/host/smc_buttons_test.py'],'strip shortcuts and menu/page navigation preservation'),
('filter',['python3','tests/host/smc_filter_test.py'],'master filter destination and independent master mute feedback'),
('clip-visibility',['sh','tests/host/smc_clip_visibility_test.sh'],'clip/menu/help/confirmation priority and ordinary field suppression'),
('part-selection',['sh','tests/host/smc_part_select_test.sh'],'part selection/navigation preservation'),
('reachability',['sh','tests/host/smc_reachability_test.sh'],'complete production UI/engine/queue/pickup with five injected or synthetic routes')]
report={'candidate':subprocess.check_output(['git','-c','safe.directory='+str(Path.cwd()),'rev-parse','HEAD'],text=True).strip(),'platform':platform.platform(),'checks':[]}
for name,argv,reason in checks:
    with (out/(name+'.log')).open('w') as f:
        p=subprocess.run(argv,stdout=f,stderr=subprocess.STDOUT)
    report['checks'].append({'name':name,'argv':argv,'reason':reason,'returncode':p.returncode,'result':'PASS' if p.returncode==0 else 'FAIL','log':name+'.log'})
    print(name+': '+report['checks'][-1]['result'],flush=True)
(out/'checks.json').write_text(json.dumps(report,indent=2)+'\n')
sys.exit(any(x['returncode'] for x in report['checks']))
