// SPDX-License-Identifier: GPL-3.0-only
// LocalStorage has atomic individual writes, but no multi-key transaction.
// Preserve a validated fallback before replacing any previously validated primary.
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
  constructor(storage,{parseDraft,validateDraft},key=DRAFT_STORAGE_KEY){
    if(typeof parseDraft!=='function'||typeof validateDraft!=='function')throw Error('Draft validators are required.');
    this.storage=storage;
    this.parseDraft=parseDraft;
    this.validateDraft=validateDraft;
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
  load(){
    try{
      const primary=this._read(this.key),backup=this._read(this.backupKey);
      this.primaryRaw=primary;
      this.backupRaw=backup;
      const goodPrimary=this._valid(primary),goodBackup=this._valid(backup);
      if(goodPrimary){
        this.status='ready';
        let warning='';
        if(!goodBackup){
          // Seed the fallback only from data that has already passed full validation.
          // This never overwrites the only valid copy (the primary).
          try{
            this._unchanged();
            this.storage.setItem(this.backupKey,primary);
            this.backupRaw=primary;
          }catch{
            warning='The draft loaded, but its safety copy could not be written. Export a copy.';
          }
        }
        return {status:'ready',draft:goodPrimary,warning};
      }
      if(primary===null&&backup===null){
        this.status='ready';
        return {status:'new',draft:null,warning:''};
      }
      if(goodBackup){
        // Leave the corrupt/missing primary and the last good backup untouched.
        // Only restore() may replace the primary, after a deliberate user action.
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
  save(doc){
    if(this.status!=='ready')throw Error('Saved draft storage is locked. Recover a validated backup first, or export this tab without replacing saved data.');
    // Reject malformed runtime documents before JSON.stringify can drop invalid fields.
    this.validateDraft(doc);
    const candidate=JSON.stringify(doc);
    this.parseDraft(candidate); // Includes schema, bounds and 1 MiB size check.
    this._unchanged();
    if(this.primaryRaw!==null){
      this.parseDraft(this.primaryRaw); // Never rotate an unreadable primary into last-good.
      this.storage.setItem(this.backupKey,this.primaryRaw);
      this.backupRaw=this.primaryRaw;
    }else if(this.backupRaw===null){
      // First-ever save: write the validated candidate to recovery storage first.
      this.storage.setItem(this.backupKey,candidate);
      this.backupRaw=candidate;
    }else{
      this.parseDraft(this.backupRaw); // Retain valid fallback from an interrupted first save.
    }
    if(this._read(this.key)!==this.primaryRaw)
      throw Error('Another tab changed the saved draft during backup. Primary was not overwritten.');
    this.storage.setItem(this.key,candidate);
    this.primaryRaw=candidate;
    return true;
  }
  restore(){
    if(this.status!=='recovery')throw Error('No validated backup is awaiting recovery.');
    this._unchanged();
    const document=this.parseDraft(this.backupRaw);
    // One atomic key write. A failure leaves the validated backup untouched.
    this.storage.setItem(this.key,this.backupRaw);
    this.primaryRaw=this.backupRaw;
    this.status='ready';
    return document;
  }
  changed(key,raw){
    return key===this.key?raw!==this.primaryRaw:key===this.backupKey?raw!==this.backupRaw:false;
  }
}
