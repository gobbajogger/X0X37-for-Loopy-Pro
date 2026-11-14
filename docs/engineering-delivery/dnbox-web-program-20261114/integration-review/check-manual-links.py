# Normal-trust read-only destination checks. No TLS override, device or external writes.
from pathlib import Path
import urllib.request,urllib.error,urllib.parse,re,json,concurrent.futures,hashlib
out=Path(__file__).parent
intake=out.parent/'learning-intake'
p=intake/'candidate-browser-manual.md'
source=p.read_text()
links=[]
for i,line in enumerate(source.splitlines(),1):
 for m in re.finditer(r'\[([^]]+)\]\(([^)]+)\)',line):
  resolved=urllib.parse.urljoin('https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/a19da721c25ab2323bb818851ab4d92e8ca3aa15/web/MANUAL.md',m.group(2))
  links.append({'line':i,'label':m.group(1),'authored':m.group(2),'resolved':resolved})
urls=sorted(set(x['resolved'].split('#')[0] for x in links))
def get(url):
 try:
  with urllib.request.urlopen(url,timeout=20) as r:
   b=r.read();return {'url':url,'status':r.status,'final_url':r.url,'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()}
 except urllib.error.HTTPError as e:return {'url':url,'status':e.code,'error':str(e)}
 except Exception as e:return {'url':url,'error':str(e)}
with concurrent.futures.ThreadPoolExecutor(max_workers=6) as pool: results=list(pool.map(get,urls))
packet={'utc':__import__('datetime').datetime.now(__import__('datetime').timezone.utc).isoformat(),'source_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'source_bytes':len(p.read_bytes()),'links':links,'destinations':results}
(out/'manual-links.json').write_text(json.dumps(packet,indent=2))
print(json.dumps(packet,indent=2))
