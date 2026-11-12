#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Test bounded production trace logic; not native audio/timing qualification."""
from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[2]
SOURCE=(ROOT/'firmware/src/app/engine.c').read_text()
DECL=SOURCE[SOURCE.index('#ifndef X0X_DIAG_STATE_TRACE'):SOURCE.index('/* ------------------------------------------------------- sequencer sink')]
HALF=SOURCE[SOURCE.index('#if X0X_DIAG_STATE_TRACE\n__attribute__((noinline, minsize)) void engine_diag_half'):SOURCE.index('void engine_render(int32_t *out_lr')]
class StateTraceTests(unittest.TestCase):
    def test_window_and_hash_reset(self):
        c='''#include <stdint.h>
#include <assert.h>
#define NTRACKS 5
struct { struct {uint32_t to_next,to_off;uint8_t pos;} t[5]; uint8_t ppat[5],playing;uint32_t mute;} seq;
static uint8_t eng_plan,eng_c1_mask,eng_808_split;
'''+DECL+HALF+'''
int main(void) {
#if X0X_DIAG_STATE_TRACE
    diag_event(123); engine_diag_half(199,0);
    assert(x0x_state_trace.written==0 && diag_count==0 && diag_events==0);
    seq.t[0].pos=3; seq.t[4].pos=7;seq.t[2].to_next=12345;
    diag_event(123); diag_event(456);diag_drums=5;diag_bass=789;
    eng_plan=3;eng_c1_mask=2;eng_808_split=1;
    engine_diag_half(200,4);
    assert(x0x_state_trace.written==1);
    assert(x0x_state_trace.row[0].half==200 && x0x_state_trace.row[0].late==4);
    assert(x0x_state_trace.row[0].count==2 && x0x_state_trace.row[0].events==((123u*33u)^456u));
    assert(x0x_state_trace.row[0].drums==5 && x0x_state_trace.row[0].bass==789);
    assert(x0x_state_trace.row[0].steps==(3u|(7u<<24)) && x0x_state_trace.row[0].plan==0x10203);
    assert(!diag_count && !diag_events && !diag_drums && !diag_bass);
    uint32_t hash=x0x_state_trace.row[0].state;
    engine_diag_half(201,4);assert(x0x_state_trace.row[1].state==hash);
    seq.t[2].to_next++;engine_diag_half(202,4);assert(x0x_state_trace.row[2].state!=hash);
    for (unsigned i=203;i<248;i++)engine_diag_half(i,4);
    assert(x0x_state_trace.written==48 && x0x_state_trace.row[47].half==247);
    engine_diag_half(248,5);engine_diag_half(1000,5);
    assert(x0x_state_trace.written==48 && x0x_state_trace.row[47].late==4);
#endif
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'test.c';p.write_text(c)
            for value in (None,'0','1'):
                args=[] if value is None else ['-DX0X_DIAG_STATE_TRACE='+value]
                subprocess.run(['cc','-O2','-Wno-attributes',*args,str(p),'-o',d+'/test'],check=True)
                subprocess.run([d+'/test'],check=True)
                if value!='1':
                    syms=subprocess.check_output(['nm',d+'/test'],text=True)
                    self.assertNotIn('x0x_state_trace',syms)
            bad=subprocess.run(['cc','-DX0X_DIAG_STATE_TRACE=2','-c',str(p),'-o',d+'/bad.o'],capture_output=True)
            self.assertNotEqual(bad.returncode,0)
    def test_decoder_bounds_and_fields(self):
        import importlib.util, struct
        spec=importlib.util.spec_from_file_location('trace_decode',ROOT/'tools/decode_state_trace.py')
        mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
        data=bytearray(1732)
        self.assertEqual(mod.decode(data)['written'],0)
        struct.pack_into('<I',data,0,1)
        struct.pack_into('<9I',data,4,200,2,3,4,5,1,0x017f3c00,0,7)
        row=mod.decode(data)['rows'][0]
        self.assertEqual(row['last_sequence_bass_request']['note'],60)
        self.assertEqual(row['drum_requests'],[(0,0)])
        with self.assertRaises(ValueError):mod.decode(data[:-1])
        struct.pack_into('<I',data,0,49)
        with self.assertRaises(ValueError):mod.decode(data)
        struct.pack_into('<I',data,0,1);struct.pack_into('<I',data,4,201)
        with self.assertRaises(ValueError):mod.decode(data)

    def test_one_hook_per_native_half_and_no_pool(self):
        main=(ROOT/'firmware/src/app/main_fm1.c').read_text()
        self.assertEqual(main.count('engine_diag_half(x0x_dbg.halves, x0x_dbg.late);'),1)
        self.assertNotIn('POOL',DECL)
        self.assertIn('volatile struct',DECL)
        build=(ROOT/'tools/build.py').read_text()
        self.assertIn('os.environ.get("X0X_DIAG_STATE_TRACE", "0")',build)
        self.assertIn('diagnostic_trace not in ("0", "1")',build)
if __name__=='__main__':unittest.main()
