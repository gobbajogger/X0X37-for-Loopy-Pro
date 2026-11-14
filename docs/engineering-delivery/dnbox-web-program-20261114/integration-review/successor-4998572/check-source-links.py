# Read-only normal-trust HTTPS checks for manual delivery and authored anchors.
from pathlib import Path
import urllib.request,urllib.error,re,json,hashlib,concurrent.futures,html,datetime
out=Path(__file__).resolve().parent
program=out.parents[1]
src=program/'learning-intake/manual-successor-4998572/web-MANUAL.md'
b=src.read_bytes()
sha=lambda x:hashlib.sha256(x).hexdigest()
blob=lambda x:hashlib.sha1(('blob '+str(len(x))+'\0').encode()+x).hexdigest()
source={'path':str(src),'bytes':len(b),'sha256':sha(b),'git_blob':blob(b),'legacy_count':b.count(b'legacy.html')}
assert source['bytes']==13276 and source['sha256']=='51b2906afe94aa51ebf5cc1d94924d17bfcb214c24afa9fe5726f8035e17b89b' and source['git_blob']=='a7f36a186659304340bbdadb2338cd8b3c94702a'
links=[]
for i,line in enumerate(b.decode().splitlines(),1):
 for m in re.finditer(r'\[([^]]+)\]\(([^)]+)\)',line):
  if '/docs/MANUAL.md' in m.group(2):links.append({'line':i,'label':m.group(1),'url':m.group(2)})
base='https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/'
raw='https://raw.githubusercontent.com/gobbajogger/X0X37-for-Loopy-Pro/'
commit='4998572bcc06676f08a7875ca49fb2a56358eff6';parent='a19da721c25ab2323bb818851ab4d92e8ca3aa15'
urls=[base+commit+'/docs/MANUAL.md',base+commit+'/web/MANUAL.md',base+parent+'/docs/MANUAL.md',raw+commit+'/docs/MANUAL.md',raw+commit+'/web/MANUAL.md','https://api.github.com/repos/gobbajogger/X0X37-for-Loopy-Pro/commits/'+commit]
def get(url):
 try:
  with urllib.request.urlopen(url,timeout=25) as r:data=r.read();return {'url':url,'status':r.status,'final_url':r.url,'bytes':len(data),'sha256':sha(data)},data
 except Exception as e:return {'url':url,'error':str(e)},None
with concurrent.futures.ThreadPoolExecutor(max_workers=6) as pool:responses=list(pool.map(get,urls))
results=[x[0] for x in responses];data={x[0]['url']:x[1] for x in responses}
# Search actual GitHub HTML and embedded rendered HTML. Raw heading text alone is not the anchor test.
page=data[base+parent+'/docs/MANUAL.md'].decode()
variants=[page,html.unescape(page),page.replace('\\"','"'),html.unescape(page).replace('\\"','"')]
for link in links:
 fragment=link['url'].split('#',1)[1] if '#' in link['url'] else None
 link['target_status']=next(r['status'] for r in results if r['url']==base+parent+'/docs/MANUAL.md')
 link['fragment']=fragment
 link['rendered_anchor_present']=True if fragment is None else any(re.search(r'id=["\'](?:user-content-)?'+re.escape(fragment)+r'["\']',v) is not None for v in variants)
 link['heading_text']=None if fragment is None else ('3. Quick start: real musical tasks' if fragment.startswith('3-') else '2. The panel')
new_keyboard=data[raw+commit+'/docs/MANUAL.md'];old_keyboard=(program/'learning-intake/candidate-keyboard-manual.md').read_bytes()
new_browser=data[raw+commit+'/web/MANUAL.md']
metadata=json.loads(data['https://api.github.com/repos/gobbajogger/X0X37-for-Loopy-Pro/commits/'+commit])
packet={'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'source':source,'links':links,'http_results':results,'keyboard_source_unchanged':new_keyboard==old_keyboard,'keyboard_git_blob':blob(new_keyboard),'browser_remote_matches_consumed':new_browser==b,'browser_remote_git_blob':blob(new_browser),'commit_parent':metadata['parents'][0]['sha'],'commit_changed_paths':[x['filename'] for x in metadata['files']]}
(out/'source-and-links.json').write_text(json.dumps(packet,indent=2))
print(json.dumps(packet,indent=2))
assert len(links)==4 and all(x['target_status']==200 and x['rendered_anchor_present'] for x in links)
assert all(x.get('status')==200 for x in results)
assert packet['keyboard_source_unchanged'] and packet['browser_remote_matches_consumed'] and packet['commit_parent']==parent
