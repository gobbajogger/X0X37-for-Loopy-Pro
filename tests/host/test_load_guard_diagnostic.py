#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Focused compile/run tests of the actual engine guard, not native audio proof."""
import ast
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SOURCE = (ROOT / 'firmware/src/app/engine.c').read_text()
GUARD = SOURCE[SOURCE.index('#ifndef X0X_DIAG_DISABLE_LOAD_GUARD'):SOURCE.index('/* mixer: per part')]
INIT = SOURCE[SOURCE.index('    guard_on = guard_applied = 0;'):SOURCE.index('    smc_solo_state = 0;')]

class LoadGuardDiagnosticTests(unittest.TestCase):
    def test_release_and_diagnostic_state_machine(self):
        harness = '''
#include <stdint.h>
#include <assert.h>
float drum808_quiet;
''' + GUARD + '\nstatic void reset(void) {\n' + INIT + '''\n}
int main(void) {
    reset();
    assert(guard_enabled == !X0X_DIAG_DISABLE_LOAD_GUARD);
    engine_guard_enable(1);
    engine_load(100, 512); guard_apply();
#if X0X_DIAG_DISABLE_LOAD_GUARD
    assert(!guard_enabled && !guard_on && !guard_applied && eng_guard_count == 0);
    for (int i=0; i<1000; ++i) {engine_guard_enable(1); engine_load(200,512); guard_apply();}
    assert(!guard_on && !eng_guard_count && drum808_quiet == 3.2e-5f);
#else
    assert(guard_on && eng_guard_count == 1 && drum808_quiet == 3.2e-3f);
    for (int i=0; i<2000; ++i) {engine_load(0,512); guard_apply();}
    assert(!guard_on && drum808_quiet == 3.2e-5f);
    engine_load(100,512); guard_apply(); assert(guard_on);
    engine_guard_enable(0); engine_load(100,512); guard_apply();
    assert(!guard_on && drum808_quiet == 3.2e-5f);
#endif
    reset(); assert(guard_enabled == !X0X_DIAG_DISABLE_LOAD_GUARD);
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as d:
            src=Path(d)/'guard.c'; src.write_text(harness)
            for value in (None,'0','1'):
                with self.subTest(value=value):
                    args=[] if value is None else ['-DX0X_DIAG_DISABLE_LOAD_GUARD='+value]
                    subprocess.run(['cc','-std=c99','-O2','-Wall','-Werror',*args,str(src),'-o',d+'/test'],check=True)
                    subprocess.run([d+'/test'],check=True)
            bad=subprocess.run(['cc','-DX0X_DIAG_DISABLE_LOAD_GUARD=2','-c',str(src),'-o',d+'/bad.o'],capture_output=True)
            self.assertNotEqual(bad.returncode,0)
            self.assertIn(b'must be 0 or 1',bad.stderr)

    def test_build_flag_is_opt_in_and_validated(self):
        # Execute the real build_app prelude only; no toolchain or SDK needed.
        module=ast.parse((ROOT/'tools/build.py').read_text())
        func=next(n for n in module.body if isinstance(n,ast.FunctionDef) and n.name=='build_app')
        start=next(i for i,n in enumerate(func.body) if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='diagnostic_guard' for t in n.targets))
        code=compile(ast.Module(body=func.body[start:start+3],type_ignores=[]),'<build guard flag>','exec')
        from unittest.mock import patch
        for value, expected in ((None,[]),('0',[]),('1',['-DX0X_DIAG_DISABLE_LOAD_GUARD=1'])):
            env={} if value is None else {'X0X_DIAG_DISABLE_LOAD_GUARD':value}
            with patch.dict(os.environ,env,clear=True):
                ns={'os':os,'flags':[]}; exec(code,ns); self.assertEqual(ns['flags'],expected)
        with patch.dict(os.environ,{'X0X_DIAG_DISABLE_LOAD_GUARD':'bad'},clear=True):
            with self.assertRaises(SystemExit): exec(code,{'os':os,'flags':[]})

if __name__ == '__main__': unittest.main()
