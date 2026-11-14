"""Standard owner-readable PDF: native actual/proposal pairs, no contact sheets."""
from pathlib import Path
import hashlib,io,json
from xml.sax.saxutils import escape
from reportlab.pdfgen import canvas
from reportlab.lib.pagesizes import A4
from reportlab.lib.colors import HexColor,white
from reportlab.lib.utils import ImageReader
from reportlab.lib.styles import ParagraphStyle
from reportlab.platypus import Paragraph
R=Path(__file__).parent;W,H=A4
INK=HexColor('#183124');MUTED=HexColor('#506255');GREEN=HexColor('#3c6a36')
def clean(s):return str(s).replace('→',' to ').replace('–','-').replace('−','-').replace('×',' x ').replace('…','...').replace('≥','>=').replace('≤','<=')
def para(c,t,x,y,w,size=9):
 p=Paragraph(t,ParagraphStyle('p',fontName='Helvetica',fontSize=size,leading=size*1.33,textColor=INK));_,h=p.wrap(w,H);p.drawOn(c,x,y-h);return y-h-8

def top(c,title,n):
 c.setFillColor(white);c.rect(0,0,W,H,fill=1,stroke=0);c.setFillColor(GREEN);c.setFont('Helvetica-Bold',9);c.drawString(40,H-30,'DnBOX R6 / MUSICAL WORKFLOW PROPOSAL / REVIEW INPUT')
 y=para(c,'<b>'+escape(clean(title))+'</b>',40,H-47,W-80,17);c.setFillColor(MUTED);c.setFont('Helvetica',8);c.drawString(40,20,'Proposal only. Independent affected assessment pending. Whole overhaul remains incomplete.');c.drawRightString(W-40,20,str(n));return y-6

def image(c,p,x,y):c.drawImage(ImageReader(io.BytesIO(p.read_bytes())),x,y-180,width=180,height=180,mask='auto')
def build():
 path=R/'DnBOX-R6-musical-workflows.pdf';c=canvas.Canvas(str(path),pagesize=A4,pageCompression=1,invariant=1);c.setTitle('DnBOX R6: musical workflow proposals');c.setAuthor('DnBOX design study');pages=[];pairs=json.loads((R/'comparisons.json').read_text())
 for n,p in enumerate(pairs,1):
  y=top(c,p['title'],n);c.setFillColor(MUTED);c.setFont('Helvetica-Bold',8);c.drawString(46,y,'ACTUAL3c41 ELITE EMULATOR');c.drawString(320,y,'R6 PROPOSAL / NOT FIRMWARE');y-=12
  image(c,R/'reference'/p['before'],46,y);image(c,R/'native'/p['after'],320,y);y-=192
  y=para(c,'Original240 x240 pixels embedded without resampling. Before inputs executed3c41; accepted d158 has identical relevant UI sources, not an executed d158 package. Proposal bars are a controlled power fixture.',40,y,W-80,8)
  for label,key in [('Keep','retained'),('Change / reconsider','changed'),('Practical purpose','why'),('Control sequence','sequence'),('Action cost','count'),('Tradeoff / unknown','cost')]:y=para(c,'<b>'+label+'</b> '+escape(clean(p[key])),40,y,W-80,8.7)
  assert y>40,(p['id'],y);pages.append({'page':n,'comparison':p['id'],'bottom_y':y});c.showPage()
 n=len(pairs)+1;y=top(c,'The whole instrument: concrete work and remaining gaps',n)
 y=para(c,'R5 was a useful conservation study, not a completed overhaul. R6 now has six connected model walkthroughs and truthful ordinary Song authoring. Existing good native behavior is retained; prototype repairs are not advertised as firmware features.',40,y,W-80)
 rows=[('Playing and all engines','909/808 typed controls and direct lower functions remain. Chicago native wave/ADSR vocabulary, bright large Juno values and Miami native family are retained. Active Drone is deferred; saved identities are not converted.'),('Sequencing and performance','Absolute64-step range, scoped locks/chance, independent current/queued patterns, cue/mute masks, one-pass override and occupied five-part copy now have concrete states and control traces. RGB pads retain source priorities; keybed lights are not invented.'),('Recording and recovery','Current stopped phrase/correction/repeated-entry data is modeled; actual3c41 has separate stored-step and three distinct nonzero PCM receipts. REWRITE is current replacement behavior, not a new take object. Running/poly/full37 recording and durable recovery remain open.'),('Generator and valid BREAK','Source NEW/MUTATE destroys more than visible cells; no preview buffer exists. Their concrete model/history remains unfinished. Actual BREAK fixture is unavailable/empty; a valid loop fixture is needed for musical override/release proof.'),('Connected control','Actual no-host Loopy and its unchanged lower pixels are compared here. Host grid/scalar/viewport operations and a legitimate SMC peer fixture are not completed. SMC normal/peek/held-FX headers require explicit power-space preservation.'),('Settings','Exact R2 component and six paired workflows are composed unchanged in HTML. Its before evidence is historical25.x; not current3c41. Common-header adoption and held-note continuity remain with its component owner.'),('Presets and lifecycle','The seven CP1 cases stay with their incumbent. Selected CP2 capture strategy follows that decision, then CP3 lifecycle. Current broadREC/ordinarySong model work does not silently freeze those future choices.'),('What the inventory means','All39 journeys and261 exact requirement rows are retained with specific remaining obligations;22 status-only R22 extraction defects are flagged. Counts, safe refusals, native pictures and model checks are not audible whole-design acceptance.')]
 for label,t in rows:y=para(c,'<b>'+label+'</b> '+escape(clean(t)),40,y,W-80,9)
 assert y>40,y;c.showPage();n+=1;y=top(c,'Evidence boundaries and the next consumer',n)
 for label,t in [
 ('Actual inputs','New isolated current3c41 Elite producer captures cover recording,64-step cue-hidden range, occupied copy, motion, panic/fresh-note, no-hostLoopy and Miami. Exact package/runtime/harness/plan/LCD hashes remain separate from proposal renderer hashes. Unchanged Pro proof is reused; no Pro/device run is invented.'),
 ('Failed attempts retained','First recording setup reachedBREAK and was silent. Negative raw encoder counts produced no turns; an earlier clear attempt leftBT unused. Corrected named attempts are visually qualified. These are producer input issues, not firmware defect findings.'),
 ('Native/model distinction','Miami factory comparison retains actual body/lower pixels; its authored-q interactive editor is separate. Chicago WAVE has a bounded source-projector recipe. Source initial/native floats, live cursors, audio backends and all-factory equivalence are not inferred from rounded UI values.'),
 ('One independent reviewer','Existing /root/pr921_latest/r3_fresh_review is the route through parent01a1af1b. R5 review does not accept R6. No new reviewer, designer or runtime writer is created. The reviewer must exercise this exact proposal and legitimate current package within the affected scope.'),
 ('Next implementation consumer','The existing firmware program owner6af85eee remains the downstream consumer within its gate. This packet is design only; no firmware edits/builds, device operation, release or merge is approved.'),
 ('Power and delivery','Top power is honest0-3bars/USB/unknown. No percentage or charging signal is fabricated. PDF is a standard parsed/rendered document; actual iOS-device opening remains NOT_RUN. Native PNGs are separately directly viewable. Library prepared-uploadHTTP401 was not retried; Git has separate owner authorization.'),
 ('Still needed','Same-hardware comparator producer output/adoption, valid BREAK fixture, SMC/clock input qualification, complete remaining unreserved journeys, protected preset interfaces, audible/durable recovery, physical recognition/readability/resources and DF1. These remain specific work; future gates do not excuse defects in this proposal.')]:y=para(c,'<b>'+label+'</b> '+escape(clean(t)),40,y,W-80,9)
 assert y>40,y;c.showPage();c.save();raw=path.read_bytes();(R/'PDF-BUILD-RECEIPT.json').write_text(json.dumps({'file':path.name,'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest(),'pages':len(pairs)+2,'pair_pages':pages,'classification':'Standard PDF; native image inputs; proposal not firmware; iOS device NOT_RUN'},indent=2)+'\n');print(json.dumps({'pdf':path.name,'bytes':len(raw),'pages':len(pairs)+2,'sha256':hashlib.sha256(raw).hexdigest()}))
if __name__=='__main__':build()
