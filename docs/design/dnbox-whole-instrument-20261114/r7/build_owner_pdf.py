"""Owner presentation: native comparisons and musical limits, no coordination details."""
from pathlib import Path
import hashlib,io,json,re
from xml.sax.saxutils import escape
from reportlab.pdfgen import canvas
from reportlab.lib.pagesizes import A4
from reportlab.lib.colors import HexColor,white
from reportlab.lib.utils import ImageReader
from reportlab.lib.styles import ParagraphStyle
from reportlab.platypus import Paragraph
R=Path(__file__).parent;W,H=A4;INK=HexColor('#183124');MUTED=HexColor('#506255')
def clean(s):return str(s).replace('→',' to ').replace('–','-').replace('−','-').replace('×',' x ').replace('…','...').replace('≥','>=').replace('≤','<=')
def para(c,t,x,y,w,size=9):
 p=Paragraph(t,ParagraphStyle('p',fontName='Helvetica',fontSize=size,leading=size*1.33,textColor=INK));_,h=p.wrap(w,H);p.drawOn(c,x,y-h);return y-h-8
def top(c,title,n):
 c.setFillColor(white);c.rect(0,0,W,H,fill=1,stroke=0);c.setFillColor(HexColor('#3c6a36'));c.setFont('Helvetica-Bold',9);c.drawString(40,H-30,'DnBOX R7 / WHOLE-INSTRUMENT WORKFLOW STUDY / PROPOSAL')
 y=para(c,'<b>'+escape(clean(title))+'</b>',40,H-47,W-80,17);c.setFillColor(MUTED);c.setFont('Helvetica',8);c.drawString(40,20,'Design proposal. Full overhaul incomplete; independent assessment remains required.');c.drawRightString(W-40,20,str(n));return y-6
def image(c,p,x,y):c.drawImage(ImageReader(io.BytesIO(p.read_bytes())),x,y-180,width=180,height=180,mask='auto')
def build():
 path=R/'DnBOX-R7-owner-comparisons.pdf';c=canvas.Canvas(str(path),pagesize=A4,pageCompression=1,invariant=1);c.setTitle('DnBOX R7: whole-instrument workflow proposals');c.setAuthor('DnBOX design study');pairs=json.loads((R/'comparisons.json').read_text());pages=[]
 for n,p in enumerate(pairs,1):
  y=top(c,p['title'],n);c.setFillColor(MUTED);c.setFont('Helvetica-Bold',8);c.drawString(46,y,clean(p.get('before_caption','ACTUAL FIRMWARE / 0.0.26 / 3c41')));c.drawString(320,y,clean(p.get('after_caption','PROPOSED UI / NOT FIRMWARE')));y-=12;image(c,R/'reference'/p['before'],46,y);image(c,R/'native'/p['after'],320,y);y-=192
  y=para(c,'Original 240 x 240 pixels embedded without resampling. Actual-before runs the pinned Elite emulator package. Accepted d158 has matching relevant UI source and was not executed here. Power bars in proposals are an inspection fixture.',40,y,W-80,8)
  for label,key in [('Keep','retained'),('Change / reconsider','changed'),('Purpose','why'),('Controls','sequence'),('Action cost','count'),('Limits / tradeoff','cost')]:y=para(c,'<b>'+label+'</b> '+escape(clean(p[key])),40,y,W-80,8.7)
  assert y>40,(p['id'],y);pages.append({'page':n,'comparison':p['id'],'bottom_y':y});c.showPage()
 n=len(pairs)+1;y=top(c,'One instrument: what is concrete, what remains',n)
 for label,t in [
 ('Playing and editing','Keep useful lower controls, fast parameter routes, native Chicago ADSR, Miami vocabulary and large bright Juno values. Held-step editing uses the current large white note/gate/accent/slide values. Drum pictures, source pad colours and physical button feedback work together; the piano keys have no lights.'),
 ('Sequencing and performance','The proposal connects absolute64-step editing, parameter scope, motion write/override/clear, independent cues/mutes, occupied five-part copy and scoped panic. Proposed feedback adds context without extra navigation. Recognition, glare and pressure-use tradeoffs still need assessment.'),
 ('Generator and correction','Current NEW/MUTATE, root/scale/seed and all eight generator controls are retained. Public generator mathematics is checked separately from audio. Select-only GEN cell focus reuses the large held editor and removes the SEQ-and-back detour. Short taps do not write/delete. Native history grouping/capacity and automatic mutation scheduling remain separate limits.'),
 ('Recording and recovery','Current stopped entry retains accent/slide modifiers and their consumed releases, correction and repeated entry. The37-key inspection surface now routes GRID, track and admitted live roles. Full running/poly recording, durable save/reload and complete recovery still need work; a lamp or signal activity is not a saved take.'),
 ('Sound and presets','Existing engine identity and sound-control vocabulary stay. Active Drone work is deferred without silently converting saved data. Preset/held-note decisions must be settled before recording-strategy and save/recovery choices are frozen. Juno interface work is not proof of AudioKitJ6 sound fidelity.'),
 ('Connected use and BREAK','The no-host Loopy comparison keeps its original body/lower pixels. Real host/peer operation, full FX/controller work, clock handover and a populated BREAK slice/live-release journey remain unfinished. An unavailable loop is not a successful performance.'),
 ('Settings','The separate Settings presentation is retained unchanged in the walkthrough, including six paired tasks. Its actual-before evidence is historical25.x, not current3c41. Common-header and held-note continuity integration remain open.')]:y=para(c,'<b>'+label+'</b> '+escape(clean(t)),40,y,W-80,9)
 assert y>40,y;c.showPage();n+=1;y=top(c,'How to read the evidence',n)
 for label,t in [
 ('Actual versus proposed','Actual captures execute source3c41 with its pinned0.0.26 Elite package and runtime. Accepted d158 shares the relevant UI files but is not an executed package here. No proposed R7 firmware exists. Historical captures, current emulator output, source math and browser model checks are separate.'),
 ('Equivalent states','Each page states its known start, control sequence and remaining differences. Miami/no-host Loopy reuse native body/lower pixels. Motion phase and mixed-source copy are explicit model fixtures. The GEN held-editor pair compares the same corrected cell across the current SEQ route and the proposed GEN route; it deliberately changes view.'),
 ('Source strengths','Current dynamic engine labels, native ADSR/Juno vocabulary, direct lower controls, large held values and source button/pad meanings are retained strengths. Repairing a prototype omission is not a new firmware feature. New feedback must prove useful in real tasks.'),
 ('Review and checks','The previous revision was useful WIP, not a completed overhaul. This successor requires its own independent affected assessment. Browser, source-math, native-image and document checks support only their named scope; counts do not establish musical success.'),
 ('Power','The platform exposes coarse0-3bars, USB-host power or unknown. It has no calibrated percentage or genuine charging line. Top status remains compact and consistent without inventing precision.'),
 ('Delivery and remaining work','This standard PDF and the separate native PNGs are the owner presentation. The self-contained walkthrough is for interaction inspection. Actual iOS-device opening and physical display/control tests remain unrun. Whole audible ideas, repeated takes, recovery, resources, competitor lessons and remaining instrument journeys still require evidence. Design delivery is not runtime implementation approval.')]:y=para(c,'<b>'+label+'</b> '+escape(clean(t)),40,y,W-80,9)
 assert y>40,y;c.showPage();c.save();raw=path.read_bytes();out={'file':path.name,'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest(),'pages':len(pairs)+2,'pair_pages':pages,'classification':'Owner-facing proposal PDF; no private coordination or internal routing details; standard parse/render checks separate'};(R/'PDF-BUILD-RECEIPT.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out|{'pair_pages':'separate receipt'}))
if __name__=='__main__':build()
