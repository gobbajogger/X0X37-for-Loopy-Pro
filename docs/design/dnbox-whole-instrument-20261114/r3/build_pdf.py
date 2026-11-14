"""Owner-facing PDF with native PNGs; historical references are clearly labelled."""
from pathlib import Path
import re
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
PS=ParagraphStyle('body',fontName='Helvetica',fontSize=10,leading=14,textColor=INK,spaceAfter=6)
def para(c,t,x,y,w,size=10):
 st=ParagraphStyle('p',parent=PS,fontSize=size,leading=size*1.4)
 t=re.sub(r',(?=[A-Za-z0-9])',', ',t)
 t=re.sub(r':(?=[A-Za-z0-9])',': ',t)
 p=Paragraph(t,st);_,h=p.wrap(w,H);p.drawOn(c,x,y-h);return y-h-8
def heading(c,title,subtitle,page):
 c.setFillColor(white);c.rect(0,0,W,H,fill=1,stroke=0);c.setFillColor(ACCENT);c.setFont('Helvetica-Bold',10);c.drawString(40,H-35,'DnBOX / R3 / POST-REJECTION DESIGN STUDY')
 c.setFillColor(INK);c.setFont('Helvetica-Bold',21);c.drawString(40,H-67,title)
 y=para(c,subtitle,40,H-83,W-80,10)
 c.setFillColor(MUTED);c.setFont('Helvetica',8);c.drawString(40,22,'Proposal only. No production implementation or whole-design acceptance.');c.drawRightString(W-40,22,str(page));return y
def native(c,path,x,y,w=180):c.drawImage(ImageReader(io.BytesIO(path.read_bytes())),x,y-w,width=w,height=w,mask='auto');return y-w
def bullet(c,label,t,y):return para(c,'<b>'+label+'</b> '+t,40,y,W-80)

def build():
 out=R/'DnBOX-R3-workflow-comparison.pdf';c=canvas.Canvas(str(out),pagesize=A4,pageCompression=1,invariant=1)
 c.setTitle('DnBOX R3: retain the workflow');c.setAuthor('DnBOX design study');c.setSubject('New R3 mockups; source-derived retention map; historical reference comparisons; acceptance open')
 pairs=[
 ('Find the drum; keep the grid','drum','909-four-kicks-playing-lcd.png',
  'Same known visible task:909, four kick hits,16 steps,1/16, 125 bpm,REC off,running. Playhead phase is not synchronized.',
  [('Keep','All 11 tracks plus accent,16 columns,LENGTH/RATE,black-key selection,white/pad step entry and PATCH+black mute.'),('Change','New percussion pictures replace row abbreviations. The selected picture/full name uses only the former unmapped K7/K8 display area; both knobs remain unmapped.'),('Access','Running snare selection:G#3, then pad 4 toggles step 5. Length/rate still need one direct K5/K6 turn. No extra page or confirmation.'),('Tradeoff','Tiny-symbol and 808 variant recognition is unqualified. The large picture must not imply a new editable control. Retain baseline if actual-use benefit does not outweigh that cost.')]),
 ('Keep the sound overview','chicago','chicago-page1-lcd.png',
  'Known visible filter task:Chicago A, FILTER 1,CUTOFF25%,PEAK75%,ENV+56%,stopped. Different versions/rasterizers; no current LCD equivalence claimed.',
  [('Keep','Four neighboring parameter groups,one four-column physical mapping,existing glyph/readout/ADSR families,reverse wrap and deep list.'),('Withdraw','The rejected single-group focus replacement. Chicago 8,Miami 6,Juno 5,Drone 3 page cycles include SOUND.'),('Access','Known OSC memory from SEQ:PARA → K2+ → K5 =3 logical gestures. Deep list:SCALE → K2+ → K2+ → K3 → PATCH =5. R3 retains both routes.'),('Tradeoff','No improvement over current parameter navigation is claimed. This is deliberate retention. Other values are illustrative fixtures; native DSP wave-cache and typography parity are not established.')]),
 ('Keep live context during WAIT','home','home-five-part-stopped-lcd.png',
  'Known stopped Home reference:P01,909four kicks,125 bpm,SWING50%,FILTER OFF,PUMP4.0dB. Current exact before remains pending intake.',
  [('Keep','Sixteen pattern thumbnails,five part-content lanes,and all four graphical PERFORM controls. Ordinary cue/WAIT/fault state does not replace these with a diagnostic list.'),('Change','Compact clock WAIT treatment plus a read-only coarse power glyph in the strip-caption gap. No charging state,percentage or extra knob binding.'),('Access','Home pump remains zero navigation plus one K8 turn,including WAIT. Filter stays K7; source DJ macro deadband is retained in the model.'),('Tradeoff','Clock/power collisions,readability and applied-state fidelity still need current-candidate comparison. Normal pads cannot substitute for all-part mute identity.')]),
 ('Keep arrangement neighbors','song','song-bar-stopped-lcd.png',
  'Known empty Song reference:first authoring entry,stopped,125 bpm. This is existing manual authoring,not a selected recording strategy.',
  [('Keep','Up to 11 visible entries with per-part patterns,mutes,repeats,selected/playing bar,and both four-knob BAR pages.'),('Withdraw','The rejected isolated-bar list. Empty remains empty with its existing add-bars hint; no fictional data fills the page.'),('Access','From Home:PATCH enters Song. K2+ reaches BAR2; K8 is repeats. PATCH returns Home. Existing key-entry and mute routes remain.'),('Tradeoff','CP2 whole-idea/repeated-take selection and dependent CP3 lifecycle remain open. A readable bar selector is not recording,replay or recovery success.')])]
 for i,(title,after,before,boundary,notes) in enumerate(pairs,1):
  y=heading(c,title,'Retain useful controls and fast routes. A change must earn its tradeoff in actual use.',i)
  c.setFillColor(MUTED);c.setFont('Helvetica-Bold',9);c.drawString(48,y-10,'ACTUAL HISTORICAL / 79280e76');c.drawString(320,y-10,'NEW R3 / DESIGN MODEL')
  y-=24;native(c,R/'reference'/before,48,y);native(c,R/'native'/(after+'.png'),320,y);y-=190
  y=para(c,'<b>Current 3c41 actual capture:</b> not yet received in this workspace. The parent reports fresh current Elite/Pro exercise; its report and launch bytes are pending intake. Historical package 0.0.25 was requested/displayed as 25.1.',40,y,W-80,9)
  y=para(c,escape(boundary),40,y,W-80,9)
  for label,t in notes:y=bullet(c,label,escape(t),y)
  c.showPage()
 y=heading(c,'One interface: screen, pads, buttons','Source-derived feedback map at 3c41ef4b. Browser diagrams are not measured hardware output.',5)
 y=bullet(c,'Physical facts','Sixteen RGB pads;37 keybed keys have no LEDs. Normal button outputs are binary. REC has its own red LED; SMK PLAY-red is unused.',y)
 # Two coordinated source snapshots, never a hardware photo or animation claim.
 for row,title,on in [(0,'Stopped four-kick pattern',{0,4,8,12}),(1,'Step5 playhead: stored hit inverts off',{0,8,12})]:
  c.setFillColor(INK);c.setFont('Helvetica-Bold',11);c.drawString(40,y-12,title);top=y-28
  for p in range(16):
   x=40+(p%8)*57;py=top-(p//8)*34;c.setFillColor(HexColor('#7f4a14') if p in on else HexColor('#0a100d'));c.roundRect(x,py-27,48,26,3,fill=1,stroke=0);c.setFillColor(white);c.setFont('Helvetica',9);c.drawCentredString(x+24,py-17,str(p+1))
  y=top-78
 for label,t in [('Held input','Holding pad 4 overrides that inversion and makes it bright. No unique held-step or accepted-lock colour is invented.'),('Mute','Muted selected-track hits still display on pads. Track/part mute identity stays on the LCD; there is no physical black-key mute lamp.'),('REC and WAIT','REC arming is not capture success. WAIT PLAY uses frame 32,Home cue pads frame 8,LCD cue frame 16. No common phase or measured blink rate is assumed.'),('Separate owners','Clip/Loopy host palette,eligible division pads and development calibration retain their gated owners. Local light changes do not prove host launch or durable recording.')]:y=bullet(c,label,t,y)
 c.showPage()
 y=heading(c,'What remains before acceptance','The revised candidate is concrete; the proof boundary remains explicit.',6)
 for label,t in [('Whole scope','All 39 R22 journeys and261 routing rows are retained.35 source-derived access tasks and complete engine/drum control tables are in the machine-readable function map.'),('No hidden removals','Mapped bottom controls,parameter columns,neighbor overview,deep-list entries,reverse wrap,held release ownership and meaningful light states must be accounted for. Planned retained-route delta is zero; actual task speed/usefulness is unqualified.'),('Current finding intake','Parent reports clip LOAD advertises masked SCALE YES and held Chicago sound stops on Global entry. Neither is called fixed. Exact stage1534 report, launch and capture bytes still need a supported result route.'),('Fresh review','The parent reconciles the existing independent emulator reviewer. No duplicate is created. That reviewer must exercise this exact R3 proposal and actual package with no prior scores or desired rating.'),('Reserved work','Settings component and seven CP1 cases stay reserved. CP2 must select one whole-idea/repeated-take strategy before CP3. Comparator adoption,DF1,combined resources,audible correction/repeat/recovery and physical qualification remain open.'),('Owner delivery','This PDF and the separate240 square PNGs are the owner-facing artifacts. The HTML workbench remains available for interaction checks; it is not the sole deliverable.')]:y=bullet(c,label,t,y)
 y=para(c,'<b>Identities</b><br/>Current source: 3c41ef4bedf9701f549ce2752b73306e2fab1eb8<br/>Tree: fa0c3fe75483534f12839eeb8f5e29ec9cb52c6d<br/>Historical package SHA256:5e24e0d4afccc30906b334fbe73bcad3621df8df3c325790b7af2f94accffb1a<br/>Rejected proposal: a8986ac2d364bb59a38e068381e18ca068fe7c8d<br/>Exact R3 HTML,PNG,source and model-check pins:ARTIFACT-MANIFEST.json.',40,y,W-80,8)
 c.save();print(json.dumps({'pdf':str(out),'bytes':out.stat().st_size,'sha256':hashlib.sha256(out.read_bytes()).hexdigest(),'pages':6,'iOS_device_open':'NOT_RUN; standardPDF with embedded native images'}))

if __name__=='__main__':build()
