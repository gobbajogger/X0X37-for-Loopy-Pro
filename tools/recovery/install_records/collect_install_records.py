#!/usr/bin/env python3
"""Offline narrow existing install-record recovery; uploaded files are never executed."""
import datetime, hashlib, json, os, stat, sys, tempfile, zipfile
from pathlib import Path
from safe_reads import safe_open_dir, read_stable, stamp
MAX_FILES=256
MAX_BYTES=32*1024*1024
MAX_FILE=2*1024*1024
MAX_ENTRIES=12000
MAX_DEPTH=5
EXACT=[
 'final_install_integration/evidence/baseline-elite.log',
 'final_install_integration/evidence/build-elite.log',
 'hardware_install_20261020/install-record.json',
 'hardware_install_20261020/install.log',
 'hardware_smc_install_20261020/install-record.json',
 'hardware_smc_install_20261020/install.log',
 'hardware_smc_install_20261020/commit.txt',
 'hardware_smc_install_20261020/dev-validation/record.json',
 'hardware_smc_install_20261020/device-checks.txt',
 'hardware_firmware_ux_install_20261020/install-record.json',
 'hardware_firmware_ux_install_20261020/install.log',
 'hardware_firmware_ux_install_20261020/controller-restore-record.json',
 'hardware_firmware_ux_install_20261020/controller-restore.log',
 'hardware_firmware_ux_repair_install_20261020/install-record.json',
 'hardware_firmware_ux_repair_install_20261020/install.log',
 'hardware_firmware_ux_repair_install_20261020/device-readback-verification.json']
PRUNE={'docs','tests','fixtures','src','hal','include','firmware','ble','tools','web','node_modules','venv','__pycache__','samples','slot_backups','archive','refs','assets','sdk','toolchain','host','host-logs','diagnostic-host-logs','build','out'}
TOKENS=('install','smc','integration','20261019','2026-10-19')
def sensitive(n):
 return n.startswith('.') or any(t in n for t in ('credential','secret','token','password','auth','keychain','backup','sdk','toolchain','jieli','ac79'))
def candidate(n, rel):
 n=n.lower(); r=rel.lower()
 if sensitive(n) or not n.endswith(('.log','.txt','.json','.jsonl','.md')): return False
 if n in ('commit.txt','record.json','manifest.json','checksums.txt','sha256sums.txt'): return True
 return any(t in n for t in ('install','diag','readback','device-check','controller-restore','flash','handoff')) or ('smc' in n and any(t in n for t in ('capture','device','success','evidence')))
def collect(root, output):
 m={'format':'smc-install-records-v1','collected_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'root':str(root),'limits':{'files':MAX_FILES,'bytes':MAX_BYTES,'file_bytes':MAX_FILE,'entries':MAX_ENTRIES,'depth':MAX_DEPTH},'inventory':[],'scan_complete':True,'warning':'Logs may contain private paths, device identifiers or secrets. Review before manual upload. Timestamps are not installation proof. No network/device/Git operations.'}
 counts={'files':0,'bytes':0,'entries':0}; seen=set()
 with zipfile.ZipFile(output,'x',compression=zipfile.ZIP_DEFLATED) as z:
  def copy(fd,name,rel):
   if rel in seen:return
   seen.add(rel); row={'path':rel}
   try:
    s=os.stat(name,dir_fd=fd,follow_symlinks=False); row.update(stamp(s))
    if not stat.S_ISREG(s.st_mode):row['status']='nonregular_not_read'
    elif s.st_size>MAX_FILE:row['status']='file_size_limit'
    elif counts['files']>=MAX_FILES or counts['bytes']+s.st_size>MAX_BYTES:row['status']='archive_budget_limit'
    else:
     data=read_stable(fd,name,s,MAX_FILE); member='evidence/project/'+rel
     z.writestr(member,data); row.update(status='copied',sha256=hashlib.sha256(data).hexdigest(),archive_member=member)
     counts['files']+=1;counts['bytes']+=len(data)
   except (OSError,ValueError) as e:row.update(status='missing_unreadable_or_changed',error=str(e))
   m['inventory'].append(row)
  def exact(fd,rel):
   current=os.dup(fd)
   try:
    parts=rel.split('/')
    for part in parts[:-1]:
     new=os.open(part,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW,dir_fd=current);os.close(current);current=new
    copy(current,parts[-1],rel)
   except OSError as e:m['inventory'].append({'path':rel,'status':'missing_unreadable_or_symlink_parent','error':str(e)})
   finally:os.close(current)
  def walk(fd,rel='',depth=0):
   names=[]
   try:
    with os.scandir(fd) as entries:
     for e in entries:
      if counts['entries']+len(names)>=MAX_ENTRIES:
       m['scan_complete']=False;m['inventory'].append({'path':rel,'status':'listing_limit'});break
      names.append(e.name)
   except OSError as e:m['inventory'].append({'path':rel,'status':'directory_error','error':str(e)});return
   names.sort(key=lambda n:(0 if '20261019' in n or '2026-10-19' in n else 1,0 if 'install' in n else 1,n))
   for n in names:
    if counts['entries']>=MAX_ENTRIES:m['scan_complete']=False;return
    counts['entries']+=1; child=rel+'/'+n if rel else n;low=n.lower()
    try:
     s=os.stat(n,dir_fd=fd,follow_symlinks=False)
     if stat.S_ISLNK(s.st_mode):m['inventory'].append({'path':child,'status':'symlink_not_followed'});continue
     if stat.S_ISDIR(s.st_mode):
      allowed=not sensitive(low) and low not in PRUNE and (depth>0 or any(t in low for t in TOKENS))
      status='directory_selected' if allowed else 'directory_not_selected'
      if allowed and depth>=MAX_DEPTH:status='depth_limit';allowed=False;m['scan_complete']=False
      m['inventory'].append({'path':child,'status':status})
      if allowed:
       sub=os.open(n,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW,dir_fd=fd)
       try:walk(sub,child,depth+1)
       finally:os.close(sub)
     elif stat.S_ISREG(s.st_mode) and candidate(n,child) and (depth>0 or any(t in low for t in TOKENS)):copy(fd,n,child)
    except OSError as e:m['inventory'].append({'path':child,'status':'directory_or_stat_error','error':str(e)})
  try:
   fd=safe_open_dir(root)
   try:
    m['root_status']='opened'
    for rel in EXACT:exact(fd,rel)
    walk(fd)
   finally:os.close(fd)
  except OSError as e:m.update(root_status='missing_unreadable_or_symlink',error=str(e),scan_complete=False)
  m['counts']=counts
  z.writestr('MANIFEST.json',json.dumps(m,indent=2))
  z.writestr('READ-ME.txt',m['warning']+'\nExcluded directories are intentional; missing/capped files are inventoried. Source project is not modified.\n')
 return m

def main():
 home=Path.home();dest=home/'Desktop'
 if not dest.is_dir():dest=home
 folder=Path(tempfile.mkdtemp(prefix='SMC-Install-Records-',dir=str(dest)));os.chmod(folder,0o700)
 archive=folder/'SMC-Install-Records.zip'
 print('Reading existing install records only. No flashing, network or automatic upload.',flush=True)
 m=collect(home/'Documents/x0x37',archive)
 digest=hashlib.sha256(archive.read_bytes()).hexdigest()
 (folder/'ARCHIVE-SHA256.txt').write_text(digest+'  '+archive.name+'\n')
 (folder/'MANIFEST.json').write_text(json.dumps(m,indent=2))
 print('Created: '+str(archive)+'\nSHA256: '+digest+'\n'+json.dumps(m['counts'])+'\nReview and upload manually. Nothing was sent.',flush=True)
 if sys.platform=='darwin':
  import subprocess
  subprocess.run(['/usr/bin/open','-R',str(archive)],check=False)
if __name__=='__main__':main()
