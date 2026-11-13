#!/usr/bin/env python3
"""Focused retained-RAM boot checks; optional native ELF checks defeat LTO narrowing."""
import argparse, pathlib, re, subprocess, tempfile
ROOT = pathlib.Path(__file__).resolve().parents[2]
def native(elf, objdump):
    syms = subprocess.check_output([objdump, '-t', elf], text=True)
    dis = subprocess.check_output([objdump, '-d', elf], text=True)
    assert re.search(r'\.noinit\s+0000000c bootguard$', syms, re.M), 'bootguard must remain a 12-byte retained object, not scalarized fields'
    start = dis.split('fm1_cstart:', 1)[1].split('\n\n', 1)[0]
    magic = re.search(r'(r\d+) = [^\n]*\b42475244\b[^\n]*', start, re.I)
    assert magic, 'native startup must retain the full BOOTGUARD_MAGIC'
    reg = magic.group(1)
    before, after = start[max(0, magic.start()-400):magic.start()], start[magic.end():magic.end()+600]
    assert re.search(r'r\d+ = \[r\d+\+0\]', before), 'native magic must be loaded as a word'
    assert re.search(r'if \(r\d+ != '+reg+r'\)', after), 'native startup must compare the loaded magic'
    assert re.search(r'(?<!b)\[r\d+\+0\] = '+reg+r'\b', after), 'native startup must store the full magic word'
    print('PASS native retained bootguard representation and 32-bit magic constant')
def seeded():
    src = (ROOT/'firmware/src/app/main_fm1.c').read_text()
    definition = re.search(r'^(?:volatile )?struct bootguard_s bootguard .*?;', src, re.M).group()
    block = src[src.index('    if (bootguard.magic != BOOTGUARD_MAGIC) {'):src.index('    p3 |= (bootguard.failed >= 2u)')]
    code = '''#include <stdint.h>\n#include <stdio.h>\n#include <setjmp.h>\n#define BOOTGUARD_MAGIC 0x42475244u\nstruct bootguard_s {uint32_t magic,failed,pending;};\n'''+definition+'''
static jmp_buf done;
static void fm1_enter_uboot(void) {longjmp(done,1);}
int main(void) {unsigned m,f,p; while(scanf("%x %x %x",&m,&f,&p)==3) {
bootguard.magic=m;bootguard.failed=f;bootguard.pending=p;
int mode;if(setjmp(done)) mode=2;else {\n'''+block+'''
mode=bootguard.failed>=2?1:0;}
printf("%u %x %x %x\\n",mode,bootguard.magic,bootguard.failed,bootguard.pending);
}return 0;}
'''
    cases=[(m,f,p) for m in (0,1,0x01000001,0xffffffff,0x42475244) for f in (0,1,2,3,4,0xffffffff) for p in (0,1,0x100)]
    expected=[]
    for m,f,p in cases:
        if m!=0x42475244:m,f,p=0x42475244,0,0
        if p:f=(f+1)&0xffffffff
        p=1
        mode=2 if f>=4 else 1 if f>=2 else 0
        if mode==2:f,p=0,0
        expected.append((mode,m,f,p))
    with tempfile.TemporaryDirectory() as td:
        p=pathlib.Path(td);(p/'test.c').write_text(code)
        subprocess.run(['cc','-std=c11','-O2','-flto','-Wall','-Wextra',str(p/'test.c'),'-o',str(p/'test')],check=True)
        out=subprocess.check_output([str(p/'test')],input=''.join('%x %x %x\n'%c for c in cases),text=True)
        actual=[tuple(int(x,16) for x in line.split()) for line in out.splitlines()]
        assert actual==expected, 'seeded retained state differs from boot policy'
    print(f'PASS {len(cases)} seeded normal/safe/UBOOT and retention cases (host, not hardware)')
if __name__=='__main__':
    a=argparse.ArgumentParser();a.add_argument('--elf');a.add_argument('--objdump');args=a.parse_args()
    if args.elf:native(args.elf,args.objdump)
    seeded()
