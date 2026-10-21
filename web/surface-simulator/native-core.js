/* SPDX-License-Identifier: GPL-3.0-only */
export class FirmwareCore {
  constructor(base='') { this.base=base; this.pending=Promise.resolve(); }
  async info(){const r=await fetch(this.base+'/api/info');if(!r.ok)throw Error('Native firmware helper unavailable');return r.json();}
  command(op,args=[]){
    const task=this.pending.then(async()=>{const r=await fetch(this.base+'/api/command',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({op,args})});const data=await r.json();if(!r.ok)throw Error(data.error);return data;});
    this.pending=task.catch(()=>{});return task;
  }
  input(kind,id,value){return this.command('input',[kind,id,value]);}
  advance(ms){return this.command('advance',[ms]);}
  releaseAll(){return this.input(5,0,0);}
  async state(){await this.pending;return (await fetch(this.base+'/api/state')).json();}
  frameURL(){return this.base+'/api/frame.png?t='+Date.now();}
}
