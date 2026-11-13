emu360 (fm1-live) box layout notes (2026-11-13)
===============================================

Portable usage and the heavy-load gate are in README.md in this folder.
The paths below record the DnBOX box layout where this stand-in scene
was first run; they are not required to use the scripts.

Trees (leave in place):
  emu:      /workspace/emu360-test/X0X37-for-Loopy-Pro-8c8fa82b789d7dabde04d2fa2334b5616becda47
  binary:   .../target/release/fm1-live   (4.8M, cargo 1.85.1, release+LTO)
  25.1 src: /workspace/emu360-test/X0X37-for-Loopy-Pro-78f81bad07b8e53161326c056a2111c663349772
  Elite:    .../build/x0x-smk37-elite.fwsc  and  .../build/smk37-elite/x0x.elf
  Pro:      .../build/x0x-smk37.fwsc        and  .../build/smk37/x0x.elf
  SDK:      /workspace/emu360-test/sdk/fw-AC79_AIoT_SDK-AC79NN_SDK_V1.2.14_2026-10-13
  sysroot:  /workspace/emu360-test/deps/sysroot   (pkgconf + X11/ALSA headers)
  env:      source /workspace/emu360-test/env.sh
  gate:     /workspace/emu360-test/bin/gate.sh

Rebuild fm1-live:
  source /workspace/emu360-test/env.sh
  cd /workspace/emu360-test/X0X37-for-Loopy-Pro-8c8fa82b789d7dabde04d2fa2334b5616becda47
  cargo build --release -p fm1-live

Rebuild 25.1 firmware:
  source /workspace/emu360-test/env.sh
  export AC79_SDK=/workspace/emu360-test/sdk/fw-AC79_AIoT_SDK-AC79NN_SDK_V1.2.14_2026-10-13
  cd /workspace/emu360-test/X0X37-for-Loopy-Pro-78f81bad07b8e53161326c056a2111c663349772
  X0X_DEV_PADCAL=0 MODEL=elite ./build.sh --board smk37
  X0X_DEV_PADCAL=0 MODEL=pro   ./build.sh --board smk37

One strict 8s shot (official-style). Cwd must have work/ (for panel.ppm):
  source /workspace/emu360-test/env.sh
  cd /workspace/emu360-test/X0X37-for-Loopy-Pro-8c8fa82b789d7dabde04d2fa2334b5616becda47
  /workspace/emu360-test/bin/gate.sh \
    --fwsc <package.fwsc> --elf <app.elf> --shot 8000 \
    --out /tmp/emu360-out

  gate.sh sets FM1_STRICT=1, --mhz 360, FM1_CORE_LOAD from ELF _c1_start/_c1_end,
  FM1_CORE_LOAD_CSV, FM1_DUMPRAM, and scores x0x_dbg.max_us / x0x_dbg.late
  (PASS = max_us < 11609 and late == 0).

  For repeat runs, add --flash FILE (format once with a short --shot first).
  gate.sh does not currently pass --flash; use the env command in README if needed:

  env FM1_STRICT=1 FM1_CORE_LOAD=<lo>-<hi> FM1_CORE_LOAD_CSV=out/core_load.csv \
      FM1_DUMPRAM=out/ram.bin \
      ./target/release/fm1-live --fwsc PKG --mhz 360 --shot 8000 --flash FILE

Default --shot input (built into fm1-live): C4-E4-G4 held from 800 ms, PLAY 1200-1350 ms.

## Heavy-load gate

Written (official scene not found): bin/heavy.sh elite|pro [fwsc]. See scenes/README.md. PASS = max_us < 11609 and late_during_test == 0 (startup late=1 subtracted via a 2000 ms boot shot). 25.1 Elite 11566 / Pro 11571, 0 late during the test.
