"""Extract public per-note DSP unchanged; generated adapter is an isolated probe.
Not complete engine: no sequencer, note allocator, global FX, parameter ramps,
AU, UI, persistence, or device integration. Supplied preset values stay local.
"""
from pathlib import Path
import json,re,struct,zipfile,hashlib,os
P=Path(__file__).resolve().parent
U=Path(os.environ['SYNTHONE_SOURCE']).resolve()
(P/'host').mkdir(exist_ok=True)
E=(U/'DSP/S1Parameter.h').read_text();E=re.sub(r'#import[^\n]*','',E).replace('typedef AK_ENUM(S1Parameter)','enum S1Parameter').replace('} S1Parameter;','};')
K=(U/'DSP/Kernel/S1DSPKernel.hpp').read_text();body=K.split('S1ParameterInfo s1p[S1Parameter::S1ParameterCount] = {')[1].split('\n    };')[0]
rows=re.findall(r'^\s*\{\s*([^,]+),\s*([^,]+),\s*([^,]+),\s*([^,]+),',body,re.M)
assert len(rows)==150,len(rows)
env={'rate_min':1/1920,'rate_max':80,'bpm_min':1,'bpm_max':200,'bars_min':1/96,'bars_max':8,'S1_NUM_BANDLIMITED_FTABLES':13}
# rate_max = 1/((4*(1/64/1.5))/(200/60)) = 80 Hz
vals=[[float(eval(s,{'__builtins__':{}},env)) for s in row[1:]] for row in rows]
head='''// Generated from pinned AudioKit public source. MIT notices in ../notices/SynthOne-MIT.txt.
#pragma once
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <stdint.h>
#include <stddef.h>
extern "C" {
#include "soundpipe.h"
#include "soundpipeextension.h"
}
#define nil nullptr
#define S1_NUM_WAVEFORMS 4
#define S1_NUM_BANDLIMITED_FTABLES 13
static inline float pow2(float x){return x*x;}
template<typename T>static inline T clamp(T x,T a,T b){return x<a?a:x>b?b:x;}
'''+E+'\n'
for i,name in enumerate(['minimums','defaults','maximums']):head+='static const float '+name+'[150]={'+','.join(format(v[i],'.9g')+'f' if '.' in format(v[i],'.9g') or 'e' in format(v[i],'.9g') else format(v[i],'.9g')+'.0f' for v in vals)+'};\n'
head+='''class S1DSPKernel {
public:
 float parameters[150];sp_data*sp;sp_ftbl*ft_array[52];float ft_frequencyBand[13];sp_ftbl*sine;
 float lfo1_0_1=0,lfo1_1_0=1,lfo2_0_1=0,lfo2_1_0=1,lfo3_0_1=0,lfo3_1_0=1,monoFrequencySmooth=261.6255653006f;
 sp_phasor*lfo1Phasor,*lfo2Phasor;sp_port*lfo1Port,*lfo2Port;float lfo1Smooth=0,lfo2Smooth=0;
 sp_data*spp(){return sp;}int sampleRate()const{return sp->sr;}
 float clampedValue(S1Parameter p,float x){return clamp(x,minimums[p],maximums[p]);}
 void processLfo();
};
'''
N=(U/'DSP/Note State/S1NoteState.hpp').read_text();head+=N[N.index('struct S1NoteState {'):N.rindex('};')+2]+'\n'
(P/'portable_interface.hpp').write_text(head)
source=(U/'DSP/Note State/S1NoteState.mm').read_text();source=re.sub(r'^#import[^\n]*\n','',source,flags=re.M)
process=(U/'DSP/Kernel/S1DSPKernel+process.mm').read_text().split('        //MARK: LFO\n')[1].split('        /// MARK: ARPEGGIATOR')[0]
(P/'portable_voice.cpp').write_text('#include "portable_interface.hpp"\n'+source+'\nvoid S1DSPKernel::processLfo(){\n'+process+'\n}\n')
wave=U/'DSP/BandlimitedWavetables'; files=json.loads((wave/'bandlimitedWaveforms.json').read_text());out=[]
for f in files:
 path=next(wave.rglob(f+'.json'));a=json.loads(path.read_text())['content'];assert len(a)==4096;out.extend(a)
freq=json.loads((wave/'bandlimitedWaveformFrequencies.json').read_text())['content'];assert len(freq)==13
(P/'host/wavetables.f32').write_bytes(struct.pack('<%df'%len(out),*out))
(P/'wave_frequencies.h').write_text('static const float wave_frequencies[13]={'+','.join(str(x)+'f' if isinstance(x,float) else str(x)+'.0f' for x in freq)+'};\n')
# Preset field names must come from the actual activePreset -> parameter setter.
manager=(U/'Presets/PresetDataManager.swift').read_text().split('    func saveValuesToPreset()')[0]
mapping=re.findall(r's\.setSynthParameter\(\.([A-Za-z0-9_]+),\s*(?:Double\()?activePreset\.([A-Za-z0-9_]+)',manager)
indices={k:int(v) for k,v in re.findall(r'(\w+)\s*=\s*(\d+)',E)}
archive=Path(os.environ['SYNTHONE_PRESETS_ARCHIVE']).resolve()
expected_archive='68ad8b5fa0adca8b5da2b03f9303e9321c6a20907f5945e3fe879bc6494009a4'
if hashlib.sha256(archive.read_bytes()).hexdigest()!=expected_archive:
 raise ValueError('Synth One preset archive does not match the pinned input')
z=zipfile.ZipFile(archive);records=[];bank_counts={}
for name in z.namelist():
 if not name.endswith('.json') or '__MACOSX' in name:continue
 presets=json.loads(z.read(name));bank_counts[name]=len(presets)
 for p in presets:
  v=[r[1] for r in vals]
  for param,key in mapping:
   if key in p:
    i=indices[param];v[i]=max(vals[i][0],min(float(p[key]),vals[i][2]))
  records.append(v)
(P/'host/preset-params.f32').write_bytes(struct.pack('<%df'%(len(records)*150),*[v for r in records for v in r]))
(P/'voice-probe-inputs.json').write_text(json.dumps({'synthone_sha':'6466a3715c96b7ecf1dd255a218cbd571408a314','archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'banks':bank_counts,'records':len(records),'source_setter_mappings':len(mapping),'scope':'static control-value per-note and LFO probe; no global FX, smoothing, sequencer or AU'},indent=2)+'\n')
print('generated',len(rows),'parameter rows',len(records),'owner records',len(out),'public wavetable samples')
