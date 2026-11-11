// SPDX-License-Identifier: GPL-3.0-only
// Community entries are untrusted data. This module never sends device commands.
import {SCHEMA,copy,validateDraft,diffDraft} from './model.js';

export const LIBRARY_SCHEMA='dnbox-community-library/1';
export const LIBRARY_LIMIT=262144;
export const SONG_LIMIT=1048576;
const LICENSES=new Set(['CC0-1.0','CC-BY-4.0','CC-BY-SA-4.0','GPL-3.0-only']);
const encoder=new TextEncoder();
const exactKeys=(value,keys)=>value&&typeof value==='object'&&!Array.isArray(value)&&
  Object.keys(value).length===keys.length&&keys.every(key=>Object.hasOwn(value,key));
const text=(value,max)=>typeof value==='string'&&value.length>0&&value.length<=max&&
  value===value.trim()&&!/[\u0000-\u001f\u007f-\u009f\u202a-\u202e\u2066-\u2069]/u.test(value);

function boundedText(raw,limit,label){
  if(typeof raw!=='string'||raw.length>limit||encoder.encode(raw).length>limit)
    throw Error(label+' size limit exceeded.');
  return raw;
}
function checkPath(path){
  if(typeof path!=='string'||!/^community-songs\/[a-z0-9][a-z0-9-]{0,63}\.json$/.test(path))
    throw Error('Song path must name a JSON file in this editor’s community-songs directory.');
  return path;
}
function trustedBase(baseURL){
  const base=new URL(baseURL);
  const local=['localhost','127.0.0.1','[::1]'].includes(base.hostname);
  if(base.username||base.password||!(base.protocol==='https:'||base.protocol==='http:'&&local))
    throw Error('Open the editor over HTTPS or localhost.');
  return base;
}
export function resolveSongURL(path,baseURL){
  const base=trustedBase(baseURL),url=new URL(checkPath(path),new URL('.',base));
  if(url.origin!==base.origin)throw Error('Song source is outside this editor.');
  return url.href;
}

export function parseLibrary(raw){
  const library=JSON.parse(boundedText(raw,LIBRARY_LIMIT,'Catalog'));
  if(!exactKeys(library,['schema','songs'])||library.schema!==LIBRARY_SCHEMA||!Array.isArray(library.songs))
    throw Error('Unsupported community library schema.');
  if(library.songs.length>200)throw Error('Catalog entry limit exceeded.');
  const ids=new Set();
  const songs=library.songs.map(song=>{
    if(!exactKeys(song,['id','title','author','license','tags','format','sha256','path'])||
      !text(song.id,64)||!/^[a-z0-9][a-z0-9-]*$/.test(song.id)||!text(song.title,96)||
      !text(song.author,80)||!LICENSES.has(song.license)||song.format!==SCHEMA||
      !Array.isArray(song.tags)||song.tags.length>8||!song.tags.every(tag=>text(tag,24))||
      typeof song.sha256!=='string'||!/^[0-9a-f]{64}$/.test(song.sha256))
      throw Error('Invalid song metadata, rights, format or checksum.');
    checkPath(song.path);
    if(ids.has(song.id))throw Error('Duplicate song identifier.');
    ids.add(song.id);
    return Object.freeze({...song,tags:Object.freeze([...song.tags])});
  });
  return Object.freeze({schema:LIBRARY_SCHEMA,songs:Object.freeze(songs)});
}

export function searchSongs(library,query=''){
  const words=String(query).slice(0,160).normalize('NFKC').toLowerCase().trim().split(/\s+/).filter(Boolean);
  return library.songs.filter(song=>{
    const haystack=[song.title,song.author,...song.tags].join(' ').normalize('NFKC').toLowerCase();
    return words.every(word=>haystack.includes(word));
  });
}

function supportedDraft(raw){
  const draft=JSON.parse(boundedText(raw,SONG_LIMIT,'Song'));
  // Do not call the legacy draft importer: library loads must never migrate data.
  if(draft?.schema!==SCHEMA)throw Error('Unsupported song draft schema; no migration was performed.');
  return validateDraft(draft);
}

async function responseText(response,limit){
  if(!response.ok)throw Error('Library download failed (HTTP '+response.status+').');
  const declared=response.headers.get('content-length');
  if(declared!==null&&(!/^\d+$/.test(declared)||Number(declared)>limit))throw Error('Download size limit exceeded.');
  if(!response.body?.getReader)throw Error('A bounded streaming download is unavailable.');
  const reader=response.body.getReader(),chunks=[];let size=0;
  try{
    for(;;){
      const {done,value}=await reader.read();if(done)break;
      size+=value.byteLength;
      if(size>limit)throw Error('Download size limit exceeded.');
      chunks.push(value);
    }
  }catch(error){
    try{await reader.cancel();}catch{/* Keep the original download error. */}
    throw error;
  }finally{reader.releaseLock();}
  const bytes=new Uint8Array(size);let at=0;
  for(const chunk of chunks){bytes.set(chunk,at);at+=chunk.byteLength;}
  return {bytes,raw:new TextDecoder('utf-8',{fatal:true}).decode(bytes)};
}

export function createLibraryController({getDraft,getRevision,replaceDraft,
  fetchImpl=globalThis.fetch,cryptoImpl=globalThis.crypto,baseURL=new URL('.',import.meta.url).href}={}){
  if(![getDraft,getRevision,replaceDraft].every(fn=>typeof fn==='function'))
    throw Error('The library requires the editor’s current draft, revision and replacement action.');
  let library=parseLibrary('{"schema":"dnbox-community-library/1","songs":[]}');
  let epoch=0,request=null,pending=null;
  function cancel(){epoch++;request?.abort();request=null;pending=null;}
  function begin(){cancel();return {epoch,revision:getRevision()};}
  function active(token,checkRevision=true){
    if(token.epoch!==epoch)throw Error('Library selection was cancelled or replaced.');
    if(checkRevision&&token.revision!==getRevision())throw Error('Draft changed during library selection; preview again.');
  }
  async function download(url,limit){
    if(typeof fetchImpl!=='function')throw Error('Library downloads are unavailable.');
    const controller=new AbortController();request=controller;
    try{return await responseText(await fetchImpl(url,{mode:'same-origin',credentials:'omit',
      redirect:'error',cache:'no-store',signal:controller.signal}),limit);}
    finally{if(request===controller)request=null;}
  }
  function preview(candidate,details,token){
    active(token);
    const summary=Object.freeze({...details,changes:diffDraft(getDraft(),candidate).length});
    pending={summary,candidate:copy(candidate),token};
    return summary;
  }
  return {
    get catalog(){return library;},
    search(query){return searchSongs(library,query);},
    cancel,
    importCatalog(raw){
      const candidate=parseLibrary(raw);cancel();library=candidate;return library;
    },
    async loadCatalog(){
      const token=begin(),base=trustedBase(baseURL);
      const {raw}=await download(new URL('community-library.json',new URL('.',base)).href,LIBRARY_LIMIT);
      active(token,false);const candidate=parseLibrary(raw);library=candidate;return library;
    },
    async prepareSong(id){
      const song=library.songs.find(entry=>entry.id===id);
      if(!song)throw Error('Select a song from this catalog.');
      const url=resolveSongURL(song.path,baseURL),token=begin();
      const {bytes,raw}=await download(url,SONG_LIMIT);active(token);
      if(!cryptoImpl?.subtle)throw Error('Secure song verification is unavailable.');
      const digest=new Uint8Array(await cryptoImpl.subtle.digest('SHA-256',bytes));active(token);
      const hash=Array.from(digest,value=>value.toString(16).padStart(2,'0')).join('');
      if(hash!==song.sha256)throw Error('Song checksum does not match this catalog. Draft unchanged.');
      return preview(supportedDraft(raw),{title:song.title,author:song.author,license:song.license,source:'Verified community catalog'},token);
    },
    prepareLocalDraft(raw,name='Local draft'){
      const candidate=supportedDraft(raw);
      const title=typeof name==='string'?name.slice(0,96):'Local draft';
      return preview(candidate,{title,author:'Local file',license:'Not supplied; personal import only',source:'Local file'},begin());
    },
    commit(summary){
      if(!pending||pending.summary!==summary)throw Error('Library preview expired; select the song again.');
      const next=pending;active(next.token);
      const changed=replaceDraft(copy(next.candidate));
      cancel();return changed;
    }
  };
}
