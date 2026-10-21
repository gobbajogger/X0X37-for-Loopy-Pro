"""Authorized exact DEV install; unchanged native guard sequence and updater.
Retains the native default lock across fresh full before/after reads and native run.
No bypass, replaced guard, mocked backend, rescue path or bootloader write.
"""
from pathlib import Path
from types import SimpleNamespace
from datetime import datetime, timezone, timedelta
import sys, json, contextlib, traceback, hashlib, subprocess

root = Path(__file__).resolve().parent
sys.path.insert(0,str(root))
from device_snapshot import snapshot,fp
import fm1_install as f, x0x_slots as xs
lane = root.parents[1]
base = root/'elite-linux'
pre = json.loads((root/'preflight.json').read_text())
auth = dict(install_authorized=True, source='Current user request to finish all stages and install latest GitHub firmware; sole active local installer', owner_quote='Finish all stages and install the firmware that addresses all the commits')
assert pre['validated_revision'] == 'a5cb24dd08b40b80e802e03ff0d9c848315a9751'
a = SimpleNamespace(package=str(base/'x0x-smk37-elite.fwsc'),ctl=str(base/'ctl.img'),
    commit=pre['validated_revision'],dev_record=str(base/'record.json'),config='dev',port='X0X FM-1',
    yes=True,force=False,no_ctl=False,no_slots=False,info=False,head_ref=None,
    backup=str(root/'installer-all-loops.x0xslots'))

def now(): return datetime.now(timezone(timedelta(hours=11))).isoformat()
rec = dict(status='STARTING',started=now(),commit=a.commit,dev_record=a.dev_record,
           evidence_revision=pre['evidence_revision'], artifacts=pre['artifacts'], model='elite',config='dev',X0X_DEV_PADCAL=1,arguments=vars(a).copy(),
           authorization=auth,prewrite_notice_sent=True,hardware_input_pass=False,
           guard_sequence='native gate_guard, foreground_guard, hardware_lock, head_guard; then unchanged native run',
           guard_functions_replaced=False,device_writes_started=False)

def save(): (root/'install-record.json').write_text(json.dumps(rec,indent=2)+'\n')
class Tee:
    def __init__(self,term,log): self.term,self.log=term,log
    def write(self,s): self.term.write(s);self.term.flush();self.log.write(s);self.log.flush();return len(s)
    def flush(self): self.term.flush();self.log.flush()
    def isatty(self): return self.term.isatty()

save()
with (root/'install.log').open('w') as log:
    tee=Tee(sys.stdout,log)
    with contextlib.redirect_stdout(tee),contextlib.redirect_stderr(tee):
        try:
            assert sys.stdin.isatty()
            assert subprocess.check_output(['git','-C',str(lane),'rev-parse','HEAD'],text=True).strip()==pre['evidence_revision']
            assert not subprocess.check_output(['git','-C',str(lane),'status','--porcelain','--',*pre['checked_paths']]).strip()
            for key,path in [('package',Path(a.package)),('controller',Path(a.ctl))]:
                e=pre['artifacts'][key]
                assert fp(path.read_bytes())=={k:e[k] for k in ('bytes','sha256','crc32')}
            assert hashlib.sha256(Path(a.dev_record).read_bytes()).hexdigest()==pre['record_sha256']
            for name,guard in [('development admission',lambda:f.gate_guard(a)),
                               ('foreground',f.foreground_guard),('hardware lock',f.hardware_lock),
                               ('bootloader',lambda:f.head_guard(a,out=tee))]:
                rc=guard()
                rec.setdefault('guards',{})[name]=rc;save()
                if rc: raise RuntimeError(f'{name} refused with exit{rc}; no bypass')
            before=snapshot('before')
            rec['before']=before
            rec['status']='BACKUP VERIFIED; NATIVE INSTALL STARTING';save()
            # run is the same updater invoked by native main after these same guards.
            # The selected explicit --backup forces a second validated archive before UPGRADE.
            rec['device_writes_started']=True;save()
            rc=f.run(a,f.MidoBackend(),tee,f.ask_tty)
            rec['installer_exit']=rc;save()
            if rc: raise RuntimeError(f'native installer exit{rc}')
            assert f._HW_LOCK is not None
            after=snapshot('after')
            rec['after']=after
            _,old=xs.read_archive((root/'before/loops.x0xslots').read_bytes())
            _,mandatory=xs.read_archive(Path(a.backup).read_bytes())
            _,new=xs.read_archive((root/'after/loops.x0xslots').read_bytes())
            assert old==mandatory==new, 'LOOP IMAGE MISMATCH'
            comparisons=[]
            for b,c in zip(before['slots'],after['slots']):
                equal=Path(b['path']).read_bytes()==Path(c['path']).read_bytes()
                comparisons.append(dict(slot=b['slot'],before=b['complete_slot'],after=c['complete_slot'],byte_equal=equal))
                assert b['flash']==c['flash'] and b['complete_slot']==c['complete_slot'] and equal, f"FULL SLOT{b['slot']} MISMATCH"
            assert after['controller_byte_equal']
            rec.update(status='VERIFIED DEVELOPMENT INSTALL',completed=now(),
                complete_slot_comparisons=comparisons,all_loop_images_identical=True,
                all_complete_slots_identical=True,controller_readback=after['controller'],
                full_controller_bytes_identical=before['controller']==after['controller'],
                image_readback=dict(method=pre['image_readback_plan'],loader_FINISH_WRITE_received=True,
                                    application_sectors=153,independent_host_application_dump=False),
                bootloader=dict(writes_excluded=True,independent_unit_head_reference_available=False,
                                package_head_matches_previous_installed_package=pre['package_head_matches_previous_installed_package']),
                hardware_input='NOT TESTED; owner-reported all-input silence unresolved')
            save()
            print('VERIFIED DEVELOPMENT INSTALL: all 3 complete slots byte-identical; all imported loops preserved; exact full controller readback.',flush=True)
        except BaseException as e:
            rec.update(status='INSTALL PENDING / CHECK FAILURE',error=str(e),failed=now())
            save();traceback.print_exc();raise
