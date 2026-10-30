# Drum MIX material candidate

The owner's requested 2.5D treatment now applies to the shared 909/808 MIX strip renderer. Voice captions move above the meters; the selected caption has a filled part-colour face. Shallow strip wells distinguish the rails without helper text. This is actual source, not a proposal image.

The level rail remains 96 pixels tall. Eleven voices, including 808 CP, independent stored level / transient hit / mute display, pan geometry, selection, and all control mappings remain unchanged. No audio, storage, firmware package, boot or website code changes.

Host proof: `sh tests/host/ui_drum_mix_material_test.sh`. The fixture covers both kits, zero/full/intermediate levels, pan endpoints/centre, hit zero/full, mute, selected first/CP/last voices, project nonmutation, inter-strip clipping, band boundary/rail endpoints and existing level/pan mappings in both banks. Synthetic engine telemetry is not hardware or audio validation.

Exact before/after images and logs are in the sibling `ui-mix-material-evidence-20261027` directory. The original renderer passes state/mapping fixtures but fails the new top-caption material assertion, as expected for this requested presentation change. It is not represented as a pre-existing functional bug.

Owner preview and independent review remain pending. No rollout, production/device acceptance, native build or final boot render is claimed.
