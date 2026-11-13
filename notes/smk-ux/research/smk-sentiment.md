# M-Vave SMK-37 user sentiment (stock firmware and hardware)

**Research date:** 13 Nov 2026, Asia/Vladivostok (UTC+10).
**Purpose:** evidence for the DnBOX firmware UX overhaul. Hardware references are secondary.
**Method:** WebSearch and WebFetch only. No browser, no sign-ins. Every quote below is copied from a page that WebFetch returned. Quotes that appeared only in search snippets, or only on aggregator pages, are **not** treated as quotes.
**Scope:** M-Vave / Cuvave SMK-37 Pro and SMK-37 Elite, their stock firmware and workflow, and any public posts about DnBOX / X0X37 firmware.

## How thin or strong the evidence is

Public writing about this keyboard is real but **narrow**. There is no magazine lab test. The useful material is one German professional review (Heise, Jul 2025), one English written review (AltWire, Sep 2025), a Synth Anatomy first-look with owner comments (Jul 2025), a long Elektronauts thread (Jun 2025–Oct 2026), and a handful of Reddit original posts. YouTube reviews exist; their transcripts were **not** fetchable here, so they are not quoted. Amazon listed 31 ratings (4.1/5, 15% one-star) but **no review bodies** came back. AliExpress and Sweetwater review text was not fetchable. Gearspace and KVR returned no useful threads. ModWiggler has a thread (`viewtopic.php?t=294075`) but the fetch was **403 Forbidden**, so it is not quoted. r/LoopyPro, r/WeAreTheMusicMakers, r/ipadmusic, and r/midicontroller (as distinct from r/midi) yielded no fetchable SMK-37 discussion.

**Strongest theme (firmware):** note and sequencing **latency / dropped notes**, reported independently by Heise, two Elektronauts owners (Pro), one Elite owner, and a Reddit r/midi post, then described as fixed by stock firmware 016 (12 Aug 2026) in both the vendor notes and a later Elektronauts post.

**Strong and mixed:** value for money (almost always positive); build and keys (usable for the price, with small/springy keys and wobbly or loose faders); the FM engine as a selling point, with limited on-device editing, basic or poor effects, and early pop/click bugs.

**Moderate:** awkward hold-to-edit menus and mode confusion; USB-MIDI port hassle; mixed Bluetooth experience; novice overwhelm.

**Thin:** actual **screen contrast / glyph legibility** (almost no one complained they could not read the 1.54-inch display; the real screen complaint is a **short timeout**); **save/recall** (vendor changelog added “manual save for the Sequencer”; no owner quote about lost patterns was fetched); **stage / live gig** use (portable sofa and commute use is attested; no gig report was fetched); the **manual** as a document (one Elite novice still felt lost after reading it).

**Very thin / none:** public user sentiment about **DnBOX** or **X0X37**. One Elektronauts post reports that X0X37 exists. No review, rating, or owner workflow comment was found.

Firmware versions matter. Early mid-2025 posts describe first firmwares (around 011–013). Firmware 013/014 (Jul 2025) claimed to fix pops and to add manual sequencer save. Firmware 015 (26 Jan 2026) added wheel calibration. Firmware 016 (12 Aug 2026) claimed to fix key MIDI-note latency and unstable tempo. Several strong negative latency quotes predate 016. The one Elite usability critique (krunchr, Apr 2026) is on **firmware 015**.

## Stock firmware versus hardware

Complaints that belong to **hardware** (firmware cannot truly fix them): mini-key size and springy feel; pad size and velocity; wobbly or liftable faders; plastic body; no 5-pin DIN and **no MIDI In**; no speaker; battery and Bluetooth radio as physical features; pitch/mod wheel mechanics (firmware added a calibration mode, which is an admission of a hardware-plus-firmware fault, but a failed wheel is still a hardware problem).

Complaints that belong to **stock firmware and its workflow**: note-sending latency and dropped first notes after a screen change; pops and clicks on patch or FX changes; awkward hold-to-show-parameter operation; short display timeout; patch-select that needs a held button; buggy arpeggiator “Random”; poor or worsened FX after an update; limited on-device FM editing; DX7 import parameter mapping; wrong notes or wrong scale display; USB-MIDI port naming; companion Midi Suite and Mac M-UPGRADE quality; missing manual sequencer save until firmware 013/014; unstable tempo until 016.

Praise that is **hardware**: surprising Elite keybed, battery, headphone jack, pads with aftertouch, portability, feature count for the price.

Praise that is **firmware / workflow**: on-screen note and chord names; DX7-style engine and SysEx loading; sequencer modes; later pop and latency fixes; Bluetooth MIDI when it works.


## Findings table

Confidence is the number of **independent fetched sources** that mention the same theme, not the number that use the same wording. A single row is still one quote. Ellipses in square brackets (`[…]`) mark a middle cut of a longer fetched sentence. Typos inside quotes are as published.

| Theme | +/- | Exact short quote | Source URL | Date (as published) | Device / firmware | Confidence |
|---|---|---|---|---|---|---|
| Build and keys | + | "The hardware is fine, the keybed, in particular, is surprisingly good." | https://www.elektronauts.com/t/m-vave-smk-37-pro-affordable-dx7-emulator/234750?page=2 | 21 Apr 2026 | SMK-37 Elite (owner since Feb 2026; firmware 015) | 7 sources, mixed |
| Build and keys | + | "For the price (or even 2x the price) its a nice keybed IMO." | https://www.elektronauts.com/t/m-vave-smk-37-pro-affordable-dx7-emulator/234750?page=2 | 28 Jul 2025 | SMK-37 Pro | 7, mixed |
| Build and keys | − | "The keys are quite small. […] for me the keys are not as good as for example from the Arturia Keystep 37 (or even ESI XSynth …)." | https://www.elektronauts.com/t/m-vave-smk-37-pro-affordable-dx7-emulator/234750?page=2 | 28 Jul 2025 | SMK-37 Pro, firmware 013 (shows as 1.10) | 7, mixed |
| Build and keys | − | "the beat pads are small and not very responsive, the sliders are wobbly and plasticky and the keys are springy withou any feedback." | https://synthanatomy.com/2025/07/m-vave-smk-37-pro-this-100e-usb-midi-controller-features-a-built-in-dx-7-synthesizer-engine.html | 16 Jun 2025 (comment by Dronte) | SMK-37 Pro; first MIDI keyboard | 7, mixed |
| Build and keys | − | "the four faders – which, in contrast to the rotary knobs, are somewhat wobbly to the touch." | https://www.heise.de/en/background/MIDI-keyboard-with-DX7-emulation-M-VAVE-SMK-37-Pro-tested-10503740.html | 30 Jul 2025, 7:38 am CEST (13:38 Asia/Vladivostok) | SMK-37 Pro; reviewer notes two firmware updates already | 7, mixed |
| Build and keys | − | "it’s generally quite sturdy but being so cheap I have found that two of the faders can be easily lifted clean is so that’s worth bearing in mind." | https://synthanatomy.com/2025/07/m-vave-smk-37-pro-this-100e-usb-midi-controller-features-a-built-in-dx-7-synthesizer-engine.html | 18 Jul 2025 (comment by Dom) | SMK-37 Pro (owner also had SMK-25) | 7, mixed |
| Build and keys | + | "Considering its price, it feels more like a $250-300 MIDI controller and doesn’t feel like junk." | https://altwire.net/m-vave-smk-37-pro-review/ | 7 Sep 2025 | SMK-37 Pro | 7, mixed |
| Build and keys | + / − | "The 37-note mini keybed isn’t in Yamaha Reface territory but is playable." | https://altwire.net/m-vave-smk-37-pro-review/ | 7 Sep 2025 | SMK-37 Pro | 7, mixed |
| Screen (chord/note readout) | + | "the little screen tells you what note you are playing, and more cool - it even tells you the chord you are playing when you play a chord. Given I have no musical training this is a nice to have for me." | https://www.elektronauts.com/t/m-vave-smk-37-pro-affordable-dx7-emulator/234750?page=2 | 28 Jul 2025 | SMK-37 Pro | 3 on readout; **0** on contrast/legibility |
| Screen (chord/note readout) | + | "When playing notes, it also shows the note being played and, if several keys are pressed simultaneously, even which chord is being played. This is certainly useful for beginners." | https://www.heise.de/en/background/MIDI-keyboard-with-DX7-emulation-M-VAVE-SMK-37-Pro-tested-10503740.html | 30 Jul 2025, 7:38 am CEST | SMK-37 Pro | 3 on readout |
| Screen (timeout, not contrast) | − | "The display's timeout is much too short, and the buttons and knobs are often very fiddly to use." | https://www.elektronauts.com/t/m-vave-smk-37-pro-affordable-dx7-emulator/234750?page=2 | 22 Apr 2026 | SMK-37 Elite, firmware 015 | 1 on timeout — thin |
| Menu depth and mode confusion | − | "All the functions sound great. The awkward operation often spoils the enjoyment of the device." | https://www.heise.de/en/background/MIDI-keyboard-with-DX7-emulation-M-VAVE-SMK-37-Pro-tested-10503740.html | 30 Jul 2025, 7:38 am CEST | SMK-37 Pro | 3 independent reviews/owners |
| Menu depth and mode confusion | − | "To change the patch, press the “Patches” button and turn the rotary control of the desired bank. After releasing the “Patches” button, the selection is active. This is unnecessarily cumbersome." | https://www.heise.de/en/background/MIDI-keyboard-with-DX7-emulation-M-VAVE-SMK-37-Pro-tested-10503740.html | 30 Jul 2025, 7:38 am CEST | SMK-37 Pro | 3 |
| Menu depth and mode confusion | − / + | "It took me a bit to get used to the button combinations and menu shortcuts, but once I learned them, navigation was smooth enough that it didn’t hinder creativity." | https://altwire.net/m-vave-smk-37-pro-review/ | 7 Sep 2025 | SMK-37 Pro | 3 |
| Menu depth and mode confusion | − | "Not to mention the poor usability of the device itself." | https://www.elektronauts.com/t/m-vave-smk-37-pro-affordable-dx7-emulator/234750?page=2 | 22 Apr 2026 | SMK-37 Elite, firmware 015 | 3 |
| Menu depth and mode confusion | − | "The hardware is solid and well equipped — it has a huge display, a great keybed, a battery, Bluetooth and TRS MIDI out — but the software and the general user experience are sloppy." | https://www.elektronauts.com/t/m-vave-smk-37-pro-affordable-dx7-emulator/234750?page=2 | 22 Apr 2026 | SMK-37 Elite, firmware 015 | 3 — cleanest hardware/firmware split |
| Learning curve for novices | − | "I’ve gone from being absolutely clueless about synthesisers and MIDI keyboards, to curious, to frustrated by the lack of tutorials specific to this device, and finally overwhelmed with all the knobs and buttons, when I tried jumping in without any guidance." | https://www.reddit.com/r/synthesizers/comments/1sqxvie/complete_newbie_with_the_mvave_smk37_elite_where/ | 20 Apr 2026 (page said “7mo ago”; title date from search: 20 Apr 2026) | SMK-37 Elite; complete beginner; had read the manual | 2–3 — moderate, small sample |
| Learning curve for novices | − | "I’ve gone through the user manual, but most of the tutorials and guides online seem to be aimed at people who already have some synth experience, which I definitely don’t have yet!" | https://www.reddit.com/r/synthesizers/comments/1sqxvie/complete_newbie_with_the_mvave_smk37_elite_where/ | 20 Apr 2026 | SMK-37 Elite | 2–3 (same post; also a manual finding) |
| BT / MIDI reliability | + | "For my purposes it is almost perfect as it’s compact and the Bluetooth function eliminates the need for more cables in my tiny recording space." | https://synthanatomy.com/2025/07/m-vave-smk-37-pro-this-100e-usb-midi-controller-features-a-built-in-dx-7-synthesizer-engine.html | 18 Jul 2025 (comment by Dom) | SMK-37 Pro | 5, mixed |
| BT / MIDI reliability | − | "on Android I didn’t manage it to work properly in any but one app (“Piano MIDI BLE USB”). That’s a pitty, because when you are on the move, you can’t send to your phone neither a BT audio, nor a BT MIDI, and have to use the OTG cable anyway (which BTW works perfectly)." | https://synthanatomy.com/2025/07/m-vave-smk-37-pro-this-100e-usb-midi-controller-features-a-built-in-dx-7-synthesizer-engine.html | 16 Jun 2025 (comment by Dronte) | SMK-37 Pro | 5, mixed |
| BT / MIDI reliability | − | "Getting USB MIDI to run is a bit of a hassle. In Studio One I needed to add an external device as “Instrument” which uses MIDIIN3 and MIDIOUT2 from the SMK-37 Pro" | https://www.elektronauts.com/t/m-vave-smk-37-pro-affordable-dx7-emulator/234750?page=2 | 28 Jul 2025 | SMK-37 Pro, firmware 013 | 5 (USB-MIDI setup, not radio) |
| BT / MIDI reliability | − | "The user interface of Midi Suite for the Mac is a complete mess with truncated text labels, incomplete drop-down menus, etc." | https://www.elektronauts.com/t/m-vave-smk-37-pro-affordable-dx7-emulator/234750?page=2 | 22 Apr 2026 | SMK-37 Elite, firmware 015; companion app, not device UI | 5 |
| BT / MIDI reliability | − | "Found, M-Update app, but so far only on PC, Mac app is not working…" | https://www.elektronauts.com/t/m-vave-smk-37-pro-affordable-dx7-emulator/234750?page=3 | 28 Apr 2026 | SMK-37 (updater tool) | 5 |
| Latency | − | "As for a seasonal musician, too much latency, for some sounds unbearable (for me at least)." | https://www.elektronauts.com/t/m-vave-smk-37-pro-affordable-dx7-emulator/234750 | 24 Jun 2025 | SMK-37 Pro, early firmware | **5+** — strongest negative theme |
| Latency | − | "It has a lag of nearly a whole quarter note at 120bpm. This makes it really difficult to record something with interesting timing like 1/8T with the arpeggiator." | https://www.elektronauts.com/t/m-vave-smk-37-pro-affordable-dx7-emulator/234750?page=2 | 28 Jul 2025 | SMK-37 Pro, around firmware 013 | 5+ |
| Latency | − | "I think it’s still fun, but nothing for “serious” production – at least in its current state." | https://www.elektronauts.com/t/m-vave-smk-37-pro-affordable-dx7-emulator/234750?page=2 | 28 Jul 2025 | SMK-37 Pro | 5+ |
| Latency | − | "For real FM fun I use my miniDEXED, and the SMK is just a doorstop for me because of the latency and delay… Sad." | https://www.elektronauts.com/t/m-vave-smk-37-pro-affordable-dx7-emulator/234750?page=2 | 29 Jul 2025 | SMK-37 Pro | 5+ |
| Latency | − | "what is currently most annoying is the lag or delay that the keyboard delivers in synthesizer mode. You can create a complete song in the DAW with it, but the timing of the individual tracks is a bit off – for professional use." | https://www.heise.de/en/background/MIDI-keyboard-with-DX7-emulation-M-VAVE-SMK-37-Pro-tested-10503740.html | 30 Jul 2025, 7:38 am CEST | SMK-37 Pro | 5+ |
| Latency | − | "Until a future firmware update provides corrections here, the built-in synthesizer of the M-VAVE SMK-37 Pro is only useful for sketchy strumming on the sofa, for example." | https://www.heise.de/en/background/MIDI-keyboard-with-DX7-emulation-M-VAVE-SMK-37-Pro-tested-10503740.html | 30 Jul 2025, 7:38 am CEST | SMK-37 Pro | 5+ |
| Latency | − | "the effects are really poor and the latency is very noticeable." | https://www.elektronauts.com/t/m-vave-smk-37-pro-affordable-dx7-emulator/234750?page=2 | 22 Apr 2026 | SMK-37 Elite, firmware 015 | 5+ (still present on Elite / 015) |
| Latency | − | "MIDI notes are sometimes output delayed when using the Elite as a MIDI controller." | https://www.elektronauts.com/t/m-vave-smk-37-pro-affordable-dx7-emulator/234750?page=2 | 22 Apr 2026 | SMK-37 Elite, firmware 015 | 5+ |
| Latency (tied to a screen change) | − | "as soon as the \"KEYS\" screen (note/chord reader) disappears and goes to the main page, there's a time window were, for some reason, it doesn't register the first two MIDI notes played (if fast). […] There's also a lag on PATCH mode." (The same post marks this "UNRELATED TO BLUETOOTH".) | https://www.reddit.com/r/midi/comments/1vogyx2/mvave_smk_37_pro_and_elite_key_notes_lag/ | 14 Aug 2026 (page said “3mo ago”; title date from search: 14 Aug 2026) | SMK-37 Pro and Elite; firmware not stated (two days after 016 was released) | 5+ |
| Latency (after the 016 fix) | + | "When I’ve lost all my hopes… THEY FINALLY FIX IT!!! Yes, they fix the latency problem on note-in / note-out. Update 16 did it. NOW I can play SMK-37." | https://www.elektronauts.com/t/m-vave-smk-37-pro-affordable-dx7-emulator/234750?page=3 | 30 Aug 2026 | SMK-37, stock firmware 016 (12 Aug 2026). Same owner who called it a “doorstop” in Jul 2025. | 5+ theme; **1** post-fix confirmation |
| Save / recall | (vendor note, not a user quote) | Firmware 013/014 changelog: "Added manual save for the Sequencer." | https://raw.githubusercontent.com/jonathaslacerda/smk-37-pro-docs/main/firmware/FIRMWARE.md | 17–18 Jul 2025 | SMK-37 Pro firmware 013/014 | **Thin:** 1 vendor bullet, **0** fetched owner quotes about lost takes |
| Manual | − | "I’ve gone through the user manual, but most of the tutorials and guides online seem to be aimed at people who already have some synth experience, which I definitely don’t have yet!" | https://www.reddit.com/r/synthesizers/comments/1sqxvie/complete_newbie_with_the_mvave_smk37_elite_where/ | 20 Apr 2026 | SMK-37 Elite | **Thin:** 1 fetched owner. A Jul 2025 Reddit post claiming the official site had no manual was **not** successfully fetched, so it is not quoted. |
| Sound engine | + | "Hi, I have my SMK-37 since thursday, and the synth is amazing!" | https://synthanatomy.com/2025/07/m-vave-smk-37-pro-this-100e-usb-midi-controller-features-a-built-in-dx-7-synthesizer-engine.html | 16 Jun 2025 (Dronte) | SMK-37 Pro | 7+, mixed |
| Sound engine | + / − | "The DX7 built in engine is pretty good, but as is noted there is sometimes some crackle/pop on certain presets, hard to reproduce." | https://www.elektronauts.com/t/m-vave-smk-37-pro-affordable-dx7-emulator/234750?page=2 | 28 Jul 2025 | SMK-37 Pro | 7+ |
| Sound engine (pops — firmware) | − | "buffer overruns cause light clicks here and there, not terrible but really annoying, when you record something, and there is sudden click, you must re-record." | https://www.elektronauts.com/t/m-vave-smk-37-pro-affordable-dx7-emulator/234750 | 24 Jun 2025 | SMK-37 Pro, early firmware | 7+ (pops also in Heise and later marked fixed) |
| Sound engine (pops after update) | + / − | "Updated my unit yesterday and the pops when loading a new patch seem to be gone entirely. […] The FX weren’t any good already, but now I think they are even worse (referring to Delay and Reverb)." | https://www.elektronauts.com/t/m-vave-smk-37-pro-affordable-dx7-emulator/234750?page=2 | 31 Jul 2025 | SMK-37 Pro, “new firmware” of late Jul 2025 (013/014) | 7+ |
| Sound engine (editing) | − | "the SMK-37 Pro does not offer full or even intuitive control over the values of the 6-operator FM patches – the numerous rotary controls would actually be ideal for this." | https://www.heise.de/en/background/MIDI-keyboard-with-DX7-emulation-M-VAVE-SMK-37-Pro-tested-10503740.html | 30 Jul 2025, 7:38 am CEST | SMK-37 Pro | 7+ |
| Sound engine (editing) | − / + | "The SMK-37 Pro lacks the Reface’s incredible feel and true FM editing capabilities, but offers DX7 patch imports that the Reface DX lacked. The Reface is better for programming, while the SMK is better for quick ideas." | https://altwire.net/m-vave-smk-37-pro-review/ | 7 Sep 2025 | SMK-37 Pro | 7+ |
| Sound engine (banks) | − | "You can only keep four banks on the hardware at a time, but with the app, you can swap between ROM sets." | https://altwire.net/m-vave-smk-37-pro-review/ | 7 Sep 2025 | SMK-37 Pro | 7+ |
| Sound engine (FX) | − | "An FX section offers distortion, reverb and delay. However, the quality is very basic." | https://www.heise.de/en/background/MIDI-keyboard-with-DX7-emulation-M-VAVE-SMK-37-Pro-tested-10503740.html | 30 Jul 2025, 7:38 am CEST | SMK-37 Pro | 7+ |
| Sound engine (import) | − | "Some imported DX7 presets differ significantly from the original presets. I think there’s something wrong with the parameter mapping during import." | https://www.elektronauts.com/t/m-vave-smk-37-pro-affordable-dx7-emulator/234750?page=2 | 22 Apr 2026 | SMK-37 Elite, firmware 015 | 7+ |
| Sound engine (arp bug — firmware) | − | "The arpeggiator is currently faulty, “Random” only plays the notes in the upward direction." | https://www.heise.de/en/background/MIDI-keyboard-with-DX7-emulation-M-VAVE-SMK-37-Pro-tested-10503740.html | 30 Jul 2025, 7:38 am CEST | SMK-37 Pro | 2 on this exact bug (Heise + DeKoepi) |
| Sound engine (arp bug — firmware) | − | "The arpeggiator is buggy - “random” just goes up, even in the new 013-firmware" | https://www.elektronauts.com/t/m-vave-smk-37-pro-affordable-dx7-emulator/234750?page=2 | 28 Jul 2025 | SMK-37 Pro, firmware 013 | 2 |
| Sound engine (scale bug — firmware) | − | "With some scales, every few notes a wrong note is played (it’s C8 everytime, you can see it on the display)." | https://www.elektronauts.com/t/m-vave-smk-37-pro-affordable-dx7-emulator/234750?page=2 | 22 Apr 2026 | SMK-37 Elite, firmware 015 | 1 fetched owner (vendor 013 notes also say “Fixed incorrect display for specific SCALE types”) |
| Live use (portable practice) | + | "The fact that it’s battery powered & has headphone jack is neat so I can just sit on the sofa and jam by myself." | https://www.elektronauts.com/t/m-vave-smk-37-pro-affordable-dx7-emulator/234750?page=2 | 28 Jul 2025 | SMK-37 Pro | 4 on portable practice; **0** on stage gigs |
| Live use (portable practice) | + | "I play on my daily train commute with it and a pair of headphones, and the phone BT for DX7 patch editing. it’s light and small enough to fit in my backpack, and for the price it has no rivals." | https://synthanatomy.com/2025/07/m-vave-smk-37-pro-this-100e-usb-midi-controller-features-a-built-in-dx-7-synthesizer-engine.html | 18 Jul 2025 (comment by DerekW) | SMK-37 Pro | 4 |
| Live use (timing for “serious” work) | − | "the built-in synthesizer of the M-VAVE SMK-37 Pro is only useful for sketchy strumming on the sofa, for example." | https://www.heise.de/en/background/MIDI-keyboard-with-DX7-emulation-M-VAVE-SMK-37-Pro-tested-10503740.html | 30 Jul 2025, 7:38 am CEST | SMK-37 Pro, pre-016 | 4 (same latency theme) |
| Value (cross-cutting) | + | "I’d recommend this to anyone on a budget in heartbeat." | https://synthanatomy.com/2025/07/m-vave-smk-37-pro-this-100e-usb-midi-controller-features-a-built-in-dx-7-synthesizer-engine.html | 18 Jul 2025 (Dom) | SMK-37 Pro | Strong agreement across almost every fetched source |
| Hardware limit: no MIDI In | − | "If you’re looking to input external MIDI, there is no MIDI In." | https://altwire.net/m-vave-smk-37-pro-review/ | 7 Sep 2025 | SMK-37 Pro | 2 (AltWire + Heise on missing DIN / no adapter) |
| DnBOX / X0X37 | (existence only) | "X0X37 and FoMni37: Charles Vestal’s fm1-x0x (909, 808, two 303s) and fm1-fomni (Omnichord-style, the pads are the strum plate)." | https://www.elektronauts.com/t/m-vave-smk-37-pro-affordable-dx7-emulator/234750?page=3 | 14 Oct 2026 | Third-party firmware, reported not reviewed | **Very thin:** 1 report, 0 owner reviews |

### Vendor changelog bullets that corroborate user themes (not user quotes)

From the community transcript at https://raw.githubusercontent.com/jonathaslacerda/smk-37-pro-docs/main/firmware/FIRMWARE.md, and from the 016 notes posted on Elektronauts on 30 Aug 2026 (https://www.elektronauts.com/t/m-vave-smk-37-pro-affordable-dx7-emulator/234750?page=3):

- 013/014 (17–18 Jul 2025): "Fixed occasional popping when switching presets"; "Added manual save for the Sequencer"; "Fixed incorrect display for specific SCALE types"; "Optimized UI overlay logic across interfaces"; "Adjusted UI delay for short-press actions."
- 015 (26 Jan 2026): "Added a calibration mode for malfunctioning Pitch/Mod wheels"; "Fixed the issue where Pitch/Mod wheels were not moving smoothly."
- 016 (12 Aug 2026): "Resolved MIDI note‑sending latency issue for keys"; "Resolved unstable tempo issue"; "Chord: Recognition Expansion" / "Chord Recognition Expansion."

These are useful as vendor admissions. They are not owner sentiment.

## DnBOX / X0X37 public sentiment

**Sources are thin to empty.** The only fetched public comment is Jukka’s 14 Oct 2026 Elektronauts post, which *reports* that X0X37 exists as a port of Charles Vestal’s fm1-x0x (909, 808, two 303s), installable from a browser, and says of a demo video covering several alternate firmwares: "This demo is more or less someone just banging on things, without thinking about being musical." That sentence is about a video of several firmwares, not a played-in review of DnBOX, and the poster twice writes "I’m reporting, not advocating."

Web search also surfaced project pages (genkigenki.github.io/smk37, altstrument.org, rackears.io) that describe the firmware. Those are project listings, not user reviews. Rackears was summarised as having no ratings. No Reddit, Gearspace, KVR, YouTube-description, or Amazon thread about DnBOX or LoopyX0X37 was fetched.

**Implication:** the UX overhaul cannot lean on existing DnBOX public praise or blame. The usable sentiment is about the **stock SMK instrument and how people failed or succeeded at making music with it**.

---

## Implications for DnBOX UX

Each item is a problem, the evidence, and a design response aimed at the 909 / 808 / Chicago / Miami / Juno / Drone screens, at first play (sound, then sequence, then save), at presets, and at state clarity (sound, step, save, playing, queued). Hardware complaints are noted, then set aside.

### 1. Latency and dropped notes (firmware — highest stakes)

**Problem.** Owners and a professional reviewer treated timing as the reason the SMK was or was not a real instrument. One Elite owner on firmware 015 still found latency “very noticeable.” One Reddit post ties missed first notes to the moment the KEYS (note/chord) screen closes. A later owner says firmware 016 fixed note-in / note-out latency.

**Evidence.** Heise 30 Jul 2025; Elektronauts sm7x7 (24 Jun, 29 Jul 2025 and 30 Aug 2026), DeKoepi (28 Jul 2025), krunchr (22 Apr 2026); r/midi u/PSn505 (14 Aug 2026); vendor 016 notes.

**Design response.** This is a hard constraint on the overhaul, not a visual theme. Engine-screen changes, piano-roll and chord labels, preset recall, and playing/queued transitions must **not** stall note scan — the stock KEYS-screen dropout is a warning against UI work that blocks input. Measure, never slower. Decorative transition delays (stock 015 even listed "Added delay effects to some UI interface transitions") are the wrong lesson. First play and live state (playing vs queued) may only change if the change is free at the audio and MIDI edge. If flash is tight, cut features before cutting timing. Applies to every engine screen equally.

### 2. Awkward menus, holds, and mode confusion (firmware)

**Problem.** The stock panel is feature-rich and awkward: hold a button to see parameters, release Patch to confirm a sound, short display timeout, fiddly knobs. Reviewers say the function list is impressive and the operation spoils it. An Elite owner split this cleanly: solid hardware, sloppy software and UX. One reviewer did learn the shortcuts and then worked smoothly — so experts can adapt, but the first hour is costly.

**Evidence.** Heise (“unnecessarily cumbersome”, “awkward operation often spoils the enjoyment”); AltWire (button combinations and menu shortcuts); krunchr (timeout, fiddly controls, “sloppy” UX) on Elite 015.

**Design response.** On every engine screen (909, 808, Chicago, Miami, Juno, Drone), the sounding parameters and the current engine identity should be visible **without a hold**. The six screens must look like one family and still be unmistakably different instruments, so the player is never unsure which engine and which page they are on. First play must not use a Patch-hold-and-release dance to get a sound. Preset recall and save should be a visible act with a visible result (saved vs unsaved), not a hidden hold. State (sound, step, save, playing, queued) stays on screen and does not time out mid-task. Do **not** replace DnBOX gestures or hide the full kit to achieve this — show the current mapping, do not remap muscle memory. (Conflict with owner rules is noted below.)

### 3. First play and the novice learning curve (firmware / documentation)

**Problem.** A complete beginner with an Elite read the manual, found tutorials aimed at people who already know synths, and felt overwhelmed by knobs and buttons. The stock device is two products at once (DX7-style synth and DAW controller). USB-MIDI setup is a hassle. The useful beginner aid that reviewers actually praised is the **on-screen note and chord name**, not a deeper menu.

**Evidence.** r/synthesizers newbie post 20 Apr 2026; Heise and stevebk on chord/note display; DeKoepi and Heise on USB-MIDI port setup.

**Design response.** First play is: a sound that is already an instrument (a 909 kit, a Chicago bass, a Juno, a Drone — not an unlabeled FM bank), a sequence the pads or keys can enter, then an obvious save. That path should not require a computer, Midi Suite, or FM theory. Advanced engine pages stay available (full kit). Chord and note names stay near the notes, with honest compact notation, because that is the stock behaviour beginners already valued. Engine screens should read as “this is the drum machine / this is the bass” rather than as a generic parameter dump.

### 4. Save, presets, and state clarity (firmware — evidence thin, still actionable)

**Problem.** Public owner quotes about lost patterns were not fetched. The vendor still added “manual save for the Sequencer” in 013/014, which is a signal that save was incomplete. Reviewers talk about eight patterns and four sound banks, and about a companion app to swap banks. State of “is this stored?” is not something owners praised.

**Evidence.** Thin: changelog only, plus AltWire’s “four banks on the hardware at a time” and Heise’s “save eight patterns.”

**Design response.** Do not treat this as a loud public wound, but do not leave it implicit. First play ends in a save the player can see. Preset and pattern journeys show what will survive a power loss. The save state is a first-class indicator next to playing and queued, on every engine screen. A short display timeout (krunchr) must not hide “unsaved.” Honesty about bank limits beats a silently rotating buffer.

### 5. Sound-engine expectations (firmware; DnBOX is a different instrument)

**Problem.** People bought the SMK for a cheap DX7-style engine, SysEx loading, and sofa play. They liked the sounds when they worked. They criticised limited on-device editing, only four banks, basic or worsened FX, pops on preset change, import mapping that did not match the original patch, and a broken Random arp. Stock pads in synth mode address one patch, not a drum kit (Heise). DnBOX will be judged as 909/808/303/Juno/Drone, not as a better Dexed.

**Evidence.** AltWire, Heise, Dronte, stevebk, kosmonaut, krunchr, fform (Synth Anatomy comment: modern patches reduced glitches; hoped for “more complete onboard editing”), DeKoepi on Random.

**Design response.** Each engine screen should present a musically complete instrument on first play (drums actually on the 909/808, a bass on Chicago, and so on), with presets that are useful as-is and truthful about what they are. Parameter control that is already on the knobs and faders should be labelled on the screen, answering the “lots of knobs, no intuitive edit” complaint, without pretending DX7 SysEx compatibility the firmware does not have. Preset changes and FX must not pop (that was a stock reliability failure). Shared FX, if kept, have to be musically usable — owners noticed when a firmware update made delay and reverb worse.

### 6. Screen, glyphs, and chord notation (firmware)

**Problem.** No fetched owner said the 240-class display was unreadable. They did say the note/chord readout helps, and one Elite owner said the timeout is too short. A later Reddit post claims note dropouts when that KEYS overlay closes. Vendor 016 lists “Chord Recognition Expansion.”

**Evidence.** stevebk, Heise, krunchr, PSn505; 016 notes. **Legibility evidence is thin.** Timeout and chord utility are real.

**Design response.** Restore and keep shipped glyphs; do not drop them from mockups. Keep and improve compact real chord notation on the SEQ / piano-roll path, because that is a praised stock behaviour. Prefer a stable, always-visible label over a fading overlay that can coincide with missed notes. Do not spend the overhaul on decorative header labels that leave engine identity and play state unclear. If a chord is ambiguous, be honest — that is already the chord-spec rule — and do not let a heavy label path affect timing. Playing, queued, and save state should share that same always-visible discipline.

### 7. Bluetooth, MIDI, and companion software (mixed hardware / host / firmware)

**Problem.** Bluetooth is a reason people like the box (fewer cables, commute, small rooms). Android BLE MIDI and Windows/Mac companion tools are messy. USB-MIDI port names confuse DAW setup. This is mostly **not** a DnBOX screen problem, and firmware cannot invent MIDI In.

**Evidence.** Dom, Dronte, DeKoepi, Heise, krunchr (Midi Suite), sm7x7 (Mac M-Update).

**Design response.** First play, sequence, and save must work on the device alone. Do not design a preset or first-play journey that requires Midi Suite. If Local / USB / host state exists, show it clearly. Be honest in the product story: wireless convenience is hardware; reliability complaints are often the host stack. No UX copy should promise zero-latency Bluetooth.

### 8. Live and portable use (mixed)

**Problem.** The loved scenario is sofa, commute, and headphones — a reliable, immediate instrument. The hated scenario is “I tried to record or play seriously and the timing was off.” No fetched source describes a stage gig. After 016, one former critic could “play SMK-37” again.

**Evidence.** stevebk, DerekW, Heise, sm7x7 on 016; absence of gig reports (thin for stage).

**Design response.** Design for both the sofa first-play and a set where playing vs queued must be obvious at a glance. Transport, queued patterns, and the sounding engine have to be readable in one look on every engine screen (909 through Drone). Reliability (no pops, no delayed notes, no wrong C8) is part of live UX. Do not treat “portable sketchpad” as a reason to leave state unclear.

### 9. Build and keys (hardware — secondary)

**Problem.** Mini keys, springy feel, small pads, wobbly or detachable faders. Elite keybed gets genuine praise. Firmware will not make the keybed into a Reface.

**Evidence.** krunchr, stevebk, DeKoepi, Dronte, Heise, Dom, AltWire.

**Design response.** Do not spend the overhaul pretending to fix the keybed. Do give clear velocity / pad / step feedback so the player can trust what they hit. Avoid UI that depends on tiny precise holds the same owners already call fiddly. Wheel and fader faults stay in hardware/QA, not in a new glyph.

---

## Themes that conflict with owner rules

Owner rules to keep: **performance**, **gestures**, the **full kit**, and **expert muscle memory**.

1. **Performance vs a richer UI.** Latency is the sentiment that decided whether owners treated the SMK as an instrument or a doorstop. Any overhaul that adds chord labels, piano roll, more glyphs, or preset journeys at the cost of note timing **conflicts** with both the owner performance rule and the public record. The design response is to keep the visual work off the audio/MIDI path and to measure. Stock firmware’s own “delay effects to some UI interface transitions” is a pattern not to copy.

2. **Novice first-play vs full kit and expert gestures.** Public novice overwhelm pushes toward a simpler first screen. Hiding engines, removing shipped glyphs, or changing established DnBOX gestures to look more like a “tutorial device” would conflict with “keep the full kit and expert muscle memory.” The non-conflicting path is a clear default (sound → sequence → save) **on top of** the existing kit, with mode and mapping visible, not a reduced instrument.

3. **Hold-to-edit is unpopular in stock, but DnBOX already has its own holds and gestures.** Replacing those gestures to answer Heise’s “cumbersome” Patch hold would break expert memory. Answer the complaint by making parameters and state visible without stealing the existing gesture set.

4. **Chord and note display.** Public sentiment **supports** keeping and improving this (Heise, stevebk, vendor “Chord Recognition Expansion”). Removing it to save flash or to tidy a mockup would go against both users and the owner chord-notation brief. Expanding it must still respect performance and honest naming.

5. **Hardware wish-lists (MIDI In, speakers, better keys, DIN).** These conflict with reality, not with owner rules. They should stay out of firmware promises.

## Sources fetched and used

| Source | URL | Result |
|---|---|---|
| Heise (Dirk Knop) | https://www.heise.de/en/background/MIDI-keyboard-with-DX7-emulation-M-VAVE-SMK-37-Pro-tested-10503740.html | Full article, 30 Jul 2025 |
| AltWire (Derek Oswald) | https://altwire.net/m-vave-smk-37-pro-review/ | Full review, 7 Sep 2025 |
| Synth Anatomy + comments | https://synthanatomy.com/2025/07/m-vave-smk-37-pro-this-100e-usb-midi-controller-features-a-built-in-dx-7-synthesizer-engine.html | First look 17 Jul 2025; owner comments Jun–Jul 2025 |
| Elektronauts pp. 1–3 | https://www.elektronauts.com/t/m-vave-smk-37-pro-affordable-dx7-emulator/234750 (and `?page=2`, `?page=3`) | Jun 2025–Oct 2026, Pro and Elite |
| r/midi latency OP | https://www.reddit.com/r/midi/comments/1vogyx2/mvave_smk_37_pro_and_elite_key_notes_lag/ | OP only (comments did not come back) |
| r/synthesizers newbie OP | https://www.reddit.com/r/synthesizers/comments/1sqxvie/complete_newbie_with_the_mvave_smk37_elite_where/ | OP only (comments did not come back) |
| Community firmware notes | https://raw.githubusercontent.com/jonathaslacerda/smk-37-pro-docs/main/firmware/FIRMWARE.md | Changelog transcript, not sentiment |
| Amazon US listing | https://www.amazon.com/keyboard-controller-production-responsive-sensitivity/dp/B0FG1LPLVR | 4.1/5 from 31 ratings; **no review text** |
| Audiowords consensus page | https://www.audiowords.net/reviews/m-vave-smk-37-elite | Used only as a pointer (22 sources listed, updated 9 Oct 2026). Its Reddit and YouTube excerpts were **not** independently fetched, so they are not quoted here. |

## Sources searched or attempted, not used for quotes

- ModWiggler thread: https://www.modwiggler.com/forum/viewtopic.php?t=294075 — WebFetch **403**.
- old.reddit.com and reddit `.json` comment pages — **timed out**. Several other Reddit threads (wheels, manuals-missing, MiniLab comparison, Windows Bluetooth, firmware 015 announcement) appeared in search and were not fully fetched; their snippet text is **not** quoted.
- YouTube reviews (including https://www.youtube.com/watch?v=Dzs5a99n5D0, plus others named by Audiowords: Marius Tarceatu, BeatProcess, EZ): descriptions and transcripts were not fetched as reliable quote sources.
- Gearspace, KVR, Sweetwater, AliExpress review bodies, r/LoopyPro, r/WeAreTheMusicMakers, r/ipadmusic: no fetchable SMK-37 user discussion in this pass.
- Official M-Vave / Cuvave product and manual pages were not required for sentiment and were not used as user quotes.

## What this file is not

It is not a claim about current DnBOX quality. It is not a licence to advertise “best in class” or “zero latency.” Early latency quotes may not apply to a unit on stock firmware 016. Elite and Pro share workflow complaints; the Elite keybed is the main hardware upgrade owners called out.
