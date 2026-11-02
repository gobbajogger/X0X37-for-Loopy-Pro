> Historical pre-measurement assessment. Subsequent bounded host measurements are in REPORT.md; this document’s NOT_RUN statements describe its own stage. No production trim was selected.

# L12: Miami louder — scoped source assessment

Scope: assessing increased Miami loudness, not an invented dB target. Existing L12/PR799 measurement flow retained. No production edit, test/render, timed benchmark, device operation or publication was performed in this assessment.

## Exact source and known evidence
Published8be16486e33f65b31c76710e1d825b6aa701c2c9, independently recovered source closure. engine.c8ddeb9d588ece0612b8aedaafe1900d573c7215f; original_miami_backend.c2efde1794c8454b7af0f9c527f49abd03a28a783. PR799 fresh read: open draft, head9973e7e947420dcab0f059cc6261983592350cd3, branchchat/live-a-safety-023. Its stated scope is measurement tooling, not levels/trims; tests and LU/peak measurements in its receipt remain NOT_RUN. Do not claim its producer owns a new gain patch. Historical ROADMAP L12 correspondence: line284 at496c8726, in progress.

No fresh or verified historical device peak/RMS/headroom numbers were established here. Earlier host CPU/PCM equivalence results are not loudness/headroom evidence. Listening feedback is not a measured dB deficit.

## Gain path
1. Voice/filter/VCA output enters Miami's original internal output chain. experiments/original_synth/miami_output.c60–84 reads native parameter96 volume, applies internal fixed-FX/decimator and calls original_master_process_i32 with volume². original master target is4×volume² (master.c36); do not change this authored engine behavior casually.
2. original_miami_backend.c546–556 copies the resulting binary32 stereo words into float output; there is no hidden post-render attenuation there. original_runtime.c277–299 dispatches it without an extra Miami gain factor.
3. engine.c1833–1894 applies preset-switch fade, per-part bus shaping, common part Level and pan; part Level is(pot/100)² at534–535. Default both bass Level100 isunity (649). Level127 is1.6129 gain (~4.15dB relative100), a range fact, not unused safe headroom or a recommendation to max it.
4. mix_targets1605–1621 applies that part gain to dry and reverb/delay sends. Sends use stereo average, preserving current routing. Shared FX follows1898–1904; final master compression/PUMP, DJ filter, physical master volume, lookahead limiter/soft ceiling follow master.c333–385. Limiter OFF still hard-clamps±1. Raising gain can produce more compression/limiting rather than proportionate output level.
5. NORMAL vs LOUD engine.c568–575 scales the entire final mix4194303 vs8388607. It is global, not a Miami-only fix. USB-full routing/physical master positions must be pinned; do not silently change either.

## PERF contamination
Actual host reproduction already proved PERF changes the project sound and applied PUMP3→21 and can persist defaults on later saves. It also resets compression/makeup/FX/preset settings. Do not use PERF TEST to prepare loudness comparisons. Pin complete sound_t, admitted banks/presets, mixer/master controls, output mode and note/velocity sequence before and after each capture. Do not tell the owner to overwrite/reload a valued unsaved project merely to measure.

## Reuse existing measurement, with limits
PR799 tools/live_a_mix_report.py uses nineteen mix/solo/bank rows, preset0, matched factory A musical line in slotsA/B, source/script/sample/WAV hashes, and actual runtime admission at capture start/end. Reuse its driver, not its old firmware ancestry. It delegates to unchanged tools/mix_report.py: ungated whole-loop K-weighted LEFT-channel LU and LEFT sample peak, not integrated stereo LUFS or true peak. Its 'raw' solo disables master compressor/PUMP only; FX, inserts, part levels and limiter remain. It cannot alone prove unclipped stereo headroom.

Existing commands (NOT_RUN for this assessment; exact driver+8be application closure must first be admitted and recorded):
```
python3 tools/live_a_mix_report.py --prepare-host --source-root . --outdir build/live-a
CC="$PWD/build/live-a/cc-live-a" X0X_HOST_OUT=build/live-a/x0x_host sh host/build_host.sh
python3 tools/live_a_mix_report.py --source-root . --host build/live-a/x0x_host --outdir build/live-a/before --label before
python3 tools/live_a_mix_report.py --before build/live-a/before/report.json --after build/live-a/after/report.json --outdir build/live-a/comparison
```
The currently recovered local source is a hash-verified sparse closure, not a Git checkout; the driver's source_snapshot requires genuine gitHEAD/status. Do not fabricate that provenance. Existing qualified executor must provide a real exact checkout, or the measurement owner must review a narrowly scoped manifest-provenance adapter. No old PR799 production hunks should be grafted just to obtain the tool.

## Minimal bounded next stage
1. Reuse any existing exact-source L12 capture if its pins/settings match; otherwise one baseline capture, no timed CPU benchmark. Record stereo peak, RMS, crest factor, finite/clipped sample counts, pre-master peak and limiter gain-reduction occupancy in a disposable host observation hook. Retain PR799's LU metric with its honest label; supplement stereo analysis, not silently relabel LUFS.
2. Workload minimum: Miami preset0 in A and B on identical line; owner-selected quiet preset if supplied; representative loud/high-resonance/dense-voice preset from already existing30-preset corpus; tails/retrigger and low/highvelocity. Compare Miami solo, Miami+909+BREAK, Miami+Chicago, and twoMiami. Pin dry/noexternalFX vs ordinarymix separately, same master settings and NORMAL/LOUD negatives. Existing preset0 nineteen-row data may cover much of this; do not rerender covered unchanged cases.
3. Determine whether the deficit is part balance, preset output, master gain reduction or global output scaling before choosing a gain. Derive candidate trim bound from measured peak/headroom and mix clipping, then present short level-matched and normal-level listening comparisons. No arbitrary permanent boost or dynamic normalization, and no automatic compressor/limiter/preset changes.
4. If a common Miami-only trim is justified, prefer a multiplier in the common post-engine part-gain route, AFTER existing bfx_shape and before dry/send mix gain: engine.c1869–1894 mix_targets + corresponding part_peak/send treatment. Gate by actual admitted runtime engine identity, not slotA/B. This preserves original oscillator/filter/internalFX arithmetic and presets; bus sends intentionally rise with dry unless owner expressly chooses otherwise. Do not boost internalVCA/VCO/master or modify the master limiter to conceal overs. Audibly changed final PCM is intentional and must not be called bit-exact; pre-trim engine PCM and unaffected Chicago-only/909-only outputs must remain exact.
5. Existing engine producer must explicitly hand off that bounded engine.c gain/meter/send hunk through the sole owner. PR813/814 caches, Miami zero-XMOD work, PERF restoration and existing reviewer claims remain untouched. Candidate requires zero unintended settings/preset mutations, no wrap/NaN, acceptable clipping/limiter exposure, preserved tails/stereo/sends/mute/Solo/fader glide, and native memory/performance gates later. No current RAM/speed benefit is claimed.

Next stage requires the existing L12 measurement owner/qualified executor; source change remains unadmitted until measured cause, trim choice and exact hunk ownership are established. This is the same L12 feedback, not another backlog item.
