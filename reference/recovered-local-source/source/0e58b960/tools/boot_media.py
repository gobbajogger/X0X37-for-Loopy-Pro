#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Bounded Elite/Pro boot media codec. No device, network or MP4 support.

Website conversion must supply 80x80 packed palette-index frames and durations.
This reference packer rejects output above the capability's payload limit.
"""
import argparse
import hashlib
import json
import struct
import zlib
from pathlib import Path
CUSTOM_MAX = 3840
COMPILED_MAX = 32768

def _compress(raw):
    out = bytearray(); i = 0
    while i < len(raw):
        at = len(out); out.append(0); flags = 0
        for bit in range(8):
            if i == len(raw): break
            length = distance = 0
            for j in range(i-1, max(-1, i-1025), -1):
                if raw[j] != raw[i]: continue
                n = 1
                while n < 18 and i+n < len(raw) and raw[j+n] == raw[i+n]: n += 1
                if n > length: length, distance = n, i-j
                if length == 18: break
            if length >= 3:
                out += struct.pack('<H', ((distance-1)<<4)|(length-3)); i += length
            else:
                flags |= 1<<bit; out.append(raw[i]); i += 1
        out[at] = flags
    return bytes(out)

def pack(palette, frames, durations, cap=CUSTOM_MAX):
    if not 1 <= len(palette) <= 16 or any(type(c) is not int or not 0 <= c <= 65535 for c in palette):
        raise ValueError('RGB565 palette must contain 1..16 integers')
    if not 1 <= len(frames) <= 60 or len(durations) != len(frames): raise ValueError('frame count')
    if any(type(t) is not int or not 100 <= t <= 1000 for t in durations) or sum(durations)>6000:
        raise ValueError('frame/total duration')
    if any(len(f)!=3200 for f in frames): raise ValueError('each 80x80 frame is 3200 packed bytes')
    raw=b''.join(frames)
    if any(b>>4>=len(palette) or b&15>=len(palette) for b in raw): raise ValueError('palette index')
    compressed=_compress(raw); h=bytearray(64)
    struct.pack_into('<BBBBHHHHIIII',h,0,3,1,1,len(palette),80,80,len(frames),sum(durations),len(compressed),len(raw),zlib.crc32(compressed),zlib.crc32(raw))
    struct.pack_into('<16H',h,32,*(list(palette)+[0]*(16-len(palette))))
    result=bytes(h)+struct.pack('<'+'H'*len(frames),*durations)+compressed
    if len(result)>cap: raise ValueError(f'output {len(result)} exceeds capability payload {cap}; no truncation or still substitution')
    return result

def unpack(data, cap=CUSTOM_MAX):
    if not 64 <= len(data) <= cap: raise ValueError('payload size')
    kind,version,codec,colors,w,h,n,total,nc,nd,cc,dc=struct.unpack_from('<BBBBHHHHIIII',data)
    if (kind,version,codec,w,h)!=(3,1,1,80,80) or not 1<=colors<=16 or not 1<=n<=60 or not 100<=total<=6000 or data[28:32]!=bytes(4):
        raise ValueError('header')
    start=64+2*n
    if start>=len(data) or nc!=len(data)-start or nd!=n*3200: raise ValueError('lengths')
    palette=struct.unpack_from('<16H',data,32)
    if any(palette[colors:]): raise ValueError('palette padding')
    durations=struct.unpack_from('<'+'H'*n,data,64)
    if any(not 100<=v<=1000 for v in durations) or sum(durations)!=total: raise ValueError('durations')
    compressed=data[start:]
    if zlib.crc32(compressed)!=cc: raise ValueError('compressed CRC')
    raw=bytearray(); pos=0;flags=0
    while len(raw)<nd:
        if pos>=len(compressed): raise ValueError('flags underflow')
        flags=compressed[pos];pos+=1
        for bit in range(8):
            if len(raw)==nd: break
            literal=flags&1;flags>>=1
            if literal:
                if pos>=len(compressed): raise ValueError('literal underflow')
                raw.append(compressed[pos]);pos+=1
            else:
                if pos+2>len(compressed): raise ValueError('reference underflow')
                token=struct.unpack_from('<H',compressed,pos)[0];pos+=2;distance=(token>>4)+1;length=(token&15)+3
                if distance>1024 or distance>len(raw) or length>nd-len(raw): raise ValueError('reference bounds')
                for _ in range(length): raw.append(raw[-distance])
    if pos!=len(compressed) or flags or zlib.crc32(raw)!=dc: raise ValueError('trailing/canonical flags/decoded CRC')
    if any(b>>4>=colors or b&15>=colors for b in raw): raise ValueError('pixel index')
    return {'palette':list(palette[:colors]),'durations':list(durations),'frames':[bytes(raw[i:i+3200]) for i in range(0,len(raw),3200)]}

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('file',type=Path);parser.add_argument('--compiled',action='store_true',help='inspect immutable compiled default, not an upload limit')
    args=parser.parse_args();data=args.file.read_bytes();result=unpack(data,COMPILED_MAX if args.compiled else CUSTOM_MAX)
    print(json.dumps({'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),'frames':len(result['frames']),'duration_ms':sum(result['durations']),'palette_rgb565':result['palette'],'audio':'none'}))
