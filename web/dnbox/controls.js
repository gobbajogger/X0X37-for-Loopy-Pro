// An accessible alternative to fm1-emu's original 37-key/16-pad panel hit-testing.
// These controls only send the SAME key/analog/button messages to fm1-web; no JS voices.
const NAMES = ['C','C♯','D','D♯','E','F','F♯','G','G♯','A','A♯','B'];
const BLACK = new Set([1,3,6,8,10]);
export const KEY_COUNT = 37;
export const PAD_COUNT = 16;
export const MIDI_BASE = 48; // C3, confirmed by fm1-emu web/main.js MIDI mapping

export function noteAt(id) {
  if (!Number.isInteger(id) || id < 0 || id >= KEY_COUNT) throw new RangeError('SMK-37 key out of range');
  const midi = MIDI_BASE + id;
  return { id, midi, label: NAMES[midi % 12] + (Math.floor(midi / 12) - 1), black: BLACK.has(midi % 12) };
}

// Multiple sources can hold the same note. Paired releases and cancellation matter for live use.
export function heldRouter(send) {
  const held = new Map();
  const valid = (kind, id) => (kind === 'key' && Number.isInteger(id) && id >= 0 && id < KEY_COUNT) ||
    (kind === 'pad' && Number.isInteger(id) && id >= 0 && id < PAD_COUNT) ||
    (kind === 'button' && Number.isInteger(id) && id >= 37 && id <= 54);
  const release = token => {
    const item = held.get(token);
    if (!item) return false;
    held.delete(token);
    send({ ...item, down: false });
    return true;
  };
  return {
    press(token, kind, id) {
      if (!valid(kind,id)) throw new RangeError('invalid SMK-37 control');
      if (held.has(token)) return false;
      held.set(token, { kind, id });
      send({ kind, id, down: true });
      return true;
    },
    release,
    releaseAll() { for (const token of [...held.keys()]) release(token); },
    get count() { return held.size; }
  };
}

export function mountControls(doc = document) {
  const root = doc.getElementById('dnbox-controls');
  const keys = doc.getElementById('dnbox-keys');
  const pads = doc.getElementById('dnbox-pads');
  const mixer = doc.getElementById('dnbox-mixer');
  const knobs = doc.getElementById('dnbox-knobs');
  if (!root || !keys || !pads || !mixer || !knobs) throw new Error('DnBOX control surface missing');
  const send = detail => doc.dispatchEvent(new CustomEvent('dnbox:input', { detail }));
  const holds = heldRouter(send);
  let booted = false;
  const buttons = [];
  const enable = yes => { booted = yes; for (const b of buttons) b.disabled = !yes; if (!yes) holds.releaseAll(); };
  function momentary(button, kind, id) {
    const token = kind + ':' + id;
    const press = e => {
      if (!booted) return;
      if (e) e.preventDefault();
      if (holds.press(token,kind,id)) {
        button.setAttribute('aria-pressed','true');
        if (e?.pointerId !== undefined) try { button.setPointerCapture(e.pointerId); } catch {}
      }
    };
    const release = () => {
      if (holds.release(token)) button.setAttribute('aria-pressed','false');
    };
    button.type = 'button';
    button.disabled = true;
    button.setAttribute('aria-pressed','false');
    button.addEventListener('pointerdown', press);
    for (const event of ['pointerup','pointercancel','lostpointercapture','blur']) button.addEventListener(event, release);
    button.addEventListener('keydown', e => { if (e.key === ' ' || e.key === 'Enter') press(e); });
    button.addEventListener('keyup', e => { if (e.key === ' ' || e.key === 'Enter') { e.preventDefault(); release(); } });
    buttons.push(button);
  }
  let white = 0;
  for (let id = 0; id < KEY_COUNT; id++) {
    const note = noteAt(id);
    const button = doc.createElement('button');
    button.className = 'note ' + (note.black ? 'sharp' : 'natural');
    button.setAttribute('aria-label', note.label + ', MIDI ' + note.midi);
    button.textContent = note.label;
    if (note.black) {
      button.style.left = ((white - 0.34) * 100 / 22) + '%';
    } else {
      button.style.left = (white * 100 / 22) + '%';
      white++;
    }
    momentary(button,'key',id);
    keys.append(button);
  }
  for (let id=0; id<PAD_COUNT; id++) {
    const button=doc.createElement('button');
    button.className='pad';
    button.textContent='PAD ' + String(id+1).padStart(2,'0');
    button.setAttribute('aria-label', 'Pad ' + (id+1));
    momentary(button,'pad',id);
    pads.append(button);
  }
  for (let i=0;i<4;i++) {
    const el=doc.createElement('label');
    el.className='slider-control';
    el.textContent='FADER ' + (i+1);
    const input=doc.createElement('input');
    input.type='range'; input.min='0'; input.max='1023'; input.step='1'; input.value='0'; input.disabled=true;
    input.setAttribute('aria-label','Fader '+(i+1));
    input.addEventListener('input',()=>{if(booted) send({kind:'analog',id:16+i,value:Number(input.value)});});
    el.append(input); mixer.append(el); buttons.push(input);
  }
  for (const [id,label,value] of [[20,'PITCH',535],[21,'MOD',629]]) {
    const el=doc.createElement('label');
    el.className='slider-control';
    el.textContent=label;
    const input=doc.createElement('input');
    input.type='range';input.min='0';input.max='1023';input.step='1';input.value=String(value);input.disabled=true;
    input.setAttribute('aria-label',label+' wheel');
    input.addEventListener('input',()=>{if(booted) send({kind:'analog',id,value:Number(input.value)});});
    if (id===20) {
      for(const event of ['pointerup','pointercancel','keyup','blur']) input.addEventListener(event,()=>{
        input.value='535';if(booted)send({kind:'analog',id,value:535});
      });
    }
    el.append(input);mixer.append(el);buttons.push(input);
  }
  for (let i=0;i<8;i++) {
    const el=doc.createElement('div');el.className='encoder-control';
    const name=doc.createElement('span'); name.textContent='K'+(i+1);
    el.append(name);
    for(const clicks of [-1,1]) {
      const b=doc.createElement('button');b.type='button';b.textContent=clicks<0?'−':'+';
      b.disabled=true;
      b.setAttribute('aria-label','Knob '+(i+1)+' '+(clicks>0?'clockwise':'counterclockwise'));
      b.addEventListener('click',()=>{if(booted)send({kind:'encoder',id:i,clicks});});
      buttons.push(b);el.append(b);
    }
    knobs.append(el);
  }
  const play = doc.getElementById('dnbox-play');
  if (play) { momentary(play,'button',53);play.textContent='SEQ PLAY (hold)'; }
  const chord = doc.getElementById('dnbox-chord');
  if (chord) {
    buttons.push(chord);chord.disabled=true;
    chord.addEventListener('click',()=>{
      if(!booted)return;
      const tokens=[12,15,19].map(id=>'example:'+id);
      for(let i=0;i<tokens.length;i++)holds.press(tokens[i],'key',[12,15,19][i]);
      setTimeout(()=>tokens.forEach(t=>holds.release(t)),420);
    });
  }
  const drop = () => { holds.releaseAll(); root.querySelectorAll('[aria-pressed]').forEach(b=>b.setAttribute('aria-pressed','false')); };
  window.addEventListener('blur',drop);
  doc.addEventListener('visibilitychange',()=>{if(doc.hidden)drop();});
  doc.addEventListener('dnbox:booting',()=>enable(false));
  doc.addEventListener('dnbox:booted',e=>enable(e.detail?.board===1));
  doc.addEventListener('dnbox:stopped',()=>enable(false));
  enable(false);
  return { releaseAll: drop, setBooted: enable };
}
