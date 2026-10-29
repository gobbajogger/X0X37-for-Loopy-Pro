#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Independent binary32 functional interpreter for Chicago public edits.
Uses libm's original fused operation, not the C implementation or its generated
private metadata. Includes every quantizer/lookup boundary and adjacent ULPs.
"""
import argparse,ctypes,ctypes.util,hashlib,json,pathlib,struct
PIN='1b57d7550e3738ef43746cea34e31185092609d1a0983ddbf7ff70bd2f287b23'
F=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]
U=lambda x:struct.unpack('<I',struct.pack('<f',x))[0]
B=lambda x:struct.unpack('<f',struct.pack('<I',x))[0]
lib=ctypes.CDLL(ctypes.util.find_library('m') or '/usr/lib/libSystem.B.dylib')
fma=lib.fmaf;fma.argtypes=[ctypes.c_float]*3;fma.restype=ctypes.c_float
def q(x,n):return min(n-1,max(0,int(fma(x,F(n-1),.5))))
def canonical(x,n):return F(q(x,n)/F(n-1))
def interpret(target,u,descriptors):
    i=target['target_id'];v=target['values'];typ=target['type_code']
    if i>120:return None
    if typ==2:y=v[q(u,len(v))]
    else:
        a,b=(v[1],v[0]) if typ==1 else (v[0],v[1])
        if typ==3:u=F(u*u)
        y=fma(u,F(b-a),a)
    if i>=109:return canonical(y,[30,2,128,128][(i-109)%4])
    d=descriptors[i];fmt=d['GUI_format']
    if fmt==18:return float(y>=.5)
    if fmt==19:return canonical(canonical(y,1001),16)
    n=1001 if fmt in (5,10) else 1281 if fmt==8 else 128 if fmt==2 else d['GUI_enum_count']
    assert n>1
    return canonical(y,n)
def generate(path,out):
    raw=path.read_bytes();assert hashlib.sha256(raw).hexdigest()==PIN
    data=json.loads(raw);ctx=data['all81_contexts'];norm=data['normalization']
    descriptors={d['native_target']:d for d in norm['target_descriptors']};count=0
    with out.open('wb') as dst:
        for macro in norm['all34_original_macro_target_records']:
            k=macro['macro_id'];values={0.0,1.0,.5}
            for n in (2,3,4,6,10,12,16,30,128,1001,1281):
                for i in range(n-1):
                    z=U(F((i+.5)/(n-1)))
                    values.update(B(j) for j in (z-1,z,z+1))
            values.update(F(i/127) for i in range(128))
            for u in sorted(values):
                f=(count*37+k*11)%81;s=ctx['presets'][f];native=[0.0]*117
                for i,x in ctx['common_nonmacro_core_values84'].items():native[int(i)]=x
                native[109:117]=s['FX1_normalized4']+s['FX2_normalized4']
                for i,x in s['canonical_native_target_values'].items():
                    if x is not None:native[int(i)]=x
                for target in macro['targets']:
                    x=interpret(target,u,descriptors)
                    if x is not None:native[target['target_id']]=x
                dst.write(struct.pack('<IIf117f',f,k,u,*native));count+=1
    print(json.dumps({'independent_public_edits':count,'each_edit_checks_full_native_fields':117,'reference_sha256':PIN,'vectors_sha256':hashlib.sha256(out.read_bytes()).hexdigest()},sort_keys=True))
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('reference',type=pathlib.Path);p.add_argument('out',type=pathlib.Path);a=p.parse_args();generate(a.reference,a.out)
