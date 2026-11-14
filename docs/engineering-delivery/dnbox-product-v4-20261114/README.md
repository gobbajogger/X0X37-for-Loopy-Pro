# DnBOX V4 reviewed product source / NIGHTLY preview candidate

The product page now explains the actual five-part firmware and musician workflows, centred on a genuine captured session and source-backed examples of changing pattern lengths, step chance/locks, recorded knob movement, bass generation, BREAK slicing and SONG arrangement. It adds a direct existing-installer CTA and clearly separate project/sample/preset, emulator and hardware-MIDI paths. The musician can understand what replaces the factory firmware and choose a real next step.

This directory preserves a reviewed **text overlay** and website screenshots. It is not a deployed site or firmware release. The complete runnable site and full review/render evidence are preserved separately in the V4 Library deliverable. No installer/catalog/package, firmware, emulator, codec or MIDI-adapter source is changed. Keeping an overlay avoids redistributing the original audio asset into this source archive.

## Exact reconstruction

Baseline is the53-file V3 site at NIGHTLY gh-pages4c580acfd50b0ec4c429ba5c7d0db7d8beb43614, prefixpreviews/dnbox-v3-20261114/. Library input: libfile_fde3_f-5f36010bb1b88191861379c30c3fa9a8_2021479872048191acbfdca1f870a539, ZIP SHA2569ce98216ab67fdc226e56bc7b7bf31a7634162a077dfa63f68fb3f953559b241. All53 baseline files matched both Library and live HTTP bytes. BASELINE.json records every exact hash.

Extract the original ZIP safely, then run:

```
python3 reconstruct_preview.py /path/to/DnBOX-v3 overlay /new/isolated/output
```

The helper verifies the exact baseline, copies it into a new directory, applies only eight reviewed overlay members (five code files plus README/history/manifest), and verifies the55-file result. Serve the output with python3 -m http.server. Do not overwrite the live installer or V3.

## Review and proof

Independent proposed-design review preceded implementation. Fresh independent full-product evaluation received no prior scores/targets and inspected24 actual desktop/phone renders and28 internal routes. It found four corrections; all are recorded in evidence/FINDINGS-RESOLUTION.md. Changed-only final independent recheck is in review/. Actual local Chromium sweep:48 responsive route renders,23 interaction assertions, no overflow/missing images/script errors after fixes. Source syntax and exact reconstruction pass. These buildless website checks require no firmware/native build.

The53 existing firmware/audio/capture/provenance assets remain exact baseline except the documented code/docs overlay. Public playable emulator and hardware companion are unavailable; their acceptance is not claimed. DNPR PR915 is file-only and undeployed. Firmware examples remain0.0.25.1, distinct from installer's current available builds. Physical device, acoustic, real phone/browserOS and live-origin Chromium acceptance are not established.

Normal verified-TLS HTTP reads of installer/projects/samples returned200 with recorded hashes. Chromium external navigation failed environment certificate trust. The attempted persistent trust addition was rejected before execution; no TLS bypass or security setting change occurred. Live-origin browser journeys must be checked by the existing publisher/tester in a normally trusted browser.

## Next authorized stage

The existing accountable NIGHTLY preview publisher consumes this exact candidate and final review, places it only at a new previews/dnbox-v4-20261114/ path, independently checks the preview diff contains55 runtime files and leaves installer/catalog/packages/V3/unrelated paths unchanged, uses its existing authorized PR/deploy route, verifies live HTTP byte identity and tests phone-width/navigation/install/file/emulator-status/companion-status journeys. No production deployment, binary publication, real emulator enablement or device write follows from website preview authority.

This executor exposes no supported Work-thread creation/readback tool. No successor is claimed. The coordinating root receives the concrete immutable packet and must verify the existing publisher's actual consumption before calling the stage delivered.
