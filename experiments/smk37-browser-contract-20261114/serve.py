#!/usr/bin/env python3
"""Serve only this fake fixture directory on loopback. No MIDI permission."""
import http.server
import pathlib
import sys

class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self,*args,**kwargs):
        super().__init__(*args,directory=str(pathlib.Path(__file__).parent),**kwargs)
    def end_headers(self):
        self.send_header('Permissions-Policy','midi=()')
        self.send_header('Cache-Control','no-store')
        super().end_headers()

if __name__=='__main__':
    http.server.ThreadingHTTPServer(('127.0.0.1',int(sys.argv[1]) if len(sys.argv)>1 else 8765),Handler).serve_forever()
