# Independent affected-unit native size evidence review

Result: PASS for scoped app sizing on both Elite and Pro. No compiler rerun, device access, package qualification or publication.

Reviewed unchanged runtime identity: main_fm1.c SHA256 `7c3748bc563ecf7b21eac213c2c07721bf2128f55cf10a073ab9cfb64329767b`. SMC remains installed source; PR196 is excluded. Parent owns any combined-candidate measurement.

| Model | Baseline app | Fixed app | Delta | Code free | RAM free | Pool free |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Elite |552604|552636|+32|36|300|8324|
| Pro |552604|552636|+32|36|300|8324|

Byte/hash verification performed independently:

- All189 current files in native/input-closure-check.json match recorded actual SHA256 values; recorded accepted hashes match task69/compiled-inputs.json's189-file closure. The sole changed compiled input is main_fm1.c, at the reviewed hash above.
- All4 generated headers and21 cached objects hash identically to the corresponding task69/codegen/build/gen and task69/codegen/build/smk37-elite bytes. This includes extra cached candidates not selected by the final linker; inspected tools/build.py links the normal startup objects, changed x0x.o and unchanged O2 units.
- Provenance hashes for task69 compiled-inputs.json, delivery-receipt.json, pro-build-receipt.json and toolchain-verification.json match their stored accepted receipts. Accepted toolchain verification reports no mismatches.
- Elite baseline and fixed x0x.bin lengths/SHA256 and x0x.o SHA256 match measurement.json. Baseline recompilation uses the installed firmware snapshot and reports552604; fixed reports552636.
- Pro fixed x0x.bin length/SHA256 and x0x.o SHA256 match measurement.json, and its current-source hash matches reviewed runtime. Pro baseline552604 is reused from accepted task69/pro-build-receipt.json with exact e75fb2a source; its stored app bytes still match accepted SHA256. A new Pro baseline compiler run is not claimed.
- Both command records use native r3/mfprev1, BOARD_SMK37=1, DEV X0X_DEV_PADCAL=1, X0X data-family/untagged policy and fixed generated-header directory. Elite explicitly defines SMK_MODEL="SMK-37 Elite" and FELUCCA_ID="SMK-37 Elite_900". Pro correctly uses the existing fm1_cc.h default SMK_MODEL="SMK-37 Pro" and FELUCCA_ID="SMK-37 Pro_900". Unchanged DSP/seq/engine object model reuse has no SMK_MODEL/FELUCCA_ID reference and both variants use the same SMK hardware board.
- Both measurements have empty native check errors; notes report app code area552672, data+bss98004/98304 and pool335740/344064, yielding36/300/8324 free. Independently parsed both final symbol tables _bss_end/_pool_start/_pool_end to confirm RAM300/pool8324 free; both final O2 object directories remain byte-identical to cached originals.

Fixed app SHA256:

- Elite `4b4139a3669479f9509069985c17c91b73285fa9a52b4275c330a033fcd43ed0`
- Pro `daf28b1a79825f14ef22e9158e9c2432ebb46c17e5418932001b29d4247eea89`

This is AFFECTED_TU_RECOMPILE_AND_APP_RELINK_ONLY. It establishes measured target-image cost for this isolated patch; no new loader/controller build, installable package/model negatives, hardware validation or combined-source DEV CHECKED status is asserted. No added static storage is observed in total RAM use. A transient read-only inspection failed before execution with exec-server transport disconnected; the later readback succeeded, so no dependent review remains blocked.
