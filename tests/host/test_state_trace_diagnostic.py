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
#define TRK_DRUM 0
struct { struct {uint32_t to_next,to_off;uint8_t pos;} t[5]; uint8_t ppat[5],playing;uint32_t mute,clk_to_next;} seq;
static uint8_t eng_plan,eng_c1_mask,eng_808_split;
'''+DECL+HALF+'''
int main(void) {
#if X0X_DIAG_STATE_TRACE
    diag_event(123); engine_diag_half(173,0);
    assert(x0x_state_trace.written==0 && diag_count==0 && diag_events==0);
    seq.t[0].pos=3; seq.t[4].pos=7;seq.t[0].to_next=12345;seq.clk_to_next=54321;
    diag_prior_cost=111;diag_best_cost=99;diag_decision=0x1000201;
    diag_event(123); diag_event(456);diag_drums=5;diag_bass=789;
    eng_plan=3;eng_c1_mask=2;eng_808_split=1;
    engine_diag_half(174,4);
    assert(x0x_state_trace.written==1);
    assert(x0x_state_trace.row[0].half==174 && x0x_state_trace.row[0].late==4);
    assert(x0x_state_trace.row[0].count==2 && x0x_state_trace.row[0].events==((123u*33u)^456u));
    assert(x0x_state_trace.row[0].drums==5 && x0x_state_trace.row[0].clock_next==54321);
    assert(x0x_state_trace.row[0].steps==(3u|(7u<<24)) && x0x_state_trace.row[0].plan==0x10203);
    assert(!diag_count && !diag_events && !diag_drums && !diag_bass);
    uint32_t phase=x0x_state_trace.row[0].phase909;
    assert(phase==12345 && x0x_state_trace.row[0].prior_cost==111 && x0x_state_trace.row[0].best_cost==99 && x0x_state_trace.row[0].decision==0x1000201);
    engine_diag_half(175,4);assert(x0x_state_trace.row[1].phase909==phase);
    seq.t[0].to_next++;engine_diag_half(176,4);assert(x0x_state_trace.row[2].phase909!=phase);
    for (unsigned i=177;i<222;i++)engine_diag_half(i,4);
    assert(x0x_state_trace.written==48 && x0x_state_trace.row[47].half==221);
    engine_diag_half(222,5);engine_diag_half(1000,5);
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
    def test_actual_planner_capture_precedes_decision(self):
        planner=SOURCE[SOURCE.index('#define ENG_C1_LAT'):SOURCE.index('/* post-mix tail on both cores.')]
        pre="""#include <stdint.h>
#include <assert.h>
#define X0X_DIAG_STATE_TRACE 1
#define ORIGINAL_SOUND_MIAMI 1
enum {JB_909,JB_808,JB_A,JB_B,JB_BRK,JB_N};
static uint8_t eng_plan;
static uint32_t eng_job_cost[JB_N]={10000,10000,10000,10000,0};
static uint32_t diag_prior_cost,diag_best_cost,diag_decision;
static struct {int kind;} original_bass[2]={{1},{1}};
static int d909,d808,brk;
static int drum909_active(const void *p){(void)p;return 1;}
static int drum808_active(const void *p){(void)p;return 1;}
static int breaks_silent(const void *p){(void)p;return 1;}
"""
        main="""
int main(void) {
#ifdef X0X_CORE_PLAN
    diag_prior_cost=11;diag_best_cost=22;diag_decision=33;
    assert(plan_pick(256)==0);
    assert(diag_prior_cost==11 && diag_best_cost==22 && diag_decision==33);
#else
    eng_plan=0;uint32_t chosen=plan_pick(256);
    assert(chosen!=0 && (diag_decision&255)==0);
    assert(((diag_decision>>8)&255)==chosen && (diag_decision>>16)==256);
    assert(diag_prior_cost==40000 && diag_best_cost<diag_prior_cost);
    uint32_t old=eng_plan;plan_pick(256);
    assert(eng_plan==old && (diag_decision&255)==old && diag_best_cost==diag_prior_cost);
    int found=0;
    for(unsigned c=1000;c<=20000 && !found;c+=1000) {
        eng_job_cost[0]=c;
        for(unsigned m=1;m<16 && !found;m++) {
            eng_plan=m;plan_pick(256);
            if(diag_best_cost<diag_prior_cost && diag_best_cost*8u>=diag_prior_cost*7u) {
                assert(eng_plan==m && (diag_decision&255)==m);
                assert(diag_prior_cost==plan_cost(m,eng_job_cost));found=1;
            }
        }
    }
    assert(found);
#endif
    return 0;
}
"""
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'planner.c';p.write_text(pre+planner+main)
            for flags in ([],['-DX0X_CORE_PLAN=0']):
                subprocess.run(['cc','-O2',*flags,str(p),'-o',d+'/test'],check=True)
                subprocess.run([d+'/test'],check=True)

    def test_decoder_bounds_and_fields(self):
        import importlib.util, struct
        spec=importlib.util.spec_from_file_location('trace_decode',ROOT/'tools/decode_state_trace.py')
        mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
        data=bytearray(2308)
        self.assertEqual(mod.decode(data)['written'],0)
        struct.pack_into('<I',data,0,1)
        struct.pack_into('<12I',data,4,174,2,3,4,5,1,1234,0,7,111,99,0x1000201)
        row=mod.decode(data)['rows'][0]
        self.assertEqual(row['clock_samples_q8'],1234)
        self.assertEqual(row['planner_comparison'],{'prior_mask':1,'best_mask':2,'stretch_frames':256})
        self.assertEqual(row['drum_requests'],[(0,0)])
        with self.assertRaises(ValueError):mod.decode(data[:-1])
        struct.pack_into('<I',data,0,49)
        with self.assertRaises(ValueError):mod.decode(data)
        struct.pack_into('<I',data,0,1);struct.pack_into('<I',data,4,175)
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
