// SPDX-License-Identifier: GPL-3.0-only
import test from 'node:test';
import assert from 'node:assert/strict';
import {webcrypto} from 'node:crypto';
import {createDraft,DraftStore} from './model.js';
import {parseLibrary,searchSongs,resolveSongURL,createLibraryController} from './song-library.js';

const baseURL='https://example.test/editor/index.html';
const entry=(extra={})=>({id:'first-song',title:'First song',author:'Contributor',license:'CC0-1.0',tags:['acid','slow'],format:'dnbox-sequencer-draft/2',sha256:'0'.repeat(64),path:'community-songs/first-song.json',...extra});
const catalog=songs=>JSON.stringify({schema:'dnbox-community-library/1',songs});
const rawDraft=note=>{const d=createDraft();d.patterns[0].bass[0].steps[63]={note,flags:1,velocity:0};return JSON.stringify(d);};
async function digest(text){return [...new Uint8Array(await webcrypto.subtle.digest('SHA-256',new TextEncoder().encode(text)))].map(x=>x.toString(16).padStart(2,'0')).join('');}
function fixture(options={}){
  const store=new DraftStore();
  const controller=createLibraryController({getDraft:()=>store.doc,getRevision:()=>store.revision,
    replaceDraft:doc=>store.replace(doc),cryptoImpl:webcrypto,baseURL,...options});
  return {store,controller};
}

// Removing strict entry validation would permit unreviewed sources or unlabelled rights.
test('catalog bounds, duplicate IDs, unknown formats and incomplete rights fail before browsing',()=>{
  assert.equal(parseLibrary(catalog([])).songs.length,0);
  assert.throws(()=>parseLibrary(catalog([entry(),entry()])),/duplicate/i);
  for(const patch of [{license:''},{license:'all rights reserved'},{format:'device-project/13'},
    {title:'hidden\u202Etitle'},{tags:Array(9).fill('acid')},{sha256:'not-a-hash'},{path:'../backup.json'}]){
    assert.throws(()=>parseLibrary(catalog([entry(patch)])));
  }
  assert.throws(()=>parseLibrary(catalog([entry({unrecognised:true})])));
  assert.throws(()=>parseLibrary(' '.repeat(262145)),/limit/i);
  assert.throws(()=>parseLibrary(catalog(Array.from({length:201},(_,i)=>entry({id:'song-'+i})))),/limit/i);
});

test('search covers title, author and tags and treats markup as text',()=>{
  const library=parseLibrary(catalog([entry({title:'<img src=x> Acid',author:'Ada'}),entry({id:'second',title:'Other',author:'Ben',tags:['fast']})]));
  assert.deepEqual(searchSongs(library,'ADA').map(x=>x.id),['first-song']);
  assert.deepEqual(searchSongs(library,'fast').map(x=>x.id),['second']);
  assert.deepEqual(searchSongs(library,'acid ada').map(x=>x.id),['first-song']);
  assert.equal(searchSongs(library,'<img')[0].title,'<img src=x> Acid');
});

// No manifest may select an arbitrary host, query, parent directory or encoded path.
test('song addresses stay within the editor catalog directory and secure origins',()=>{
  assert.equal(resolveSongURL('community-songs/first-song.json',baseURL),'https://example.test/editor/community-songs/first-song.json');
  assert.equal(resolveSongURL('community-songs/a.json','http://localhost:8000/editor/'),'http://localhost:8000/editor/community-songs/a.json');
  for(const path of ['https://evil.test/x.json','//evil.test/x.json','../x.json','/community-songs/x.json',
    'community-songs/%2e%2e.json','community-songs/x.json?token=x','community-songs/x.json#part',
    'community-songs/x\\y.json'])assert.throws(()=>resolveSongURL(path,baseURL));
  assert.throws(()=>resolveSongURL('community-songs/a.json','http://example.test/editor/'));
});

test('verified library preview preserves hidden raw cells and commits exactly one Undo transaction',async()=>{
  const raw=rawDraft(73),hash=await digest(raw),{store,controller}=fixture({fetchImpl:async()=>new Response(raw)});
  controller.importCatalog(catalog([entry({sha256:hash})]));
  const preview=await controller.prepareSong('first-song');
  assert.equal(store.history.length,0);assert.equal(store.doc.patterns[0].bass[0].steps[63].flags,0);
  assert.equal(preview.title,'First song');assert.ok(preview.changes>0);
  assert.equal(controller.commit(preview),true);
  assert.deepEqual(store.doc.patterns[0].bass[0].steps[63],{note:73,flags:1,velocity:0});
  assert.equal(store.history.length,1);store.undo();
  assert.equal(store.doc.patterns[0].bass[0].steps[63].flags,0);
  assert.throws(()=>controller.commit(preview),/preview/i);
});

test('corrupt payload, invalid schema and oversized streaming responses cannot replace a draft',async()=>{
  for(const [raw,hash] of [[rawDraft(70),'0'.repeat(64)],['{}',await digest('{}')]]){
    const {store,controller}=fixture({fetchImpl:async()=>new Response(raw)}),before=structuredClone(store.doc);
    controller.importCatalog(catalog([entry({sha256:hash})]));
    await assert.rejects(controller.prepareSong('first-song'));
    assert.deepEqual(store.doc,before);assert.equal(store.history.length,0);
  }
  const {store,controller}=fixture({fetchImpl:async()=>new Response(new ReadableStream({start(c){c.enqueue(new Uint8Array(1048577));c.close();}}))});
  controller.importCatalog(catalog([entry()]));
  await assert.rejects(controller.prepareSong('first-song'),/limit/i);
  assert.equal(store.history.length,0);
});

test('remote failure and unsafe imported catalogs do not invoke mutation or fetch untrusted hosts',async()=>{
  let fetched=0;
  const {store,controller}=fixture({fetchImpl:async(url,options)=>{
    fetched++;assert.equal(url,'https://example.test/editor/community-songs/first-song.json');
    assert.equal(options.redirect,'error');assert.equal(options.credentials,'omit');
    return new Response('',{status:404});
  }});
  assert.throws(()=>controller.importCatalog(catalog([entry({path:'https://evil.test/x.json'})])));
  assert.equal(fetched,0);controller.importCatalog(catalog([entry()]));
  await assert.rejects(controller.prepareSong('first-song'),/404/);
  assert.equal(store.history.length,0);assert.equal(fetched,1);
});

test('changed draft and cancelled selection invalidate delayed downloads and old confirmations',async()=>{
  let finish;const raw=rawDraft(64),hash=await digest(raw);
  const {store,controller}=fixture({fetchImpl:()=>new Promise(resolve=>{finish=resolve;})});
  controller.importCatalog(catalog([entry({sha256:hash})]));
  const delayed=controller.prepareSong('first-song');
  store.edit(d=>{d.patterns[0].bass[0].length=20;});finish(new Response(raw));
  await assert.rejects(delayed,/changed/i);
  assert.equal(store.doc.patterns[0].bass[0].length,20);
  const preview=controller.prepareLocalDraft(raw,'local.json');
  controller.cancel();assert.throws(()=>controller.commit(preview),/preview/i);
  const next=controller.prepareLocalDraft(raw,'local.json');
  store.edit(d=>{d.patterns[0].bass[0].length=21;});
  assert.throws(()=>controller.commit(next),/changed/i);
  assert.equal(store.doc.patterns[0].bass[0].length,21);
});

test('preview candidate cannot be forged or tampered through public summary and old schema is never migrated',()=>{
  const {store,controller}=fixture();const preview=controller.prepareLocalDraft(rawDraft(90),'local.json');
  assert.throws(()=>controller.commit({...preview}),/preview/i);
  assert.ok(Object.isFrozen(preview));
  assert.equal(Object.hasOwn(preview,'candidate'),false);
  assert.throws(()=>controller.prepareLocalDraft(JSON.stringify({...createDraft(),schema:'dnbox-sequencer-draft/1'}),'old.json'),/schema/i);
  controller.commit(preview);assert.equal(store.doc.patterns[0].bass[0].steps[63].note,90);
});

test('a later local preview invalidates an earlier in-flight library selection',async()=>{
  let finish;const raw=rawDraft(64),hash=await digest(raw);
  const {store,controller}=fixture({fetchImpl:()=>new Promise(resolve=>{finish=resolve;})});
  controller.importCatalog(catalog([entry({sha256:hash})]));
  const delayed=controller.prepareSong('first-song');
  const local=controller.prepareLocalDraft(rawDraft(75),'newer.json');finish(new Response(raw));
  await assert.rejects(delayed,/cancelled|selection/i);
  controller.commit(local);assert.equal(store.doc.patterns[0].bass[0].steps[63].note,75);
});

test('bundled catalog reads remain same-origin and cannot replace a newer locally selected catalog',async()=>{
  let finish;
  const {controller}=fixture({fetchImpl:(url,options)=>{
    assert.equal(url,'https://example.test/editor/community-library.json');
    assert.equal(options.mode,'same-origin');
    return new Promise(resolve=>{finish=resolve;});
  }});
  const old=controller.loadCatalog();
  controller.importCatalog(catalog([entry({id:'local-catalog-song'})]));
  finish(new Response(catalog([])));
  await assert.rejects(old,/cancelled|selection/i);
  assert.deepEqual(controller.search('').map(song=>song.id),['local-catalog-song']);
});

test('declared oversized downloads and missing secure checksum support fail closed',async()=>{
  const {controller,store}=fixture({fetchImpl:async()=>new Response('{}',{headers:{'content-length':'1048577'}})});
  controller.importCatalog(catalog([entry()]));
  await assert.rejects(controller.prepareSong('first-song'),/limit/i);
  assert.equal(store.history.length,0);
  const raw=rawDraft(74),other=fixture({cryptoImpl:null,fetchImpl:async()=>new Response(raw)});
  other.controller.importCatalog(catalog([entry()]));
  await assert.rejects(other.controller.prepareSong('first-song'),/verification/i);
  assert.equal(other.store.history.length,0);
});
