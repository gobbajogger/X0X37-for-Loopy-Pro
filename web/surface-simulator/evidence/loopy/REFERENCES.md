# Loopy surface on a 13-inch iPad Pro

Verified 2026-10-21. This is a reference for an **interactive, modeled example**, not a reconstruction of the user's unavailable saved Loopy project. The current Apple specs page describes M5 hardware; the requested generation was unspecified. Publisher screenshots include older iPad artwork and must not be used as 13-inch hardware measurements.

## Hardware geometry

Apple specifies **281.6 × 215.5 × 5.1 mm**, with a **2752 × 2064** display at **264 ppi**. In landscape, the outer width:height is **1.3067285**, while the display is exactly **4:3**. [Apple specifications](https://www.apple.com/ipad-pro/specs/)

Derived—not a manufacturer bezel measurement: pixel pitch implies a display bounding rectangle of approximately **264.776 × 198.582 mm**, leaving centered insets of **8.412 mm** left/right and **8.459 mm** top/bottom. These are **2.9873% of body width** and **3.9253% of body height**. Rounded-corner radii in the helper are illustrative. Landscape front-camera placement is on the long bezel. [Apple dimensions image](https://www.apple.com/v/ipad-pro/specs/e/images/specs/weight_dimensions_2__oa0ioy4jxvey_large.png)

## Authentic Loopy behavior (condensed)

The canvas is customizable, not a fixed pad grid. Upper tools include project, undo/redo, transport, session recording and tempo. Lower tools include mixer, sequencer, edit and clear; the bottom module strip holds inputs/effects. A compact mixer sits below a reduced canvas; its expanded mode exposes routing.

Clip state: playing uses its group color; stopped uses black; muted uses dim color; recording uses orange-red. Transitional count-ins/outs use wipers. Color groups aggregate clips into mixer channels and share routing/settings—not universal instrument categories. Audio loops have circular outlines and radial waveforms; MIDI clips use dotted outlines.

Default tap toggles playback, or recording when empty; two-finger tap overdubs. These gestures are configurable. MIDI bindings trigger actions. Loopy normally returns feedback on the binding's message/channel, but controller compatibility and configuration determine whether LEDs respond. Never imply an unverified hardware feedback path works. [Official manual, sections 1, 3, 5, 6 and 13](https://loopypro.com/manual/)

## Verified visual reference inventory

Links are references only: no third-party screenshots are packaged here.

| Reference | Dimensions | Use |
| --- | --- | --- |
| [Publisher iPad clip canvas, App Store](https://is1-ssl.mzstatic.com/image/thumb/Purple122/v4/8d/a8/d5/8da8d599-d43d-a55a-9293-a14a9748194f/b272f462-9668-4b9a-9eea-038c2b280c13_English_iPad_Pro_3rd_Gen_1.jpg/626x834bb.webp) | Browser decodes 625×834 | Visually checked: marketing crop; dark canvas, circular outlines, bright radial waveform donuts, square custom pads, bottom module strip. Not a full un-cropped app screen. |
| [Publisher App Store page](https://apps.apple.com/us/app/loopy-pro-looper-daw-sampler/id1492670451) | Responsive page | Expand “iPhone, iPad” to expose iPad screenshots. |
| [Compact mixer](https://loopypro.com/wp-content/uploads/2021/12/1600px-Mixer_in_Simple_Mode_2.0-1024x250.jpg) | 1024×250 | Visually checked: black background, vertical rounded dark strips, color cap and footer, thin fader tracks, M/S and balance controls. |
| [Expanded mixer](https://loopypro.com/wp-content/uploads/2021/12/Extended_Mixer_2.0_For_Manual-1024x648.jpg) | 1024×648 | Manual-linked routing view. |
| [Upper toolbar](https://loopypro.com/wp-content/uploads/2026/07/UpperToolsPhone-1024x92.jpg) | 1024×92 | Phone-labelled toolbar crop; icon identity only, not iPad spacing. |
| [Lower toolbar](https://loopypro.com/wp-content/uploads/2026/07/LowerToolsPhone-1-1024x133.png) | 1024×133 | Phone-labelled crop; icon identity only. |
| [Clip-state comparison](https://loopypro.com/wp-content/uploads/2021/12/clip-states-239x300.jpg) | 239×300 advertised | Official manual-linked state examples. |
| [Recording state](https://loopypro.com/wp-content/uploads/2021/12/recording_state-297x300.jpg) | 297×300 advertised | Official manual-linked recording example. |

## Proposed demo interaction—not a verified device map

Keep the firmware/backend demonstration distinct from modeled Loopy behavior. Show separate colored lanes for MIDI commands, audio and state feedback. Do not animate audio down a MIDI-only link.

1. SMK pad press: illuminate the physical pad, pulse its command path, identify the selected modeled Loopy clip, then show queued→recording→playing as appropriate. If showing note routing instead of an action binding, label the destination instrument explicitly.
2. SMC control change: show the exact control ID/value entering x0x, then its actual mapped output; update only the destination the firmware map establishes. A modeled parameter change should visibly move the corresponding on-screen control.
3. Feedback: a separate return pulse should update modeled hardware LEDs only when explicitly enabled. Label this as simulated unless the real return-path mapping has been verified.
4. Retain a readable explanation beside the selected route: source → message/transformation → target → state. Make empty, recording, playing and queued states distinguishable with text, not color alone.

## Raised frame helper

`ipad-13-frame.css` and `ipad-13-frame-fragment.html` provide original CSS geometry; move the existing live canvas into `.lp-ipad13__display`. No screenshot background or replacement control logic is required. This preserves native DOM input. The optional angled view is presentation only; `.is-flat` restores a directly readable performance view. Set `--ipad-width` once; body depth scales from Apple's 5.1/281.6 ratio.
