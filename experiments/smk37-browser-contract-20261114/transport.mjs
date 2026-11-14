// SPDX-License-Identifier: GPL-3.0-only
// Engineering prototype: the only accepted endpoint is the fake class below.
const fakeEndpoints = new WeakSet();
const bounded = (a, item) => { a.push(item); if (a.length > 128) a.shift(); };
const integer = (value, min, max, label) => {
  if (!Number.isInteger(value) || value < min || value > max) throw new RangeError(label);
  return value;
};

export class FakeMidiEndpoint {
  #up = true;
  #listeners = new Set();
  #sent = [];
  #failures = [];
  #loopback = false;
  constructor({loopback = false} = {}) {
    if (new.target !== FakeMidiEndpoint) throw new Error('fake endpoint subclass is not accepted');
    fakeEndpoints.add(this); this.#loopback = !!loopback; Object.freeze(this);
  }
  get connected() { return this.#up; }
  get sent() { return this.#sent.map(bytes => [...bytes]); }
  subscribe(fn) { this.#listeners.add(fn); return () => this.#listeners.delete(fn); }
  #emit(event) { for (const fn of [...this.#listeners]) fn(event); }
  send(bytes) {
    if (!this.#up) throw new Error('fake endpoint is disconnected');
    const failure = this.#failures.shift();
    if (failure === 'before') throw new Error('injected failure before fake enqueue');
    bounded(this.#sent, [...bytes]);
    if (failure === 'after') throw new Error('injected failure after fake enqueue');
    if (this.#loopback) this.receive(bytes);
  }
  failNext(when = 'before') {
    if (!['before', 'after'].includes(when)) throw new Error('invalid fake failure');
    this.#failures.push(when);
  }
  receive(bytes) { if (this.#up) this.#emit({type:'message', data:[...bytes]}); }
  lose() { if (this.#up) { this.#up = false; this.#emit({type:'disconnect'}); } }
}
Object.freeze(FakeMidiEndpoint.prototype);

export class MidiContract {
  #endpoint = null;
  #unsubscribe = null;
  #generation = 0;
  #armed = false;
  #channel = 2;
  #notes = new Map();
  #sustain = new Set();
  #bend = new Set();
  #running = false;
  #fault = null;
  #uncertain = false;
  #lastIncident = null;
  #feedback = [];
  #log = [];
  #clock;
  #armedAt = 0;
  constructor({now = () => Date.now()} = {}) { this.#clock = now; }
  get status() {
    const connected = !!this.#endpoint?.connected;
    return Object.freeze({state:!connected ? 'disconnected' : this.#fault ? 'fault' : this.#armed ? 'armed' : 'connected-disarmed',
      endpoint:connected ? 'FAKE ONLY' : 'none', channel:this.#channel, armed:this.#armed,
      held:[...this.#notes.keys()], releaseUnconfirmed:this.#uncertain,
      lastIncident:this.#lastIncident, fault:this.#fault, feedback:this.#feedback.map(x=>({...x,bytes:[...x.bytes]})),
      acknowledgement:'none; fake enqueue and received stream are not device state'});
  }
  get log() { return this.#log.map(x=>({...x, ...(x.bytes ? {bytes:[...x.bytes]} : {})})); }
  #record(kind, details = {}) { bounded(this.#log,{kind,...details}); }
  #clearFeedback() { this.#feedback = []; }
  #incident(reason) { this.#uncertain = true; this.#lastIncident = reason; this.#record('unconfirmed',{reason}); }
  #ready() {
    this.poll();
    if (!this.#endpoint?.connected) throw new Error('disconnected');
    if (!this.#armed || this.#fault) throw new Error('explicit arm required');
  }
  #transmit(bytes) {
    this.#record('tx-attempt',{bytes:[...bytes]});
    try { this.#endpoint.send(bytes); }
    catch (error) {
      this.#incident(`Send failed; delivery unknown: ${error.message}`);
      this.#cleanup('send failure');
      this.#fault = 'send failure; reconnect fake endpoint before re-arming';
      throw error;
    }
  }
  #cleanup(reason) {
    this.#armed = false;
    const packets = [];
    for (const [note,ch] of this.#notes) packets.push([0x80+ch-1,note,0]);
    for (const ch of this.#sustain) packets.push([0xB0+ch-1,64,0]);
    for (const ch of this.#bend) packets.push([0xE0+ch-1,0,64]);
    if (this.#running) packets.push([0xFC]);
    // Retire once: a later connection must never receive old obligations.
    this.#notes.clear(); this.#sustain.clear(); this.#bend.clear(); this.#running = false;
    this.#clearFeedback();
    for (const bytes of packets) {
      this.#record('release-attempt',{reason,bytes:[...bytes]});
      try {
        if (!this.#endpoint?.connected) throw new Error('link unavailable');
        this.#endpoint.send(bytes);
      } catch (error) { this.#incident(`Release unconfirmed (${reason}): ${error.message}`); this.#fault = 'release unconfirmed; reconnect fake endpoint before re-arming'; }
    }
    this.#record('disarm',{reason});
  }
  connect(endpoint) {
    if (!fakeEndpoints.has(endpoint) || !endpoint.connected) throw new Error('only a connected FakeMidiEndpoint is accepted');
    if (this.#endpoint) this.disconnect('endpoint replacement');
    this.#endpoint = endpoint; this.#armed = false; this.#fault = null; this.#clearFeedback();
    const generation = ++this.#generation;
    this.#unsubscribe = endpoint.subscribe(event => {
      if (generation !== this.#generation || this.#endpoint !== endpoint) return;
      if (event.type === 'disconnect') this.disconnect('endpoint disappeared');
      else if (event.type === 'message') this.#observe(event.data);
    });
    this.#record('connected',{endpoint:'FAKE ONLY'});
  }
  disconnect(reason = 'explicit disconnect') {
    this.#cleanup(reason);
    ++this.#generation;
    this.#unsubscribe?.(); this.#unsubscribe = null; this.#endpoint = null;
    this.#record('disconnected',{reason});
  }
  arm() {
    this.poll();
    if (!this.#endpoint?.connected || this.#fault) throw new Error('connected healthy fake endpoint required');
    if (this.#armed) return;
    this.#armed = true; this.#armedAt = this.#clock(); this.#record('armed');
  }
  disarm(reason = 'explicit release') { this.#cleanup(reason); }
  setChannel(channel) {
    integer(channel,1,15,'channel must be 1..15; 16 is reserved from this prototype to avoid SMC User interception');
    if (channel === this.#channel) return;
    this.#cleanup('channel changed'); this.#channel = channel; this.#record('channel',{channel});
  }
  noteOn(note, velocity = 100) {
    integer(note,0,127,'note'); integer(velocity,1,127,'velocity'); this.#ready();
    if (this.#notes.has(note)) return false;
    this.#notes.set(note,this.#channel);
    this.#transmit([0x90+this.#channel-1,note,velocity]); return true;
  }
  noteOff(note) {
    integer(note,0,127,'note');
    const channel = this.#notes.get(note); if (!channel) return false;
    // Keep ownership until send returns, so a thrown send triggers a safety OFF.
    this.#transmit([0x80+channel-1,note,0]); this.#notes.delete(note); return true;
  }
  controlChange(controller,value) {
    integer(controller,0,127,'controller'); integer(value,0,127,'CC value');
    if (![1,64].includes(controller)) throw new Error(`unsupported control CC${controller}; no general parameter or panic protocol`);
    this.#ready();
    if (controller === 64) this.#sustain.add(this.#channel);
    this.#transmit([0xB0+this.#channel-1,controller,value]);
    if (controller === 64 && value === 0) this.#sustain.delete(this.#channel);
  }
  pitchBend(value) {
    integer(value,0,16383,'pitch bend'); this.#ready(); this.#bend.add(this.#channel);
    this.#transmit([0xE0+this.#channel-1,value & 127,value >> 7]);
    if (value === 8192) this.#bend.delete(this.#channel);
  }
  realtime(operation) {
    const operations = new Map([['clock',0xF8],['start',0xFA],['continue',0xFB],['stop',0xFC]]);
    if (!operations.has(operation)) throw new Error(`unsupported transport ${String(operation)}; Song Position is not sequencer seek`);
    const status = operations.get(operation);
    this.#ready();
    if (operation === 'start' || operation === 'continue') this.#running = true;
    this.#transmit([status]);
    if (operation === 'stop') this.#running = false;
  }
  unsupported(operation) { throw new Error(`unsupported operation: ${String(operation)}; no wire message sent`); }
  poll() {
    if (!this.#armed) return;
    const now = this.#clock();
    if (now < this.#armedAt || now-this.#armedAt >= 10000) this.disarm('10 second arm lease / clock rollback');
  }
  bindLifecycle(target, doc = target.document) {
    let active = true;
    const guard = reason => { if (active) this.disarm(reason); };
    const blur = () => guard('focus lost');
    const hide = () => guard('page hidden or exiting');
    const visibility = () => { if (doc?.hidden) hide(); };
    const key = e => { if (e.key === 'Escape') guard('Escape'); };
    target.addEventListener('blur',blur); target.addEventListener('pagehide',hide); target.addEventListener('keydown',key);
    doc?.addEventListener('visibilitychange',visibility);
    return () => { active = false; target.removeEventListener('blur',blur); target.removeEventListener('pagehide',hide); target.removeEventListener('keydown',key); doc?.removeEventListener('visibilitychange',visibility); };
  }
  #observe(data) {
    // Input is observation only. Never dispatch to outgoing semantic methods.
    if (!this.#armed || !Array.isArray(data) || data.length < 1 || data.some(x=>!Number.isInteger(x)||x<0||x>255)) return;
    const [st,d1,d2] = data;
    const ty=st & 0xF0, ch=(st & 15)+1;
    let label;
    if ([0xF8,0xFA,0xFB,0xFC].includes(st) && data.length===1) label='transport observed';
    else if ([0x80,0x90,0xB0,0xE0].includes(ty) && ch===this.#channel && data.length===3 && d1<128 && d2<128) {
      if (ty===0xB0 && ![1,64].includes(d1)) return;
      label=ty===0x80 || (ty===0x90 && d2===0) ? 'note release observed' : ty===0x90 ? 'note observed' : 'control observed';
    } else return;
    bounded(this.#feedback,{label,bytes:[...data]}); this.#record('rx-observed',{bytes:[...data],label});
  }
}
