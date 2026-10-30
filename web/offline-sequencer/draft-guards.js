// SPDX-License-Identifier: GPL-3.0-only
// LocalStorage has atomic single writes, but no multi-key transaction.
// Serialize read/compare/backup/primary writes across same-origin browser tabs.
export class StorageRevision {
  constructor(raw=null){this.raw=raw;}
  save(storage,key,doc){
    if(storage.getItem(key)!==this.raw)return false;
    const next=JSON.stringify(doc);
    storage.setItem(key,next);
    this.raw=next;
    return true;
  }
  changed(newRaw){return newRaw!==this.raw;}
}
export class SelectionEpoch {
  constructor(){this.current=0;}
  next(){return ++this.current;}
  valid(token){return this.current===token;}
}

export const DRAFT_STORAGE_KEY='dnbox-offline-draft-v2';
export class SafeDraftStorage {
  constructor(storage,{parseDraft,validateDraft,locks},key=DRAFT_STORAGE_KEY){
    if(typeof parseDraft!=='function'||typeof validateDraft!=='function')throw Error('Draft validators are required.');
    this.storage=storage;
    this.parseDraft=parseDraft;
    this.validateDraft=validateDraft;
    this.locks=locks;
    this.key=key;
    this.backupKey=key+':last-good';
    this.primaryRaw=null;
    this.backupRaw=null;
    this.status='unread';
  }
  _read(key){
    const raw=this.storage.getItem(key);
    if(raw!==null&&typeof raw!=='string')throw Error('Browser storage returned unreadable draft data.');
    return raw;
  }
  _valid(raw){
    if(raw===null)return null;
    try{return this.parseDraft(raw);}catch{return null;}
  }
  _unchanged(){
    if(this._read(this.key)!==this.primaryRaw||this._read(this.backupKey)!==this.backupRaw)
      throw Error('Another tab changed a saved draft. Export this tab before reloading.');
  }
  _exclusive(write){
    if(!this.locks||typeof this.locks.request!=='function')
      throw Error('Exclusive draft saving requires a secure HTTPS or localhost browser with Web Locks. Export the local draft; saved storage was not changed.');
    return this.locks.request(this.key+':exclusive-write',{mode:'exclusive'},write);
  }
  load(){
    try{
      const primary=this._read(this.key),backup=this._read(this.backupKey);
      this.primaryRaw=primary;
      this.backupRaw=backup;
      const goodPrimary=this._valid(primary),goodBackup=this._valid(backup);
      if(goodPrimary){
        // Loading is read-only: automatic backup seeding could race a second tab.
        // The first locked save will preserve this validated primary as last-good.
        this.status='ready';
        return {status:'ready',draft:goodPrimary,warning:goodBackup?'':'Last-good recovery copy will be established before the next save.'};
      }
      if(primary===null&&backup===null){
        this.status='ready';
        return {status:'new',draft:null,warning:''};
      }
      if(goodBackup){
        this.status='recovery';
        return {status:'recovery',draft:goodBackup,warning:'The main saved draft is corrupt or missing. The last good copy is available; confirm recovery before editing.'};
      }
      this.status='locked';
      return {status:'locked',draft:null,warning:'Saved draft data is unreadable, with no verified recovery copy. Storage writes are blocked; existing bytes are preserved.'};
    }catch{
      this.status='locked';
      return {status:'locked',draft:null,warning:'Browser draft storage could not be read. No saved data was changed; writes are blocked.'};
    }
  }
  async save(doc){
    if(this.status!=='ready')throw Error('Saved draft storage is locked. Recover a validated backup first, or export this tab without replacing saved data.');
    this.validateDraft(doc);
    const candidate=JSON.stringify(doc);
    this.parseDraft(candidate); // Includes schema, bounds and 1 MiB size check.
    return this._exclusive(()=>{
      if(this.status!=='ready')throw Error('Saved draft storage is locked.');
      // All comparisons and writes are inside one origin-wide exclusive Web Lock.
      this._unchanged();
      if(this.primaryRaw!==null){
        this.parseDraft(this.primaryRaw); // Never rotate an unreadable primary into last-good.
        this.storage.setItem(this.backupKey,this.primaryRaw);
        this.backupRaw=this.primaryRaw;
      }else if(this.backupRaw===null){
        this.storage.setItem(this.backupKey,candidate);
        this.backupRaw=candidate;
      }else{
        this.parseDraft(this.backupRaw);
      }
      this.storage.setItem(this.key,candidate);
      this.primaryRaw=candidate;
      return true;
    });
  }
  async restore(){
    if(this.status!=='recovery')throw Error('No validated backup is awaiting recovery.');
    return this._exclusive(()=>{
      if(this.status!=='recovery')throw Error('No validated backup is awaiting recovery.');
      this._unchanged();
      const document=this.parseDraft(this.backupRaw);
      // A failed write leaves the separately validated backup untouched.
      this.storage.setItem(this.key,this.backupRaw);
      this.primaryRaw=this.backupRaw;
      this.status='ready';
      return document;
    });
  }
  changed(key,raw){
    return key===this.key?raw!==this.primaryRaw:key===this.backupKey?raw!==this.backupRaw:false;
  }
}
