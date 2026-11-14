# Read-only normal-trust HTTP/source checks. No issue submission or device action.
from pathlib import Path
import urllib.request,urllib.error,hashlib,re,html,json,concurrent.futures,datetime
out=Path(__file__).resolve().parent
urls={
'notes_page':'https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/4998572bcc06676f08a7875ca49fb2a56358eff6/web/NEW.md',
'notes_raw':'https://raw.githubusercontent.com/gobbajogger/X0X37-for-Loopy-Pro/4998572bcc06676f08a7875ca49fb2a56358eff6/web/NEW.md',
'release':'https://github.com/gobbajogger/X0X37-for-Loopy-Pro/releases/tag/v0.0.25.1',
'nightly_status':'https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/new/',
'withdrawal_record':'https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/120',
'withdrawal_metadata':'https://api.github.com/repos/gobbajogger/x0x37-for-Loopy-Pro_nightly/pulls/120',
'feature_form':'https://github.com/gobbajogger/X0X37-for-Loopy-Pro/issues/new?template=feature_request.yml',
'default_form_source':'https://raw.githubusercontent.com/gobbajogger/X0X37-for-Loopy-Pro/035026fe70059af35e25c7c121b40f318b732fe3/.github/ISSUE_TEMPLATE/feature_request.yml'}
def get(kv):
 key,url=kv
 try:
  with urllib.request.urlopen(url,timeout=25) as r:b=r.read();return {'key':key,'url':url,'method':'GET','status':r.status,'final_url':r.url,'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()},b
 except urllib.error.HTTPError as e:return {'key':key,'url':url,'method':'GET','status':e.code,'error':str(e)},None
 except Exception as e:return {'key':key,'url':url,'method':'GET','error':str(e)},None
with concurrent.futures.ThreadPoolExecutor(max_workers=6) as pool:responses=list(pool.map(get,urls.items()))
results=[x[0] for x in responses];bodies={x[0]['key']:x[1] for x in responses}
notes=bodies['notes_raw'];noteshtml=bodies['notes_page'].decode();forms=bodies['default_form_source'];status=bodies['nightly_status'].decode();withdraw=json.loads(bodies['withdrawal_metadata'])
variants=[noteshtml,noteshtml.replace('\\"','"'),html.unescape(noteshtml),html.unescape(noteshtml).replace('\\"','"')]
facts={'notes_bytes':len(notes),'notes_git_blob':hashlib.sha1(('blob '+str(len(notes))+'\0').encode()+notes).hexdigest(),'notes_sha256':hashlib.sha256(notes).hexdigest(),'notes_anchor_present':any(re.search(r'id=["\'](?:user-content-)?dnbox-install-and-data-workspace["\']',v) for v in variants),'notes_have_candidate_pin':b'94197fc8' in notes,'notes_synth_names_present':all(x in notes for x in [b'Chicago',b'Miami',b'Juno',b'Drone']),'notes_source_has_generated_site_links':all(x in notes for x in [b'legacy.html',b'../history/']),'default_form_bytes':len(forms),'default_form_sha256':hashlib.sha256(forms).hexdigest(),'default_form_git_blob':hashlib.sha1(('blob '+str(len(forms))+'\0').encode()+forms).hexdigest(),'default_form_existing_fields':all(x in forms for x in [b'Keyboard model',b'What should change',b'Why it would help',b'Extra context']),'nightly_has_corrected_and_original_pins':all(x in status for x in ['94197fc8','600b3b45']),'nightly_has_physical_unverified_notice':bool(re.search(r'Physical startup, restart and recovery are not yet verified',status)),'withdrawal_title':withdraw['title'],'withdrawal_mentions_withdrawal':bool(re.search('withdraw',withdraw['title']+' '+withdraw.get('body',''),re.I))}
packet={'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'requests':results,'facts':facts,'limits':'GET only; no issue created, no form submitted, no vote/inbox/scheduler action; ordinary trust, no certificate bypass; no external Chromium result claimed'}
(out/'http-source-results.json').write_text(json.dumps(packet,indent=2));print(json.dumps(packet,indent=2))
assert facts['notes_bytes']==2174 and facts['notes_sha256']=='22637418716534c5e84381751c6a38aea8c786764dc2050adf1a47d348e0fa5d' and facts['notes_git_blob']=='730dbab61e4b398de855c0ef74911b06b79dbe19' and facts['notes_anchor_present']
assert facts['default_form_bytes']==825 and facts['default_form_sha256']=='c49fed71a790ac96d5ffbd13bdfb225ab8ec3e95dc1f6443ffcad84a1d557c35' and facts['default_form_existing_fields']
assert all(r.get('status')==200 for r in results if r['key']!='feature_form')
