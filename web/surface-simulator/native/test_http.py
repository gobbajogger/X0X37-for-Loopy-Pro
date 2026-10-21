#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
import functools,http.server,json,threading,urllib.request,urllib.error,tempfile,pathlib
from server import Core,Handler,HERE
web=tempfile.TemporaryDirectory();root=pathlib.Path(web.name)
(root/'index.html').write_text('<h1>local test</h1>')
(root/'empty').mkdir();(root/'build').mkdir();(root/'build/private').write_text('not served')
(root/'.hidden').write_text('not served');(root/'escape').symlink_to(HERE/'server.py')
s=http.server.HTTPServer(('127.0.0.1',0),functools.partial(Handler,directory=str(root)))
s.core=Core(HERE/'build/core.so');t=threading.Thread(target=s.serve_forever,daemon=True);t.start()
url=f'http://127.0.0.1:{s.server_port}'
try:
    assert json.load(urllib.request.urlopen(url+'/api/info'))['backend']=='native-host-firmware'
    def post(data):return urllib.request.urlopen(urllib.request.Request(url+'/api/command',data=json.dumps(data).encode(),headers={'Content-Type':'application/json'}))
    assert json.load(post({'op':'input','args':[0,10,1]}))['state']['playing']==1
    assert json.load(post({'op':'input','args':[0,10,0]}))['state']['heldButtons']==0
    for request in [urllib.request.Request(url+'/api/state',headers={'Host':'attacker.example'}),urllib.request.Request(url+'/api/command',data=b'{}',headers={'Content-Type':'application/json','Origin':'https://attacker.example'})]:
        try:urllib.request.urlopen(request);raise AssertionError('must reject')
        except urllib.error.HTTPError as e:assert e.code==403
    for path in ['/empty/','/build/private','/.hidden','/escape']:
        try:urllib.request.urlopen(url+path);raise AssertionError('private paths must reject')
        except urllib.error.HTTPError as e:assert e.code==403
    for path in ['/','/api/state','/../server.py']:
        try:urllib.request.urlopen(urllib.request.Request(url+path,method='HEAD'));raise AssertionError('HEAD must reject')
        except urllib.error.HTTPError as e:assert e.code==405
    for data in [{'op':'advance','args':[1001]},{'op':'smc-midi','args':[256,1,1]},{'op':'notes','args':[60,999,0,0,0,0]}]:
        try:post(data);raise AssertionError('must reject invalid bounds')
        except urllib.error.HTTPError as e:assert e.code==400
    (HERE/'build/actual-firmware-frame.png').write_bytes(urllib.request.urlopen(url+'/api/frame.png').read())
    print('HTTP state/input/frame, hostile Host and cross-origin rejection: PASS')
finally:s.shutdown();s.server_close();t.join();web.cleanup()
