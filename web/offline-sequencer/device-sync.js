// SPDX-License-Identifier: GPL-3.0-only
import {openProject,requireProjectModel} from './project-adapter.js';

const unavailable=()=>Object.assign(new Error(
  'Owner-bound revisioned snapshots and atomic compare-and-write are unavailable in this firmware protocol. Use an exported project file; no device command was sent.'
),{code:'unsupported-atomic-sync'});

// B1 retains MIDI selection/connection. This controller neither requests MIDI
// ports nor identifies a keyboard from its USB display name.
export class DeviceSync {
  #store;
  #device=null;
  #model=null;
  #generation=0;
  #projects=new WeakMap();
  #previews=new WeakMap();
  constructor(store) {
    if(!store||!Number.isSafeInteger(store.revision)||typeof store.replace!=='function')
      throw new Error('A local DraftStore is required.');
    this.#store=store;
  }
  attach(device,{model}={}) {
    requireProjectModel(model);
    if(!device||typeof device.identity!=='string'||!/^X0X [\x20-\x7e]{1,20}$/.test(device.identity)||!device.link)
      throw new Error('Supply an already identified ProjectDevice from the connection owner.');
    this.#device=device;this.#model=model;this.#generation++;
    // Reconnection never loads a project or resets local edits/history.
  }
  disconnect() {this.#device=null;this.#model=null;this.#generation++;}
  preview(bytes,{model=this.#model}={}) {
    const project=openProject(bytes,{model});
    const token=Object.freeze({project,revision:this.#store.revision,model:project.model});
    this.#previews.set(token,{project,revision:this.#store.revision,generation:this.#generation});
    return token;
  }
  applyRead(token) {
    const pending=this.#previews.get(token);
    if(!pending||pending.revision!==this.#store.revision||pending.generation!==this.#generation)
      throw new Error('Local draft or connection changed after project preview; preview again.');
    const changed=this.#store.replace(pending.project.draft);
    this.#projects.set(this.#store.doc,pending.project);this.#previews.delete(token);
    return changed;
  }
  edit(mutator) {
    const project=this.#projects.get(this.#store.doc);
    if(!project)throw new Error('This draft has no accepted project origin. Open a project first.');
    const changed=this.#store.edit(mutator);
    if(changed)this.#projects.set(this.#store.doc,project);
    return changed;
  }
  exportProject() {
    const project=this.#projects.get(this.#store.doc);
    if(!project)throw new Error('This draft has no accepted project origin. Open a project first.');
    return project.exportDraft(this.#store.doc);
  }
  async read() {throw unavailable();}
  async write() {throw unavailable();}
}
