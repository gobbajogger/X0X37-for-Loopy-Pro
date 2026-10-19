// SPDX-License-Identifier: GPL-3.0-only
import { readFileSync } from "node:fs";
import vm from "node:vm";

const html=readFileSync(new URL("./x0x_controller.html",import.meta.url),"utf8");
const code=html.slice(html.indexOf("/*CONTROLLER-BEGIN*/"),html.indexOf("/*CONTROLLER-END*/"));
const C=vm.runInNewContext(code+";({PARTS,noteMessage,clockPeriod,makeController})",{setInterval,clearInterval});
let failed=0;
const ok=(v,s)=>{ console.log(`${s.padEnd(58)} ${v?"ok":"FAIL"}`); if(!v) failed++; };
const eq=(a,b)=>a.length===b.length&&a.every((x,i)=>x===b[i]);

ok(C.PARTS.map(x=>x.channel).join() === "10,11,2,3,4", "default channels match the manual");
ok(eq(C.noteMessage(10,36,110,true),[0x99,36,110]) && eq(C.noteMessage(10,36,0,false),[0x89,36,0]), "note on and off use the selected one-based channel");
ok(Math.abs(C.clockPeriod(120)-20.8333333)<0.001, "MIDI clock is 24 pulses per quarter note");

const sent=[]; let tick=null, period=0, cancelled=0;
const ctl=C.makeController(x=>sent.push(x),(fn,ms)=>{tick=fn;period=ms;return 7;},id=>{if(id===7)cancelled++;});
ctl.noteOn(2,60,100); ctl.noteOn(2,60,100); ctl.noteOff(2,60);
ok(sent.length===2 && eq(sent[0],[0x91,60,100]) && eq(sent[1],[0x81,60,0]), "held note sends once and releases cleanly");
ctl.transport(0xFA,120,true); tick(); ctl.transport(0xFC,120,false);
ok(eq(sent[2],[0xFA]) && eq(sent[3],[0xF8]) && eq(sent[4],[0xFC]) && Math.abs(period-20.8333333)<0.001 && cancelled===1, "transport starts clock and Stop cancels it");
ctl.noteOn(3,64,127); ctl.panic();
ok(ctl.heldCount()===0 && sent.some(x=>eq(x,[0x82,64,0])) && sent.filter(x=>x[1]===123).length===16, "panic releases held notes and sends all-notes-off");

if(failed) process.exit(1);
