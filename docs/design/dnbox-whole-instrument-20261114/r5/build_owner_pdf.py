"""Owner-readable standard PDF; genuine pinned before captures and proposal afters."""
from pathlib import Path
import io,json,hashlib
from xml.sax.saxutils import escape
from reportlab.pdfgen import canvas
from reportlab.lib.pagesizes import A4
from reportlab.lib.colors import HexColor,white
from reportlab.lib.utils import ImageReader
from reportlab.lib.styles import ParagraphStyle
from reportlab.platypus import Paragraph
R=Path(__file__).parent;W,H=A4
INK=HexColor('#173124');MUTED=HexColor('#516357');GREEN=HexColor('#427132')
def clean(t):return str(t).replace('→',' to ').replace('–','-').replace('−','-').replace('×',' x ').replace('…','...').replace('≥','>=').replace('≤','<=')
def para(c,t,x,y,w,size=9):
 p=Paragraph(t,ParagraphStyle('p',fontName='Helvetica',fontSize=size,leading=size*1.4,textColor=INK));_,h=p.wrap(w,H);p.drawOn(c,x,y-h);return y-h-8
def top(c,title,page):
 c.setFillColor(white);c.rect(0,0,W,H,stroke=0,fill=1);c.setFillColor(GREEN);c.setFont('Helvetica-Bold',9);c.drawString(40,H-32,'DnBOX / R5 / WHOLE-INSTRUMENT PROPOSAL')
 y=para(c,'<b>'+escape(clean(title))+'</b>',40,H-50,W-80,18)
 c.setFillColor(MUTED);c.setFont('Helvetica',8);c.drawString(40,20,'Proposal. Independent affected review pending. No firmware implementation or device proof.');c.drawRightString(W-40,20,str(page));return y-4
def native(c,p,x,y):c.drawImage(ImageReader(io.BytesIO(p.read_bytes())),x,y-180,width=180,height=180,mask='auto')
def note(c,label,t,y):return para(c,'<b>'+label+'</b> '+escape(clean(t)),40,y,W-80)
def build():
 out=R/'DnBOX-R5-whole-instrument-comparison.pdf';c=canvas.Canvas(str(out),pagesize=A4,pageCompression=1,invariant=1);c.setTitle('DnBOX R5: current workflows and whole-instrument proposal');c.setAuthor('DnBOX design study');pages=[]
 for n,p in enumerate(json.loads((R/'comparisons.json').read_text()),1):
  y=top(c,p['title'],n);c.setFont('Helvetica-Bold',8);c.setFillColor(MUTED);c.drawString(46,y,'ACTUAL SOURCE3c41 EMULATOR');c.drawString(320,y,'R5 PROPOSAL / NOT FIRMWARE');y-=12
  native(c,R/'reference'/p['before'],46,y);native(c,R/'native'/p.get('after',p['id']+'.png'),320,y);y-=193
  y=para(c,'Native240 x240 images embedded without resampling. Actual3c41 captures are separate from accepted d158, whose UI/glyph files are identical. Controlled proposal power state is3bars, not a measured percentage. State/value/phase limits are named below.',40,y,W-80,8)
  for label,key in [('Retain','retained'),('Change / reconsider','changed'),('Useful outcome','why'),('Physical route','sequence'),('Access cost','count'),('Tradeoff / still unproved','cost')]:y=note(c,label,p[key],y)
  assert y>40,(p['id'],y);pages.append({'page':n,'comparison':p['id'],'bottom':y});c.showPage()
 groups=json.loads((R/'evidence/whole-journey-audit.json').read_text())['groups'];y=top(c,'The complete musical journey, with current strengths retained',9)
 y=para(c,'Twenty groups cover J01-J39. This is a whole-instrument overhaul with shared visual/control grammar; the eight paired pages are curated examples, not whole-design acceptance. Exact routes, conditional action counts and source anchors remain in the separate complete map.',40,y,W-80)
 for col in range(2):
  yy=y;x=40+col*(W-70)/2;ww=(W-100)/2
  for g in groups[col*10:col*10+10]:
   yy=para(c,'<b>'+escape(clean(g['id']+' '+g['goal']))+'</b>',x,yy,ww,9)
   yy=para(c,escape(clean(g['route'][0])),x,yy,ww,8)
  assert yy>42,yy
 c.showPage();y=top(c,'One physical interface; honest status and remaining work',10)
 for label,t in [
 ('Keep what works','37 hardware keys: legacy control roles use F3-G5/27 keys; the admitted synth PART/SEQ Free Play path uses all37 with REC off, and clip keyboard also uses all37. Sixteen pads retain GRID/piano context and original-release ownership. No keybed lamps are invented. K1 remains master; K2-K4 keep their source roles; K5-K8 retain aligned page controls.'),
 ('Lights','Normal part hue, used-pattern dimness, actual selected/held/playhead priority, source button on/off, REC arm and host-derived clip colours remain separate meanings. Empty Song usedP1 is dim RGB31,18,5. LCD STOP/WAIT pending identity stays steady; physical WAIT/cue light policies are independent.'),
 ('Compact top power','Every locally owned page and overlay uses the same top-right rectangle. Honest values are EMPTY/1bar/2bars/FULL, USB host power and unknown. There is no charger line or calibrated percentage. Percentage inside the glyph is the requested future data contract, not a conversion from thirds or ADC thresholds.'),
 ('Existing versus proposed','Dynamic engine/slot labels, fast parameter routes and Chicago full-row ADSR already exist. R5 reuses native material, parameter glyph vocabulary and musical pattern thumbnail. Recognizable drum identities and Juno heading applications of the shared family are proposals. Repairs to a broken R4 model are not new firmware improvements.'),
 ('Real evidence','31 focused producer browser checks, complete changed matrix layout inventory, source-projected128-position Chicago WAVE drawings, and bounded actual3c41 Juno/BREAK captures are separately pinned. One misrouted producer capture attempt is retained and excluded. None is independent R5 or device acceptance.'),
 ('Unfinished musical outcomes','Recording, whole phrase/correction/repeated takes, valid-loop BREAK performance/release, generator recovery, presets, durability, SMC/Loopy feedback and physical recognition remain in the full map. A safe refusal is not musical success. No native resource, DSP fidelity, iOS-device or tactile/glare qualification is claimed.'),
 ('Reserved interfaces','The seven CP1 cases stay with the incumbent. CP1 decision precedes selected CP2 strategy then CP3 lifecycle. Settings R2 remains unchanged and component-owned; it needs the common-header interface, not an unauthorized rewrite. Same-hardware comparator adoption and DF1 remain open. Drone data/ID availability stays with the firmware owner.'),
 ('Next consumer','The existing parent routes this frozen successor to the same independent reviewer for affected checks; no replacement reviewer or runtime writer. PNG/PDF are the owner-facing route. HTML is retained for validation, not the sole iOS delivery.')]:y=note(c,label,t,y)
 assert y>40,y;c.save()
 receipt={'path':out.name,'bytes':out.stat().st_size,'sha256':hashlib.sha256(out.read_bytes()).hexdigest(),'pages':10,'native_images':'8 actual plus8 proposal, 240x240 each','pair_page_bottoms':pages,'iOS_device_open':'NOT_RUN; standard PDF parsing/render follows'}
 (R/'PDF-RECEIPT.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':build()
