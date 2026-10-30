# Selected lane UI revision

Removes the temporary screen’s Select button depiction. A solid white lane title identifies selection; amber control focus and the lower value readout identify the touched control. LOCK continues to name captured target context. Main MIX follows its channel editor selection; drum MIX follows the visible kit even when a different part is current. Selection outside a captured bank produces no title highlight.

Physical Select, repeat-press main/drum mixer switching, lock behavior and knob/fader targets are unchanged. UI framebuffer checks cover both kits, MIX/SEQ, both drum banks, main/FX/master, separate touch focus, and locked selection. Tests also require all eight strips and absence of the Select row. Three new runtime mutations test incorrect touch-based selection, visible-kit selection and main MIX selection. Independent review caught and verified the visible-kit case.

Elite DEV size check passed with 660 B code, 316 B RAM and 8324 B pool free. This is BUILD_ONLY evidence. Prior unchanged controller/input preparation and interaction evidence from PR168 are reused; no hardware qualification or rollout is implied.
