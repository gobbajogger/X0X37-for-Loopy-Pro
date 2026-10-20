"""Record verified installation and data preservation; never accesses a device."""
from pathlib import Path
import hashlib,json,zlib
out=Path(__file__).resolve().parent
r=json.loads((out/'install-record.json').read_text())
p=json.loads((out/'preflight.json').read_text())
assert r['status']=='VERIFIED DEVELOPMENT INSTALL'
assert r['installer_exit']==0 and all(v==0 for v in r['guards'].values())
assert r['all_complete_slots_identical'] and r['all_loop_images_identical']
assert all(x['byte_equal'] and x['before']==x['after'] for x in r['complete_slot_comparisons'])
assert r['after']['controller_byte_equal']
assert r['controller_readback']==p['artifacts']['controller_full_64k']
def fp(path):
 b=Path(path).read_bytes();return {'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest(),'crc32':f'{zlib.crc32(b):08x}'}
archives={phase:fp(out/phase/'loops.x0xslots') for phase in ['before','after']}
archives['native_installer']=fp(out/'installer-all-loops.x0xslots')
ledger={'result':'INSTALL AND DATA CHECKS PASS','source_revision':r['commit'],'validated_configuration':{'board':'smk37','model':'elite','config':'dev','X0X_DEV_PADCAL':1},'owner_thread':'01a15b38-73dd-7363-9646-99f95a8f770c','source':'Latest private GitHub main a5cb24d confirmed by API and git ls-remote before validation.','started':r['started'],'completed':r['completed'],'dev_record':r['dev_record'],'validation_input_sha256':p['artifacts']['validation_input_sha256'],'artifacts':p['artifacts'],'fresh_artifacts_equal_previous_install':p['fresh_artifact_comparison'],'guards':r['guards'],'bootloader':r['bootloader'],'image_readback':r['image_readback'],'controller_readback':r['controller_readback'],'slots':r['complete_slot_comparisons'],'all_loop_data_preserved':True,'imported_loops':sum(x['nz'] for x in r['before']['layout']['slots']),'archives':archives,'archive_paths':'Archives and complete binary snapshots remain local beside this ledger and are excluded from commits.','physical_behavior':'NOT TESTED','original_input_failure':'UNRESOLVED','limits':['Native loader verifies application sectors before FINISH_WRITE; no independent host application dump is available.','No independent unit bootloader-head reference is available. The package head matches the prior installed artifact; loader writes exclude the bootloader.','Installation and readback do not establish physical input behavior.'],'attempts':{'native_worker':'Initial evidence check refused before target build; recorded in attempt-1.json. Separate user-requested repair thread started.','direct_linux':'DEV CHECKED with actual app/controller builds and before/after input identity.'},'repair_thread':json.loads((out/'native-repair-successor.json').read_text()),'release':False,'full_gate':False,'source_repairs':False,'remaining_install_work':None}
(out/'install-ledger.json').write_text(json.dumps(ledger,indent=2)+'\n')
print('PASS: exact DEV install, full controller readback, three complete slots identical,',ledger['imported_loops'],'loops preserved.')
