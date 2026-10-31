#!/usr/bin/env python3
"""Scratch-only fake builds; real runner/record/guard, never flashable firmware."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import unittest

from test_dev_validate import DevelopmentValidation, ROOT, GUARD
sys.path.insert(0, str(ROOT/'tools'))
import validation_inputs as vi
from fm1pkg_make import ufw

RUNNER = ROOT/'tools/dev_validate_composed.py'


class ComposedValidation(unittest.TestCase):
    git = DevelopmentValidation.git

    def setUp(self):
        DevelopmentValidation.setUp(self)
        build = self.repo/'build.sh'
        build.write_text(build.read_text().replace(chr(0), r"\x00")+"\nmkdir -p build/smk37-elite\nprintf app > build/smk37-elite/x0x.bin\n")
        ctl = self.repo/'ble/ctl'
        ctl.mkdir(parents=True)
        (ctl/'build_ctl.sh').write_text('mkdir -p ble/ctl/out\nprintf controller > ble/ctl/out/ctl.img\n')
        tools = self.repo/'tools'
        tools.mkdir()
        # Deliberately fake composition. These tests validate orchestration only.
        (tools/'package_nightly.py').write_text('''import sys,shutil
if '-o' in sys.argv: shutil.copyfile(sys.argv[1],sys.argv[sys.argv.index('-o')+1])
print('fixture package framing checked')
''')
        self.composer = self.root/'composer/tools/package_stock_loops.py'
        self.composer.parent.mkdir(parents=True)
        self.composer.write_text('''import sys,shutil,json
shutil.copyfile(sys.argv[1],sys.argv[sys.argv.index('--output')+1])
open(sys.argv[sys.argv.index('--receipt')+1],'w').write('{}')
print('fixture composition checked')
''')
        self.bank = self.composer.parent.parent/'assets/factory-breaks/factory-break-pack.bin'
        self.bank.parent.mkdir(parents=True)
        self.bank.write_bytes(b'fake stock')
        self.git('add', '.')
        self.git('-c','user.name=t','-c','user.email=t@t','commit','-qm','composed fixture')
        self.commit = self.git('rev-parse','HEAD')
        raw = subprocess.check_output([sys.executable, '-c',
            "import sys; sys.path.insert(0,sys.argv[1]); from fm1pkg_make import ufw; "
            "sys.stdout.buffer.write(ufw(bytes(1024),b'FELUCCA-LOADER-1'+bytes(128),'SMK-37 Elite_900'))",
            str(ROOT/'tools')])
        h = lambda b: hashlib.sha256(b).hexdigest()
        self.data = {'base':self.commit,'scope':'composed orchestration fixture',
                     'checks':[{'reason':'fixture test','argv':[sys.executable,'test.py']}],
                     'composition':{'app_sha256':h(b'app'),'raw_package_sha256':h(raw),
                         'controller_sha256':h(b'controller'),'controller_bytes':10,
                         'bundled_package_sha256':h(raw),'package_sha256':h(raw),
                         'composer':str(self.composer),'dependency_pins':{
                             str(p):vi.content(p)['sha256'] for p in (self.composer.parent,self.bank)}}}
        self.write_plan()

    def write_plan(self):
        self.plan.write_text(json.dumps(self.data))

    def run_it(self):
        return subprocess.run([sys.executable,str(RUNNER),'--repo',str(self.repo),
            '--commit',self.commit,'--model','elite','--plan',str(self.plan),
            '--output',str(self.out)],env=self.env,capture_output=True,text=True)

    def guard(self,*extra):
        return subprocess.run(['bash',str(GUARD),self.commit,'--fwsc',str(self.out/'composed-final.fwsc'),
            '--ctl',str(self.out/'ctl.img'),'--dev-record',str(self.out/'record.json'),*extra],
            env=self.env,capture_output=True,text=True)

    def ready(self):
        r=self.run_it();self.assertEqual(r.returncode,0,r.stdout+r.stderr)

    def test_real_record_and_guard_preserve_queue_release_and_hash_guards(self):
        self.ready()
        record=json.loads((self.out/'record.json').read_text())
        self.assertEqual(set(record['logs']),{'build','controller','check-0','check-1','check-2','check-3'})
        self.assertEqual(self.guard().returncode,0)
        self.assertEqual(self.guard('--config','rel').returncode,3)
        self.queue.write_text('- [ ] missing pending\n')
        self.assertEqual(self.guard().returncode,3)
        self.queue.write_text('')
        for name in ('build.log','controller.log','check-2.log','ctl.img','composed-final.fwsc'):
            p=self.out/name;old=p.read_bytes();p.write_bytes(old+b'tampered')
            self.assertEqual(self.guard().returncode,3,name);p.write_bytes(old)
        self.assertEqual(self.guard().returncode,0)

    def test_each_output_pin_refuses_instead_of_substituting(self):
        for key in ('app_sha256','raw_package_sha256','controller_sha256','bundled_package_sha256','package_sha256'):
            with self.subTest(key=key):
                old=self.data['composition'][key];self.data['composition'][key]='0'*64;self.write_plan()
                r=self.run_it();self.assertEqual(r.returncode,3,r.stdout+r.stderr)
                self.assertFalse((self.out/'record.json').exists())
                self.data['composition'][key]=old
                if self.out.exists():shutil.rmtree(self.out)

    def test_stock_dependency_drift_refused(self):
        self.bank.write_bytes(b'changed')
        r=self.run_it();self.assertEqual(r.returncode,3);self.assertFalse((self.out/'record.json').exists())

    def test_check_failure_has_no_record(self):
        self.data['checks'][0]['argv']=[sys.executable,'-c','raise SystemExit(9)'];self.write_plan()
        r=self.run_it();self.assertEqual(r.returncode,3);self.assertFalse((self.out/'record.json').exists())

    def test_tracked_source_mutation_has_no_record(self):
        self.data['checks'][0]['argv']=[sys.executable,'-c',"open('test.py','w').write('changed')"];self.write_plan()
        r=self.run_it();self.assertEqual(r.returncode,3);self.assertFalse((self.out/'record.json').exists())

    def test_sdk_drift_during_execution_has_no_record(self):
        self.data['checks'][0]['argv']=[sys.executable,'-c',f"open({str(self.sdk/'library')!r},'w').write('changed')"];self.write_plan()
        r=self.run_it();self.assertEqual(r.returncode,3);self.assertFalse((self.out/'record.json').exists())

    def test_controller_size_refused(self):
        self.data['composition']['controller_bytes']=11;self.write_plan()
        r=self.run_it();self.assertEqual(r.returncode,3);self.assertFalse((self.out/'record.json').exists())

    def test_composer_dependency_drift_refused(self):
        self.composer.write_text(self.composer.read_text()+'# changed\n')
        r=self.run_it();self.assertEqual(r.returncode,3);self.assertFalse((self.out/'record.json').exists())

    def test_unknown_base_refused(self):
        self.data['base']='f'*40;self.write_plan()
        r=self.run_it();self.assertEqual(r.returncode,3);self.assertFalse((self.out/'record.json').exists())

    def test_source_executable_mode_drift_refused(self):
        self.data['checks'][0]['argv']=[sys.executable,'-c',"import os; os.chmod('test.py',0o755)"];self.write_plan()
        r=self.run_it();self.assertEqual(r.returncode,3);self.assertFalse((self.out/'record.json').exists())

    def test_existing_output_is_never_overwritten(self):
        self.out.mkdir();(self.out/'keep').write_text('unchanged')
        r=self.run_it();self.assertEqual(r.returncode,3);self.assertEqual((self.out/'keep').read_text(),'unchanged')


if __name__=='__main__':unittest.main()
