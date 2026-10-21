#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Local-only actual X0X host app server. Standard library only; no hardware I/O."""
import argparse, ctypes, functools, http.server, json, pathlib, struct, urllib.parse, zlib
HERE=pathlib.Path(__file__).resolve().parent
FIELDS='ms playing view part recording buttons keys dimKeys bpmX10 midiCount noteOns heldButtons heldKeys overlay'.split()
class Core:
    def __init__(self,path):
        self.lib=ctypes.CDLL(str(path))
        self.lib.sim_input.argtypes=[ctypes.c_int]*3
        self.lib.sim_advance.argtypes=[ctypes.c_int]
        self.lib.sim_state.argtypes=[ctypes.POINTER(ctypes.c_uint32)]
        self.lib.sim_rgb.argtypes=[ctypes.POINTER(ctypes.c_uint8)]
        self.lib.sim_notes.argtypes=[ctypes.c_int]*6+[ctypes.POINTER(ctypes.c_uint8)]
        self.lib.sim_pad.argtypes=[ctypes.c_int]*6
        self.lib.sim_relative.argtypes=[ctypes.c_int]
        self.lib.sim_smc_link.argtypes=[ctypes.c_int]*2
        self.lib.sim_smc_message.argtypes=[ctypes.c_int]*3
        self.lib.sim_smc_mmc.argtypes=[ctypes.c_int]
        self.lib.sim_smc_state.argtypes=[ctypes.POINTER(ctypes.c_uint32)]
        self.lib.sim_init()
    def state(self):
        a=(ctypes.c_uint32*len(FIELDS))(); self.lib.sim_state(a)
        state=dict(zip(FIELDS,a))
        smc=(ctypes.c_uint32*142)();self.lib.sim_smc_state(smc)
        state['smc']={'up':smc[0],'picked':smc[1],'seen':smc[2],'solo':smc[3],'rxDiag':smc[4],'feedbackCount':smc[5],'values':list(smc[6:14]),'leds':list(smc[14:142])}
        return state
    def png(self):
        rgb=(ctypes.c_uint8*(240*240*3))();self.lib.sim_rgb(rgb);rgb=bytes(rgb)
        def chunk(k,d):return struct.pack('>I',len(d))+k+d+struct.pack('>I',zlib.crc32(k+d)&0xffffffff)
        raw=b''.join(b'\0'+rgb[y*720:(y+1)*720] for y in range(240))
        return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',240,240,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(raw))+chunk(b'IEND',b'')
    def command(self,data):
        if not isinstance(data,dict):raise ValueError('object required')
        op=data.get('op');args=data.get('args',[])
        if not isinstance(args,list) or len(args)>6 or any(type(x) is not int or abs(x)>2147483647 for x in args):raise ValueError('bounded integer args required')
        if op=='input' and len(args)==3: result=self.lib.sim_input(*args)
        elif op=='advance' and len(args)==1:result=self.lib.sim_advance(*args)
        elif op=='smc-link' and len(args)==2:result=self.lib.sim_smc_link(*args)
        elif op=='smc-midi' and len(args)==3:result=self.lib.sim_smc_message(*args)
        elif op=='smc-mmc' and len(args)==1:result=self.lib.sim_smc_mmc(*args)
        elif op=='notes' and len(args)==6:
            out=(ctypes.c_uint8*7)();n=self.lib.sim_notes(*args,out)
            if n<0:raise ValueError('notes range')
            return {'notes':list(out[:n])}
        elif op=='pad' and len(args)==6:result=self.lib.sim_pad(*args)
        elif op=='relative' and len(args)==1:
            if not 0<=args[0]<128:raise ValueError('relative range')
            return {'value':self.lib.sim_relative(*args)}
        else:raise ValueError('unknown operation or argument count')
        if result<0:raise ValueError('input out of range')
        return {'value':result,'state':self.state()}
class Handler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Cache-Control','no-store');self.send_header('X-Content-Type-Options','nosniff');super().end_headers()
    def allowed(self):
        host=self.headers.get('Host','');allowed={f'127.0.0.1:{self.server.server_port}',f'localhost:{self.server.server_port}'}
        origin=self.headers.get('Origin')
        return host in allowed and (origin is None or origin == 'http://'+host) and self.headers.get('Sec-Fetch-Site') not in {'cross-site'}
    def reply(self,status,data,kind='application/json'):
        if kind=='application/json':data=json.dumps(data).encode()
        self.send_response(status);self.send_header('Content-Type',kind);self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data)
    def do_HEAD(self):
        self.send_response(405);self.send_header('Allow','GET, POST');self.send_header('Content-Length','0');self.end_headers()
    def list_directory(self,path):
        self.reply(403,{'error':'directory listing disabled'});return None
    def do_GET(self):
        if not self.allowed():return self.reply(403,{'error':'local same-origin only'})
        path=urllib.parse.urlparse(self.path).path
        if path=='/api/state':return self.reply(200,self.server.core.state())
        if path=='/api/frame.png':return self.reply(200,self.server.core.png(),'image/png')
        if path=='/api/info':return self.reply(200,{'backend':'native-host-firmware','screen':[240,240],'keys':27,'smc':'production MIDI queue/mapping/pickup/LED logic; no radio','limitations':['FM-1 host HAL, not SMK hardware main loop','No BLE, USB, device timing or live audio output','SMK clip key and pad functions are separate pure firmware calls'], 'buttons':['FX','SEL','ENV','LFO','EDIT','GLO','HOME','SAVE','ARP','SEQ','PLAY','REC','OCT-','OCT+'],'encoders':['SELECT','ALGO','PRESET','K1','K2','K3','K4']})
        # Serve only within explicit web root; exclude symlink escapes and dotfiles.
        target=pathlib.Path(self.translate_path(self.path)).resolve()
        root=pathlib.Path(self.directory).resolve()
        if not target.is_relative_to(root) or any(p.startswith('.') or p in {'build','__pycache__'} for p in target.relative_to(root).parts):return self.reply(403,{'error':'outside web root'})
        return super().do_GET()
    def do_POST(self):
        if not self.allowed():return self.reply(403,{'error':'local same-origin only'})
        if self.path!='/api/command':return self.reply(404,{'error':'not found'})
        try:
            n=int(self.headers.get('Content-Length','0'))
            if n<1 or n>1024 or self.headers.get('Content-Type')!='application/json':raise ValueError('small JSON body required')
            result=self.server.core.command(json.loads(self.rfile.read(n)))
        except (ValueError,TypeError,json.JSONDecodeError) as exc:return self.reply(400,{'error':str(exc)})
        return self.reply(200,result)
def main():
    p=argparse.ArgumentParser();p.add_argument('--port',type=int,default=8765);p.add_argument('--web',type=pathlib.Path,default=HERE/'demo');p.add_argument('--library',type=pathlib.Path,default=HERE/'build/core.so');a=p.parse_args()
    s=http.server.HTTPServer(('127.0.0.1',a.port),functools.partial(Handler,directory=str(a.web.resolve())))
    s.core=Core(a.library.resolve());print(f'Actual firmware host UI: http://127.0.0.1:{s.server_port}',flush=True)
    try:s.serve_forever()
    except KeyboardInterrupt:pass
    finally:s.server_close()
if __name__=='__main__':main()
