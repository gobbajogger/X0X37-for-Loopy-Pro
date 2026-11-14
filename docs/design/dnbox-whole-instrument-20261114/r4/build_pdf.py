"""Curated R4 PDF; genuine current captures and explicit proposal pixels."""
from pathlib import Path
import hashlib,json,io
from xml.sax.saxutils import escape
from reportlab.pdfgen import canvas
from reportlab.lib.utils import ImageReader
from reportlab.lib.pagesizes import A4
from reportlab.lib.colors import HexColor,white
from reportlab.lib.styles import ParagraphStyle
from reportlab.platypus import Paragraph
R=Path(__file__).parent;W,H=A4
INK=HexColor('#173124');MUTED=HexColor('#516357');ACCENT=HexColor('#427132')
def para(c,t,x,y,w,size=10):
 p=Paragraph(t,ParagraphStyle('p',fontName='Helvetica',fontSize=size,leading=size*1.4,textColor=INK));_,h=p.wrap(w,H);p.drawOn(c,x,y-h);return y-h-9
def top(c,title,n):
 c.setFillColor(white);c.rect(0,0,W,H,fill=1,stroke=0);c.setFillColor(ACCENT);c.setFont('Helvetica-Bold',10);c.drawString(40,H-35,'DnBOX / R4 / CORRECTIVE PROPOSAL')
 c.setFillColor(INK);c.setFont('Helvetica-Bold',20);c.drawString(40,H-67,title);c.setFillColor(MUTED);c.setFont('Helvetica',8);c.drawString(40,22,'Design model only. Same-reviewer reassessment pending. No firmware implementation.');c.drawRightString(W-40,22,str(n));return H-95
def native(c,p,x,y):c.drawImage(ImageReader(io.BytesIO(p.read_bytes())),x,y-180,width=180,height=180,mask='auto')
def note(c,label,t,y):return para(c,'<b>'+label+'</b> '+escape(t),40,y,W-80)
def build():
 out=R/'DnBOX-R4-workflow-comparison.pdf';c=canvas.Canvas(str(out),pagesize=A4,pageCompression=1,invariant=1);c.setTitle('DnBOX R4: controls and feedback');c.setAuthor('DnBOX design study')
 pairs=json.loads((R/'comparisons.json').read_text())
 pairs.append({'id':'mix909','title':'Track mixer and source-owned pads','before':'elite-focused-04-mix-909.png','retained':'Eleven kit tracks, selected track, static level travel, pan thumbs, four selected-track controls and kit pad ownership.','changed':'Replaces the wrong five-part display on drum MIX. Track selection stays on the kit page; pad editing affects that kit. Header, footer, hue and content follow the visible kit.','why':'The screen and pads must describe the same destination, even when a melodic part is selected globally.','sequence':'From initial SEQ: NOTE REPEAT twice reaches909 MIX. Black selects its track; pad writes its step. K3 follows the source part-selection rule.','count':'No added input on these routes. PARTS/MASTER/WAVE have no unheld drum-pad layer. MIX holds do not acquire p-lock scope.','cost':'R4 uses exact current-source fresh-project levels. Selected kick -3.8dB matches the actual capture; remaining captured project raw cells are not verified. Static levels are dim; no live meter is fabricated.'})
 for n,p in enumerate(pairs,1):
  y=top(c,p['title'],n);c.setFillColor(MUTED);c.setFont('Helvetica-Bold',9);c.drawString(48,y,'ACTUAL CURRENT / 3c41 / ELITE');c.drawString(320,y,'R4 / PROPOSED CANVAS');y-=14
  native(c,R/'reference'/p['before'],48,y);native(c,R/'native'/(p['id']+'.png'),320,y);y-=194
  y=para(c,'Genuine existing current-firmware LCD capture versus a controlled R4 input model. No candidate firmware was executed. Transport phase, native font rasterization and unspecified project cells are not synchronized.',40,y,W-80,9)
  for label,key in [('Keep','retained'),('Correct','changed'),('Purpose','why'),('Control sequence','sequence'),('Access / feedback','count'),('Limit / tradeoff','cost')]:y=note(c,label,p[key],y)
  c.showPage()
 y=top(c,'New cues; distinct sound identities',6);c.setFillColor(MUTED);c.setFont('Helvetica-Bold',9);c.drawString(48,y,'NEW PROPOSAL / JUNO');c.drawString(320,y,'NEW PROPOSAL / 808 VARIANTS');y-=14
 native(c,R/'native/juno.png',48,y);native(c,R/'native/808variant.png',320,y);y-=194
 for label,t in [('Keep','Juno retains four simultaneous groups, fixed K5-8 positions and its fifth SOUND page.808 retains its actual eleven-track order and all sequence controls.'),('New art','Juno saw/pulse blend, sub, pulse width, filter/positive ENV, stage envelopes, triangle PWM LFO and stereo chorus cues derive from the current custom-Juno code. Every cue and readout uses the same effective q0..127 value.'),('Correction','Juno WAVE is continuous. Untouched JUNO POLY values use its actual16-control factory vector rather than all50%. Low/mid/high congas differ by head width and pitch tick; cowbell and808 hat/cymbal destinations follow their actual order.'),('Tradeoff','Small-symbol recognition remains unmeasured. Full selected names remain useful. Juno art is new, not a pre-existing actual glyph. A current Juno LCD capture is still needed for a direct actual/proposed pair.'),('Scope','Source-defined custom Juno semantics are not AudioKitJ6 or imported-preset/audio fidelity. Active Drone design is deferred by the owner; stable IDs and saved Drone data must remain intact under the existing firmware-owner availability contract.')]:y=note(c,label,t,y)
 c.showPage();y=top(c,'What this revision proves and leaves open',7)
 for label,t in [('Confirmed corrections','The producer checks exercise mutation-to-label/curve feedback, continuous controls and bounds, source clamp/page memory, correct slot identities, two independent kit owners, MIX selection/write, hue and pad layers. These are proposal checks, not independent acceptance.'),('One physical interface','Normal kit pads retain content, playhead inversion and final held-input priority. Drum PART retains content; FX and non-kit MIX pages do not inherit it. SOURCE snapshots are separate from physical brightness. No keybed LEDs or new mute/REC-success pad colour is invented.'),('Whole-instrument limits','The full39-journey/261-row source inventory remains available. This bounded R4 editor does not implement every scalar, list, musical note, controller, recording, storage or recovery transaction. Source inspection and expected action counts do not prove all retained routes, speed or insignificant tradeoffs.'),('Reserved dependencies','Settings and seven CP1 cases remain with incumbents. CP2 selects one whole-idea/correction/repeated-take capture strategy before dependent CP3 lifecycle. Comparator adoption and DF1 remain open. Current held-note loss on Global entry and clip LOAD confirmation mismatch are not fixed by this proposal.'),('Review and delivery','Frozen96b4 remains the reviewed defective predecessor. R4 requires affected reassessment by the same independent reviewer. This PDF and native PNGs are the owner-facing route; the self-contained HTML supports interaction validation. Actual iOS-device opening remains NOT_RUN.'),('Runtime boundary','Current before source is3c41ef4bedf9701f549ce2752b73306e2fab1eb8. Existing exact Elite/Pro AudioWorklet reviews and their package/runtime hashes are retained. R4 itself has no firmware build, device result, native resource qualification, audio parity, release or merge.')]:y=note(c,label,t,y)
 y=para(c,'Exact HTML, PDF, frame, source and model receipt identities are in ARTIFACT-MANIFEST.json and model-inputs.json. The current actual captures and proposal PNGs have separate manifests and evidence classes.',40,y,W-80,9)
 c.save();print(json.dumps({'path':str(out),'bytes':out.stat().st_size,'sha256':hashlib.sha256(out.read_bytes()).hexdigest(),'pages':7,'iOS_device_open':'NOT_RUN'}))
if __name__=='__main__':build()
