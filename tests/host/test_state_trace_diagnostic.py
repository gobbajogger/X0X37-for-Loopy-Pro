#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Focused production input-timing trace tests; not native timing proof."""
from pathlib import Path
import importlib.util
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[2]
SOURCE=(ROOT/'firmware/src/app/engine.c').read_text()
DECL=SOURCE[SOURCE.index('#ifndef X0X_DIAG_STATE_TRACE'):SOURCE.index('/* ------------------------------------------------------- sequencer sink')]
HALF=SOURCE[SOURCE.index('#if X0X_DIAG_STATE_TRACE\n__attribute__((noinline, minsize)) void engine_diag_half'):SOURCE.index('void engine_render(int32_t *out_lr')]
class InputTraceTests(unittest.TestCase):
    def test_window_default_and_decoder(self):
        pre="""#include <stdint.h>
#include <assert.h>
static uint32_t diag_consume,diag_command,diag_offset;
static uint32_t plat_diag_key_tick(unsigned n){return n==13?100:200;}
"""
        main="""
int main(void){
#if X0X_DIAG_STATE_TRACE
 diag_consume=345;diag_command=0x90102003;diag_offset=256;
 engine_diag_half(173,0);assert(!x0x_state_trace.written&&!diag_consume);
 diag_consume=346;engine_diag_half(174,1);
 assert(x0x_state_trace.row[0].key18_tick==100&&x0x_state_trace.row[0].key22_tick==200);
 assert(x0x_state_trace.row[0].consume_tick==346&&x0x_state_trace.row[0].command==0x90102003);
 assert(!diag_consume);
 for(unsigned h=175;h<222;h++)engine_diag_half(h,1);
 assert(x0x_state_trace.written==48&&x0x_state_trace.row[47].half==221);
 engine_diag_half(222,2);assert(x0x_state_trace.written==48);
#endif
 return 0;}
"""
        with tempfile.TemporaryDirectory() as d:
            f=Path(d)/'t.c';f.write_text(pre+DECL+HALF+main)
            for flag in ('0','1'):
                subprocess.run(['cc','-O2','-Wno-attributes','-DX0X_DIAG_STATE_TRACE='+flag,str(f),'-o',d+'/t'],check=True)
                subprocess.run([d+'/t'],check=True)
                if flag=='0':self.assertNotIn('x0x_state_trace',subprocess.check_output(['nm',d+'/t'],text=True))
            self.assertNotEqual(subprocess.run(['cc','-DX0X_DIAG_STATE_TRACE=2','-c',str(f),'-o',d+'/bad'],capture_output=True).returncode,0)
        import struct
        spec=importlib.util.spec_from_file_location('decode',ROOT/'tools/decode_state_trace.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
        buf=bytearray(1348);struct.pack_into('<I7I',buf,0,1,174,1,100,200,346,0x90102003,256)
        row=m.decode(buf)['rows'][0];self.assertEqual((row['cq_kit'],row['cq_voice'],row['cq_velocity12']),(1,2,3))
        struct.pack_into('<I',buf,4+4*4,0);self.assertIsNone(m.decode(buf)['rows'][0]['cq_voice'])
        with self.assertRaises(ValueError):m.decode(buf[:-1])
        struct.pack_into('<I',buf,0,49)
        with self.assertRaises(ValueError):m.decode(buf)
    def test_hook_scope(self):
        hal=(ROOT/'firmware/hal/smk_input.h').read_text()
        self.assertLess(hal.index('smk_diag_key_tick[k] = now;'),hal.index('smk__key_event(k, smk_vel7('))
        self.assertIn('diag_consume = plat_ticks24(); diag_command = c; diag_offset = engine_diag_render_offset;',SOURCE)
        ui=(ROOT/'firmware/src/app/ui.c').read_text();self.assertNotIn('engine_diag_key',ui)
    def test_platform_keybed_only_bounds(self):
        plat=(ROOT/'firmware/src/app/plat_fm1.c').read_text()
        f=plat[plat.index('uint32_t plat_diag_key_tick(unsigned note)'):plat.index('\n#endif\n#if BOARD_SMK37 && X0X_ORIGINAL_SYNTH')]
        code='''#include <stdint.h>
#include <assert.h>
#define BOARD_SMK37 1
#define SMK_KEY_F3 5
static struct {uint32_t knotes;} fm1_in;
static volatile uint32_t smk_diag_key_tick[37];
'''+f+'''\nint main(void){smk_diag_key_tick[18]=123;fm1_in.knotes=1u<<13;assert(plat_diag_key_tick(13)==123);assert(plat_diag_key_tick(27)==0);fm1_in.knotes=0;assert(plat_diag_key_tick(13)==0);return 0;}'''
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'test.c';p.write_text(code)
            subprocess.run(['cc','-O2',str(p),'-o',d+'/test'],check=True);subprocess.run([d+'/test'],check=True)
if __name__=='__main__':unittest.main()
