"""Assemble the R3 design workbench. No firmware build or execution."""
import base64, hashlib, json
from pathlib import Path

ROOT=Path(__file__).parent
BASE=ROOT
def sha(b):return hashlib.sha256(b).hexdigest()
def uri(p):return 'data:image/png;base64,'+base64.b64encode(p.read_bytes()).decode()

def build():
    params=json.loads((ROOT/'parameter-control-map.json').read_text())
    drums=json.loads((ROOT/'drum-function-map.json').read_text())
    counts=json.loads((ROOT/'source-access-counts.json').read_text())
    pairs=[
      dict(id='drum',title='Find the drum without losing the sequencer',before='909-four-kicks-playing-lcd.png',fixture='drum',retained='All eleven tracks plus accent;16 columns; K5 length; K6 rate; PATCH+black mute; current pad route.',changed='New instrument pictures replace row abbreviations. A large selected-instrument readout uses only the previously unmapped K7/K8 display area. Those knobs remain unmapped.',why='The selected full name anchors the tiny row picture. No selection, audition, step-write or mute gesture is added.',cost='Recognition of tiny and808variant pictures is unqualified. The new right-half readout must not imply editable K7/K8 controls.',sequence='Running909SEQ: G#3 selects snare; pad4 toggles step5. K5/K6 still edit length/rate directly.',count='Same source-derived sequence and input count; no timed/device result. Four-kick comparison fixture uses kick only.'),
      dict(id='chicago',title='Keep the sound overview and direct access',before='chicago-page1-lcd.png',fixture='chicago',retained='Four neighboring rows in one matrix, physical K5–K8 columns, existing parameter glyphs/ADSR, reverse wrap, PARA cycle and SCALE deep list.',changed='The rejected focused page is withdrawn. R3 returns to current-source overview topology and includes SOUND in the full8page cycle.',why='The current UI already lets a player see the next group and reach cutoff from SEQ in two navigation gestures plus one value turn.',cost='No improvement over current parameter navigation is claimed. It is retained. These are model pixels, not exact current firmware font/glyph output.',sequence='Known OSC page memory: PARA → K2+ → K5. Existing deep list: SCALE → K2+ → K2+ → K3 → PATCH.',count='3gestures direct;5via deep list, identical in retained source routes. Mechanical detents and time unmeasured.'),
      dict(id='home',title='Keep the whole pattern and four performance controls',before='home-five-part-stopped-lcd.png',fixture='home',retained='Sixteen pattern thumbnails, five part-content lanes, current/queued identity and four graphical PERFORM controls.',changed='WAIT/fault feedback stays in the header rather than replacing Home with a diagnostic list. A read-only coarse power picture occupies the strip-caption gap.',why='A pending cue or missing clock must not hide pump/filter or the playing context.',cost='Power semantics are source-derived coarse/USB/unknown, not percentage/charging. Collision and physical readability still require current execution/review.',sequence='Home: K8 adjusts pump; K7 adjusts filter. A#3 requests BREAK mute. No extra page.',count='0navigation+1value gesture for pump. Same count while cue/WAIT is displayed.'),
      dict(id='song',title='Keep neighboring bars and both BAR control pages',before='song-bar-stopped-lcd.png',fixture='song',retained='Up to11visible bar entries, selected/playing bar, per-part patterns/mutes/repeats, and both four-knob BAR pages.',changed='The rejected one-entry list is withdrawn. Empty Song remains an empty entry with its existing add-bars hint.',why='Seeing neighbors is useful arrangement context. A prettier isolated bar does not justify losing it.',cost='This is existing manual authoring. CP2 strategy and CP3 recording lifecycle remain unresolved; no new recorder or chooser is frozen.',sequence='From Home: PATCH → Song. K2+ changes to BAR2. Its K8 is repeats; PATCH returns Home.',count='Existing source route retained; no added page/confirm. Populated11row capacity is a separate source-layout fixture.')
    ]
    for p in pairs:
        f=ROOT/'reference'/p['before'];p['before_image']=uri(f);p['before_sha256']=sha(f.read_bytes())
    data={'source_commit':'3c41ef4bedf9701f549ce2752b73306e2fab1eb8','source_tree':'fa0c3fe75483534f12839eeb8f5e29ec9cb52c6d','engines':params['engines'],'drums':drums['drum_voice_map'],'trackMaps':params['drums'],'rates':['1/16','1/16T','1/32','1/8T','1/8','1/4'],'pairs':pairs,'counts':counts['scenarios'],'drum_counts':drums['representative_task_counts']}
    if (ROOT/'proposal-images.json').exists():data['proposalImages']=json.loads((ROOT/'proposal-images.json').read_text())
    fonts=[]
    for weight,file in [(500,'BarlowSemiCondensed-Medium.ttf'),(600,'BarlowSemiCondensed-SemiBold.ttf')]:
        b=(BASE/'fonts'/file).read_bytes();fonts.append(f"@font-face{{font-family:Barlow;font-weight:{weight};src:url(data:font/ttf;base64,{base64.b64encode(b).decode()})}}")
    template=(ROOT/'workbench.template.html').read_text()
    result=template.replace('/*FONTS*/','\n'.join(fonts)).replace('/*DATA*/',json.dumps(data).replace('</','<\\/')).replace('/*RENDERER*/',(ROOT/'proposal.js').read_text())
    (ROOT/'workbench.html').write_text(result)
    (ROOT/'model-inputs.json').write_text(json.dumps({'html_sha256':sha(result.encode()),'renderer_sha256':sha((ROOT/'proposal.js').read_bytes()),'source_commit':data['source_commit'],'source_tree':data['source_tree'],'parameter_map_sha256':sha((ROOT/'parameter-control-map.json').read_bytes()),'drum_map_sha256':sha((ROOT/'drum-function-map.json').read_bytes()),'current_actual_captures':'NOT_RECEIVED','emulator':'Parent fresh3c41Elite/Pro report relayed; report/launchbytes notyetavailable here; no rerun'},indent=2)+'\n')
    print(json.dumps({'html_bytes':len(result.encode()),'html_sha256':sha(result.encode()),'engine_page_counts':{k:len(v['pages']) for k,v in data['engines'].items()}}))

if __name__=='__main__':build()
