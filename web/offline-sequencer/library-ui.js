// SPDX-License-Identifier: GPL-3.0-only
import {LIBRARY_LIMIT,SONG_LIMIT} from './song-library.js';

// Mount inside the editor's existing modal. Every untrusted label uses textContent.
// The caller disposes this view when that modal closes or changes context.
export function mountLibrary({host,controller,notify=()=>{},onCommitted=()=>{},autoLoad=true}){
  if(!host||!controller)throw Error('A library host and controller are required.');
  const document=host.ownerDocument,root=document.createElement('section');
  root.className='community-library';host.replaceChildren(root);
  let disposed=false,viewEpoch=0,query='';
  const node=(tag,text,attrs={})=>{
    const element=document.createElement(tag);
    if(text!==undefined)element.textContent=text;
    for(const [name,value] of Object.entries(attrs))element.setAttribute(name,String(value));
    return element;
  };
  const button=(label,action)=>{
    const element=node('button',label,{type:'button'});
    element.addEventListener('click',event=>{if(!disposed)action(event);});return element;
  };
  const valid=token=>!disposed&&viewEpoch===token;
  function error(reason){
    if(disposed)return;
    const label=reason instanceof Error?reason.message:String(reason);
    let status=root.querySelector('[data-library-status]');
    if(!status){status=node('p','',{'data-library-status':'',role:'status','aria-live':'polite'});root.append(status);}
    status.textContent=label;notify(label,true);
  }
  function begin(){viewEpoch++;controller.cancel();return viewEpoch;}
  function preview(summary){
    root.replaceChildren(node('h3','Preview: '+summary.title));
    root.append(node('p',summary.author+' · '+summary.license));
    root.append(node('p',summary.source+' · '+summary.changes+' changed fields.'));
    root.append(node('p','This replaces the complete local draft: all 32 patterns, five parts, sound drafts and engine choices. One Undo restores the previous draft. It does not load a device project, song chain or samples.'));
    const actions=node('div',undefined,{class:'actions'});
    actions.append(button('Replace local draft',()=>{
      try{
        const changed=controller.commit(summary);viewEpoch++;
        notify(changed?'Library draft loaded locally as one Undo transaction. Nothing sent to a device.':'This draft already matches the selected song.');
        onCommitted(changed);
      }catch(reason){error(reason);}
    }),button('Back to library',()=>{begin();browse();}));
    root.append(actions);
  }
  function filePicker(label,limit,onRead){
    const wrapper=node('label',label,{class:'file-button'});
    const input=node('input',undefined,{type:'file',accept:'.json,application/json'});
    input.addEventListener('change',async()=>{
      const file=input.files?.[0];if(!file)return;
      const token=begin();
      try{
        if(!Number.isSafeInteger(file.size)||file.size>limit)throw Error('Selected file exceeds its size limit.');
        const raw=await file.text();if(!valid(token))return;
        onRead(raw,file.name);
      }catch(reason){if(valid(token))error(reason);}
      finally{input.value='';}
    });
    wrapper.append(input);return wrapper;
  }
  function browse(){
    if(disposed)return;
    root.replaceChildren(node('p','Browse contributor-labelled local drafts. Catalog hashes verify downloaded bytes; contributor and license claims still require review. No upload or publication occurs here.'));
    const search=node('input',undefined,{type:'search','aria-label':'Search community songs',placeholder:'Title, author or tag',maxlength:160});
    search.value=query;
    const results=node('div',undefined,{class:'library-results'});
    const status=node('p','',{'data-library-status':'',role:'status','aria-live':'polite'});
    const actions=node('div',undefined,{class:'actions'});
    actions.append(filePicker('Open local draft',SONG_LIMIT,(raw,name)=>preview(controller.prepareLocalDraft(raw,name))),
      filePicker('Open local catalog',LIBRARY_LIMIT,raw=>{controller.importCatalog(raw);query='';browse();}),
      button('Reload bundled catalog',()=>loadCatalog()));
    function renderResults(){
      results.replaceChildren();const songs=controller.search(query);
      status.textContent=controller.catalog.songs.length===0?
        'No community songs are bundled yet. Open a local draft, or load a catalog with reviewed metadata and rights.':
        songs.length+' matching song'+(songs.length===1?'':'s')+'.';
      for(const song of songs){
        const item=node('article');item.append(node('h3',song.title),node('p',song.author+' · '+song.license),node('p',song.tags.join(' · ')));
        item.append(button('Preview '+song.title,async()=>{
          const token=begin();status.textContent='Downloading and verifying '+song.title+'…';
          try{const summary=await controller.prepareSong(song.id);if(valid(token))preview(summary);}
          catch(reason){if(valid(token))error(reason);}
        }));results.append(item);
      }
    }
    search.addEventListener('input',()=>{begin();query=search.value;renderResults();});
    root.append(search,actions,status,results);renderResults();
  }
  async function loadCatalog(){
    if(disposed)return;
    const token=begin();
    try{await controller.loadCatalog();if(valid(token)){query='';browse();}}
    catch(reason){if(valid(token))error(reason);}
  }
  browse();if(autoLoad)void loadCatalog();
  return {
    dispose(){if(disposed)return;disposed=true;viewEpoch++;controller.cancel();},
    reload:loadCatalog
  };
}
