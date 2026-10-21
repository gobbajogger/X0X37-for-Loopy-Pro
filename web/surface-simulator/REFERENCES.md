# Manufacturer reference evidence — 2026-10-21

## Provenance

All original images and PDFs were downloaded from official manufacturer catalog links. No Pro or 25 model substituted for Elite. `products-data.js` is retained as evidence only, never executed.

- Catalog: https://www.cuvave.com/products-data.js (Elite object lines 297–365; SMC object lines 584–642).
- Elite product page: https://www.cuvave.com/product?id=smk-37-elite
- Elite top image: https://www.cuvave.com/images/productInfo/smk-37-elite-1.webp
- Elite angled/rear/detail images: same directory `smk-37-elite-2.webp` through `smk-37-elite-5.webp`.
- Elite manual: https://manualf.oss-cn-hongkong.aliyuncs.com/manual/MIDI-KEYBOARD/SMK-37Elite-manual.pdf
- SMC product page: https://www.cuvave.com/product?id=smc-mixer
- SMC top image: https://www.cuvave.com/images/productInfo/smc-mixer-1.webp
- SMC angled images: same directory `smc-mixer-2.webp`, `smc-mixer-3.webp`.
- SMC manual: https://manualf.oss-cn-hongkong.aliyuncs.com/manual/MIDI-KEYBOARD/SMC-MIXER_ZE.pdf

Visually inspected exact top and angled images for both devices, Elite rear image and manual panel diagram. `elite-panel.png` is rendered from PDF page 4 (printed EN 02). Text extracted into `elite-manual.txt` and `smc-manual.txt`.

## Physical inventory and layout

### SMK-37 Elite

Manufacturer dimensions: 458 × 181 × 50 mm. Top photographed body approx bounds x40–1120, y40–460 in a 1160×500 image. Top aspect about 2.57; published physical aspect 2.53. Photo-derived SVG viewBox 1000×395 preserves physical overall aspect approximately; these are visually traced coordinates, not CAD measurements.

- Dark gray body, orange wheels, encoders, fader caps, buttons and accidental keys; ivory naturals/pads.
- 37 piano keys: 22 ivory naturals, 15 orange accidentals. Three full C–B octaves plus final C. NOT 37 white keys.
- Two vertical wheels at left above keyboard, pitch on left and modulation on right.
- Display at x134,y18,w85,h83 in 1000-wide trace. LCD visible area x143,y27,w67,h65.
- Bank button cluster below display: 2 columns × 3 rows. Top KNOB BANK/FADER BANK; middle left/right arrows; bottom OCTAVE-/OCTAVE+. PAD BANK legend spans top two buttons, not a separate physical button.
- 8 encoders in ONE horizontal row, centers x274+60.7i,y38, radius18.
- 16 pads in TWO rows × EIGHT columns. Upper row trace top-left x250+60.7i,y70; lower y115.5; dimensions48×38. No invented pad-bank extra row.
- 4 faders at upper right x772+62.3i; slot y28–109.
- 12 function buttons in TWO rows × SIX columns below faders, x751+37.1i, y130/146. Top labels: ARP, NOTE REPEAT, SCALE, CHORD, GLOBE, BT. Bottom: PATCH, PARA, FX, SEQ, SEQ PLAY, SEQ REC. Printed GLOBE is intentional.
- Keys start x28,y209; white pitch42.8,width41.8,height180. Accidental width23,height110; center at relevant natural-key boundaries.
- Top photo has subtle key-function legends above keyboard. Manual diagram additionally shows transport symbols below pads; top photograph does not clearly show them, so they are not added as fictional buttons.
- Back edge, viewed from rear, left-to-right in connector recess: SWITCH, charging indicator, USB-C, MIDI OUT 3.5mm, AUDIO 3.5mm, SUSTAIN 1/4 inch.

### SMC-Mixer

Manufacturer dimensions: 256 × 122 × 40 mm. Top image 1000×1000 has large transparent padding: body x48–954,y305–737. Trace is 1000×477 based on published physical ratio.

- Light gray/silver body, black encoders and fader caps, light square track buttons with gray rims.
- 8 columns, each with rotary encoder on top, a short LED directly below, long vertical fader, FOUR track buttons vertically to the RIGHT of its fader.
- 32 track buttons: M (mute), S (solo), R (record), hollow square (select), in that order top-to-bottom. Do not place buttons in rows under all faders.
- 11 global buttons on a SINGLE horizontal row along bottom: play, stop (physical icon appears as ||), record, rewind, fast-forward, previous 8-track bank, next 8-track bank, up, down, left, right.
- BT and SHIFT are TWO EXTRA physical buttons at upper far right. Total physical buttons45 = 32 track + 11 globals + 2 extra; manufacturer count43 is assignable buttons, not complete physical count.
- Encoders trace center x53+118.4i,y70,r31; LED y114; fader slot y152–357; track button xcenter+43, y162+51.8j,w33,h33.
- BT trace x932,y48,w52,h24; SHIFT x932,y106,w52,h24.
- Global buttons trace x64+82.7i,y410,w47,h23.
- No screen, no master ninth fader, no pads.

## Geometry deliverable

`surfaces.js` exports `renderSmk()` and `renderSmc()` returning native SVG strings. All geometry is newly authored. Each operable control group has `data-control`. Pad IDs run upper row left-to-right 0–7, lower8–15; key IDs are chromatic left-to-right0–36. These are geometry IDs only; native firmware logical mapping must be applied separately by integration owner.

Images/PDF are local research evidence, not licensed redistributable assets; do not bundle original manufacturer materials into source deliverables without a separate licensing basis. Native SVG recreation contains no embedded manufacturer photo. No third-party script was executed. `node` successfully imported module and rendered both SVG strings; XML verification performed separately. CairoSVG was absent; both SVGs were then raster-rendered using installed Inkscape and visually inspected. Geometry matches the verified anatomy. Consuming owner's interactive browser verification remains required.
