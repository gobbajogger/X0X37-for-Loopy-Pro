/* SPDX-License-Identifier: GPL-3.0-only */
/* Physical inspection router, not firmware execution. Press-time owners survive view changes. */
const r7InputBase={reset,padDown,release,cancelPad};
function inputOwners(){S.physicalOwners??={};S.whiteRoleOrigins??={};S.whiteRoleTimes??={};}
function inputSyncPads(){S.physicalPadsHeld=Object.entries(S.whiteRoleOrigins).filter(([,v])=>Object.keys(v).some(o=>o.startsWith('pad:'))).map(([n])=>+n);}
function inputKeyMark(physical,origin,down){if(down){S.keyOrigins[physical]??={};S.keyOrigins[physical][origin]=true;}else if(S.keyOrigins[physical]){delete S.keyOrigins[physical][origin];if(!Object.keys(S.keyOrigins[physical]).length)delete S.keyOrigins[physical];}}
function whiteRoleDown(n,origin,browserClock=false){
 inputOwners();const owners=S.whiteRoleOrigins[n]??={};if(owners[origin])return;const already=Object.keys(owners).length;owners[origin]=true;
 if(!already){S.whiteRoleTimes[n]={start:performance.now(),duration:0,browserClock};
  if(S.view==='GEN'&&S.part>2&&!S.heldPatch&&!S.heldSave&&S.overlay!=='ASK'){
   if(typeof genGridOwner==='function'&&genGridOwner())genWhiteDown(n);
   else if(S.rec||S.freePlay[S.sourcePart-2]){const physical=5+WHITE_LOGICAL[n];S.padKeyOwners[n]=physical;noteDown(physical,100,'pad:'+n);}
  }else r7InputBase.padDown(n);
 }
 if(browserClock&&S.whiteRoleTimes[n])S.whiteRoleTimes[n].browserClock=true;inputSyncPads();render();
}
function whiteRoleUp(n,origin,elapsed=80,cancel=false){
 inputOwners();const owners=S.whiteRoleOrigins[n];if(!owners?.[origin])return false;delete owners[origin];const timing=S.whiteRoleTimes[n];if(timing)timing.duration=Math.max(timing.duration,elapsed,timing.browserClock?performance.now()-timing.start:0);
 inputSyncPads();if(Object.keys(owners).length){render();return true;}delete S.whiteRoleOrigins[n];delete S.whiteRoleTimes[n];
 if(S.press?.genFocus&&S.press.input===n&&typeof genFinishFocus==='function')genFinishFocus(S.press);
 if(cancel)r7InputBase.cancelPad(n);else r7InputBase.release(timing?.duration??elapsed,n);inputSyncPads();return true;
}
padDown=function(n,browserClock=false){if(!Number.isInteger(n)||n<0||n>=16)return;whiteRoleDown(n,'pad:'+n,browserClock);};
release=function(elapsed=800,n=null){const input=n??S.lastPadInput??S.press?.input;if(input!==null&&input!==undefined&&whiteRoleUp(input,'pad:'+input,elapsed))return;return r7InputBase.release(elapsed,n);};
cancelPad=function(n){if(!whiteRoleUp(n,'pad:'+n,0,true))return r7InputBase.cancelPad(n);};
function physicalDown(physical,browserClock=false){
 if(!Number.isInteger(physical)||physical<0||physical>36)return;inputOwners();if(S.physicalOwners[physical])return;
 const logical=physical-5,w=WHITE_LOGICAL.indexOf(logical),b=BLACK_LOGICAL.indexOf(logical),central=logical>=0&&logical<=26,synth=S.part>2;
 const live37=synth&&!S.rec&&!S.heldPatch&&!S.heldSave&&['PART','SEQ'].includes(S.view)&&(S.view==='PART'||S.freePlay[S.sourcePart-2]);
 const kbd=synth&&(S.view==='PART'||S.rec||S.freePlay[S.sourcePart-2]);let owner={kind:'none',start:performance.now(),browserClock};S.physicalOwners[physical]=owner;
 if(S.clipOn||S.overlay==='ASK'){S.note='Physical owner masked/unmodeled host context; no local pattern write';render();return;}
 if(!central){if(live37){owner.kind='note';noteDown(physical);}else{S.note='Outer key has no admitted legacy GRID role';render();}return;}
 if(w>=0&&(S.heldPatch||S.heldSave||['HOME','SONG'].includes(S.view)||mixKit()>=0||!kbd)){
  if(['HOME','SONG','SEQ','PART','GEN'].includes(S.view)||mixKit()>=0){owner.kind='white';owner.n=w;inputKeyMark(physical,'physical-grid',true);whiteRoleDown(w,'key:'+physical,browserClock);return;}
 }
 if(kbd&&['PART','SEQ','GEN'].includes(S.view)){owner.kind='note';noteDown(physical);return;}
 if(b>=0&&(S.part<3||['HOME','SONG','MIX'].includes(S.view))){owner.kind='black';owner.n=b;blackDown(b);return;}
 S.note='Physical key has no action in this source context';render();
}
function physicalUp(physical,elapsed){
 inputOwners();const owner=S.physicalOwners[physical];if(!owner)return;delete S.physicalOwners[physical];const duration=elapsed??(owner.browserClock?performance.now()-owner.start:80);
 if(owner.kind==='white'){inputKeyMark(physical,'physical-grid',false);whiteRoleUp(owner.n,'key:'+physical,duration);}
 else if(owner.kind==='note')noteUp(physical);else if(owner.kind==='black')blackUp(owner.n);render();
}
function physicalCancel(physical){inputOwners();const owner=S.physicalOwners[physical];if(!owner)return;delete S.physicalOwners[physical];if(owner.kind==='white'){inputKeyMark(physical,'physical-grid',false);whiteRoleUp(owner.n,'key:'+physical,0,true);}else if(owner.kind==='note')noteUp(physical);else if(owner.kind==='black')blackUp(owner.n);render();}
function cancelButton(name){S.buttons=S.buttons.filter(x=>x!==name);S.usedButtons=S.usedButtons.filter(x=>x!==name);if(name==='BT')S.heldSave=false;if(name==='PATCH')S.heldPatch=false;if(!S.buttons.includes('FX')&&!S.buttons.includes('SEQ'))S.panicLatched=false;render();}
Object.assign(window.proposal,{padDown,release,cancelPad,physicalDown,physicalUp,physicalCancel,cancelButton});
