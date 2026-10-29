// SPDX-License-Identifier: GPL-3.0-only
// Optimistic local-storage version guard for ordinary stale-tab overwrites.
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
