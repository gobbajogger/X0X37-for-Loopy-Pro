#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Shared exact Chicago context from the pinned functional inventory only.

No component binaries, raw disassembly, opaque native banks or SDK input.
The golden vectors are captured original saved values, not a replay of C map.
"""
import argparse, hashlib, json, pathlib, struct
PIN='1b57d7550e3738ef43746cea34e31185092609d1a0983ddbf7ff70bd2f287b23'
def literal(x):
    return struct.unpack('<f',struct.pack('<f',x))[0].hex()+'f'
def generate(reference,header,fixture,expanded=False):
    raw=reference.read_bytes()
    if hashlib.sha256(raw).hexdigest()!=PIN:raise ValueError('Chicago inventory pin mismatch')
    src=json.loads(raw);norm=src['normalization'];ctx=src['all81_contexts'];presets=ctx['presets']
    descriptors={d['native_target']:d for d in norm['target_descriptors']}
    ignored=set(norm['ignored_targets_in_current_original_CMP4_process'])
    assert ignored=={128,129,131,132,133} and len(presets)==81
    counts={53:10,54:16,60:6,61:4,62:2,107:2,108:128,24:1281}
    rows,look,ids,starts=[],[],[],[0]
    for c in norm['all34_original_macro_target_records']:
        assert c['macro_id']==len(starts)-1
        for t in c['targets']:
            i=t['target_id']
            if i in ignored:continue
            v=t['values'];typ=t['type_code'];offset=len(look)
            assert typ in (0,1,2,3)
            if typ==2:look+=v;a=b=0
            else:assert len(v)==2;a,b=v
            n=counts.get(i,1001) if i<109 else [30,2,128,128][(i-109)%4]
            rows.append('{%d,%d,%d,%d,%d,%d,%s,%s}'%(i,typ,len(v),offset,n,1001 if i==54 else 0,literal(a),literal(b)))
            ids.append(i)
        starts.append(len(rows))
    assert len(ids)==len(set(ids))==29 and sum(i<109 for i in ids)==25
    base=[0.0]*117
    for i,x in ctx['common_nonmacro_core_values84'].items():base[int(i)]=x
    assert len(ctx['common_nonmacro_core_values84'])==84
    base[109:117]=presets[0]['FX1_normalized4']+presets[0]['FX2_normalized4']
    lines=['/* PRIVATE functional Chicago metadata; inactive experimental backend. */',
      'typedef struct {uint8_t native_id,type,length,lookup;uint16_t canonical_count,outer_count;float a,b;} original_chicago_mapping;',
      'static const uint8_t original_chicago_macro_start[23]={'+','.join(map(str,starts))+'};',
      'static const original_chicago_mapping original_chicago_mappings[29]={'+',\n'.join(rows)+'};',
      'static const float original_chicago_lookup[%d]={'%len(look)+','.join(map(literal,look))+'};',
      'static const float original_chicago_base[117]={'+','.join(map(literal,base))+'};',
      'static const float original_chicago_arp_base[10]={'+','.join(map(literal,norm['ARP_converted_values']['53_base_note_Q10_table']))+'};']
    golden=bytearray();codes=[];dictionary=[];seen={};name_offset=[];name_bytes=bytearray()
    for s in presets:
        assert len(s['macro22'])==22 and len(s['name'])<20
        native=base[:]
        for i,v in s['canonical_native_target_values'].items():
            if int(i) in ignored:assert v is None
            else:native[int(i)]=v
        assert native[109:113]==s['FX1_normalized4'] and native[113:117]==s['FX2_normalized4']
        assert hashlib.sha256(struct.pack('<109f',*native[:109])).hexdigest()==s['core109_float_bytes_sha256']
        golden+=struct.pack('<117f22f',*native,*s['macro22'])
        name_offset.append(len(name_bytes));name_bytes+=s['name'].encode()+b'\0'
        for value in s['macro22']:
            key=struct.pack('<f',value)
            if key not in seen:seen[key]=len(dictionary);dictionary.append(value)
            codes.append(seen[key])
    packed=bytearray((len(codes)*9+7)//8)
    assert len(dictionary)<=512
    for k,code in enumerate(codes):
        bit=k*9;byte=bit>>3;word=code<<(bit&7)
        packed[byte]|=word&255;packed[byte+1]|=word>>8
    assert all(((packed[k*9//8]|(packed[k*9//8+1]<<8))>>(k*9%8))&511==code for k,code in enumerate(codes))
    if expanded:
        lines+=['#define ORIGINAL_CHICAGO_EXPANDED_FACTORY 1',
          'static const char original_chicago_factory_name[81][20]={'+','.join(json.dumps(s['name']) for s in presets)+'};',
          'static const float original_chicago_factory_macro[81][22]={']
        lines+=['{'+','.join(map(literal,s['macro22']))+'},' for s in presets];lines+=['};']
    else:
        lines+=['static const float original_chicago_factory_dictionary[%d]={'%len(dictionary)+','.join(map(literal,dictionary))+'};',
          'static const uint8_t original_chicago_factory_codes[%d]={'%len(packed)+','.join(map(str,packed))+'};',
          'static const uint16_t original_chicago_factory_name_offset[81]={'+','.join(map(str,name_offset))+'};',
          'static const char original_chicago_factory_names[%d]={'%len(name_bytes)+','.join(map(str,name_bytes))+'};']
    lines+=['']
    header.write_text('\n'.join(lines));fixture.write_bytes(golden)
    print(json.dumps({'factories':81,'public_controls':22,'effective_targets':29,'original_noop_records':5,'shared_native_fields':88,'snapshot_bytes':208,'expanded_snapshot_bytes':560,'factory_values':len(codes),'dictionary_values':len(dictionary),'macro_raw_bytes':len(codes)*4,'macro_compact_bytes':len(dictionary)*4+len(packed),'names_raw_bytes':81*20,'names_compact_bytes':len(name_bytes)+81*2,'factory_readonly_saving_bytes':len(codes)*4+81*20-len(dictionary)*4-len(packed)-len(name_bytes)-81*2,'expanded':expanded,'fixture_bytes':len(golden),'fixture_sha256':hashlib.sha256(golden).hexdigest()},sort_keys=True))
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('reference',type=pathlib.Path);p.add_argument('header',type=pathlib.Path);p.add_argument('fixture',type=pathlib.Path);p.add_argument('--expanded',action='store_true');a=p.parse_args();generate(a.reference,a.header,a.fixture,a.expanded)
