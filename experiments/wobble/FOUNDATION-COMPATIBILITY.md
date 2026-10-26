# Exact foundation compatibility

Host-only checked against PR212 source
`6f717e216390ffca0d20be9e2204ab401ad09792`, not a copied/reimplemented foundation.
The exact `synth_engine.h` Git blob is `2ce3c273bfb48bd5e3b5b2719b4df8255555352b`;
its included `bass303.h` blob is `085b8521f7410ef03bd53aa82961ace51eaf5b60`.

`foundation_adapter.h` provides every `synth_ops_t` callback, caller-owned144-byte
context and distinct experimental ID`0x57424c31`/schema1. Sixteen documented
parameters accept integer0–127 and normalize to0–1; auxiliary pitch bend/damper
are not persisted or routed through the parameter bank. Callback note velocity
is explicit; accent and slide flags do not import303 semantics. Off releases;
panic immediately clears DSP/gate state while retaining all parameters.
Transport observation and lite quality are no-ops; transport-stop panic is owned
by the foundation. This does not claim transport-sync or an overload quality mode.

The adapter initializes at44100Hz, uses valid fragments1–256 and explicitly
downmixes `(L+R)/2`. One fixed256-float/1024-byte stack temporary stores right
output. That temporary is additional target stack pressure and is not accounted
for by144-byte context size. The combined owner must measure/rework it and extend
stereo mixing/sends before firmware registration. The header fails compilation
unless `X0X_SYNTH_HOST_TEST` is explicitly defined; no firmware registry is edited.

Given `FOUNDATION` pointing to the exact foundation checkout, from this repo root:

```sh
clang -std=c99 -O2 -Wall -Wextra -Werror -ffp-contract=off -DX0X_SYNTH_HOST_TEST -I "$FOUNDATION/firmware/src/dsp" experiments/wobble/test_foundation.c build/wobble-host/wobble.o -o build/wobble-host/test_foundation
build/wobble-host/test_foundation
clang -std=c99 -O1 -g -ffp-contract=off -fsanitize=address,undefined -fno-sanitize-recover=all -DX0X_SYNTH_HOST_TEST -I "$FOUNDATION/firmware/src/dsp" experiments/wobble/test_foundation.c experiments/wobble/wobble.c -o build/wobble-host/test_foundation_sanitized
build/wobble-host/test_foundation_sanitized
```

Both normal and ASan/UBSan checks pass. Tests compare every fragment1–256 against
the direct44100Hz stereo engine downmix, reject303/wrong-schema/out-of-range keys,
exercise request saturation and boundary ACK/panic, preserve prepared settings,
release to exact silence and clear transport-stop voices. This is interface
compatibility, not a firmware-selection, lifecycle, legacy-storage, stereo-send,
native-fit, device-timing or hardware pass. Root's combined owner remains responsible
for those stages. Existing48kHz reference measurements are unchanged.
