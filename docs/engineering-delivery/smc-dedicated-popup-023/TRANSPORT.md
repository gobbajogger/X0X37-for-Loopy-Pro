# Transport feedback

RW/FF 0x5B/ 0x5C cue patterns and show NEXT [HOME pattern glyph] > number through
the existing header toast for 1000 ms. No popup activation or cancellation of an
existing popup. A cancelled cue shows --. PLAY and MMC Play/Stop remain
transport-only. No keyboard view/part/mixer selection/page writes.

Owner approved without preview: no visual approval wait. [Actual framebuffer
before/after and reproduction](pattern-notice-proposal/README.md).

Fresh 47 SMC/mixer selectors pass, including 4958 popup checks and all nine prior
failures. Both Elite/Pro SDK V1.2.1, X0X_DEV_PADCAL=0 builds pass with 2432 B free
static RAM each. smc_connected additionally uses a SMC_PEERS-bounded loop with
one-/two-peer central-role/up-mask regressions passing UBSan/bounds at O0/O2.

Full repository host suite and physical hardware tests remain NOT_RUN; no flashing
or binaries published. Exact source/test/build inputs: verification.json.
