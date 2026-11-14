import {MidiContract,FakeMidiEndpoint} from './transport.mjs';
const controller = new MidiContract();
let endpoint = null, detachLifecycle = null;
const $ = id => document.getElementById(id);
for(let channel=1;channel<=15;channel++)$('channel').add(new Option(String(channel),String(channel)));
$('channel').value='2';
function render() {
  $('status').textContent=JSON.stringify(controller.status,null,2);
  $('sent').textContent=JSON.stringify(endpoint?.sent ?? [],null,2);
  $('log').textContent=JSON.stringify(controller.log,null,2);
}
function invoke(fn) {
  try { fn(); $('result').textContent='Operation completed in fake endpoint only; no device acknowledgement.'; }
  catch(error) { $('result').textContent=`Rejected or failed: ${error.message}`; }
  render();
}
$('connect').onclick=()=>invoke(()=>{
  const next=new FakeMidiEndpoint(); controller.connect(next); endpoint=next;
  detachLifecycle?.(); detachLifecycle=controller.bindLifecycle(window,document);
});
$('disconnect').onclick=()=>invoke(()=>controller.disconnect());
$('lose').onclick=()=>invoke(()=>endpoint?.lose());
$('arm').onclick=()=>invoke(()=>controller.arm());
$('disarm').onclick=()=>invoke(()=>controller.disarm());
$('channel').onchange=()=>invoke(()=>controller.setChannel(Number($('channel').value)));
$('note').onpointerdown=e=>{
  if(e.button!==0)return;
  e.preventDefault();$('note').setPointerCapture(e.pointerId); invoke(()=>controller.noteOn(60));
};
for(const event of ['pointerup','lostpointercapture'])$('note').addEventListener(event,()=>invoke(()=>controller.noteOff(60)));
$('note').addEventListener('pointercancel',()=>invoke(()=>controller.disarm('pointer cancelled')));
$('note').onkeydown=e=>{if([' ','Enter'].includes(e.key)){e.preventDefault();if(!e.repeat)invoke(()=>controller.noteOn(60));}};
$('note').onkeyup=e=>{if([' ','Enter'].includes(e.key)){e.preventDefault();invoke(()=>controller.noteOff(60));}};
$('mod').onclick=()=>invoke(()=>controller.controlChange(1,75));
$('pedal').onclick=()=>invoke(()=>controller.controlChange(64,127));
$('bend').onclick=()=>invoke(()=>controller.pitchBend(16383));
for(const button of document.querySelectorAll('[data-rt]'))button.onclick=()=>invoke(()=>controller.realtime(button.dataset.rt));
$('incoming').onclick=()=>invoke(()=>endpoint?.receive([0x90+controller.status.channel-1,60,100]));
$('unsupported').onclick=()=>invoke(()=>controller.controlChange(74,99));
$('seek').onclick=()=>invoke(()=>controller.realtime('position'));
$('sysex').onclick=()=>invoke(()=>controller.unsupported('historical WATCH / DeviceSync'));
$('failure').onclick=()=>invoke(()=>endpoint?.failNext('after'));
setInterval(()=>{controller.poll();render();},100);
// Explicit inspection hook for the local acceptance suite. Never a real MIDI adapter.
window.fixture=Object.freeze({controller,get endpoint(){return endpoint;},render});
render();
