# Independent combined integration QA

Scope: sequential host correctness of experimental DEV backend/mixer, default303 behavior and release exclusion. No source modifications by reviewer. Root remains sole integrator.

PASS:
- Own ASan+UBSan review.c harness: stereo exactly direct original output for fragments1..256; bounded mono exactly(L+R)/2 for all fragments; output canaries intact; bad bounds/alias refusal; switch saturation/ACK/back-to303 cleanup; stale engine/schema/index rejection; off/release silence and transport panic retaining parameters; malformed 7-bit bank refusal without mutation; byte-bank/LXSE roundtrip.
- Own actual whole-app build against frozen foundation6f717e216390ffca0d20be9e2204ab401ad09792 and recovered candidate default:1267756 WAV bytes identical, SHA256 a75af3fc060631d1140b3970ae7a7a492e9937a60f88f852e4e4f85f8d7f7030.
- Actual recovered engine.c included unchanged via scratch wrapper. Private scratch host hook sets W_STEREO=1 and part303A reverb100/delay95. Full7490ms scenario matched independent dedicated-array/pre-render reference byte-for-byte:1267756 bytes, SHA256 db993853d8f1fd2788a4a8d22247a4a048849da7723c50ef996c481b32e94d99. This tests side preservation, sequential reuse and send downmix without adding product API.
- Experimental whole-app compiles/renders and differs from default, confirming flag selects alternate.
- Existing526336-sample303 suite/firmware registry negatives and70509-check state suite rerun; backend normal+ASan/UBSan/release preprocessing exclusions rerun.
- Release compiler probes refuse alternate without DEV_PADCAL and refuse X0X_VERSION with alternate. Source build.py excludes alternate object unless flag1 and rejects versioned/non-DEV alternate.

No actionable host correctness defect found. Recovered sequential mixer reuses output scratch only after core2 finishes and consumes each part into dry/send buses before rendering the next; no extra stereo array remains. Default conditional path retains original render order, verified by bytes.

Reviewed source SHA256:
engine.c3031a1a26684b3dfd558ad8f51e782a28e4726d30ecf76c4a3cd317c7a1ded24
synth_engine.h93820335e3d433cfc3a71b78d7f6b868231d72449c9a712d23dde93cd3d4a326
wobble_backend.h90904505c8ed8b3a0781af684596649f8d877f91ea9606f83ebc10f6c95c76b9
wobble_config.hfa7bc6659e60a8b221af9029f094f5f0cc68c3598360eb4ef0b717d3d4098b48
build.py1bebd5267efcd130a1519bfad3a60e0be57a843ab91ad125b92d3c60fe1482ac

Reproduction scratch: review.c, mix_check.py, side-sends.x0x, exact_engine_wrapper.c, qa_host.c, reference_engine.c. mix_check.py constructs wrapper/reference/host and runs actual app comparison; local paths must be adapted when moved. No private WAV committed.

Acceptance is HOST_CORRECTNESS_SCOPED only. Host fidelity remains FAILED: envelope/spectrum/history mismatches are not fixed by integration. Native full links remain failed per independent native build owner; no native-fit, measured target high-water/cycles/deadline, hardware or installability acceptance. Byte-bank/LXSE boundary is tested but no public project/file/flash/editor/undo persistence route is implemented. Boot-only DEV fixture has no public menu/switch/control API. These limits remain open and are not waived by host passes.
