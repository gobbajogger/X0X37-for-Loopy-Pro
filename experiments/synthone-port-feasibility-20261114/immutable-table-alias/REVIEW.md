# Independent review scope

The independent source reviewer accepted `FILES.json` SHA-256
`b48834c5c3dfc73ff4b838750fcb9f46e17cb54815bcd098f6630dad2ec4e34c`
after verifying all ten listed files before and after review. No actionable
findings remained. This is a concise record derived from the returned review
message; no separate original review-report artifact was produced.

Review exposed a compiler-input binding defect: quoted or ambient shadow headers
could replace intended dependencies while both compared graphs still agreed.
The repaired runner stages only authenticated inputs in a fresh compiler tree,
rechecks that closure, and removes ambient include-path variables.

The reviewer independently ran the seven binding checks, including the original
RED controls. Repaired O2 and ASan/UBSan runs used the shadow tree with all four
include variables poisoned. Each passed 5,322 bitwise PCM frames, 5,391 state
snapshots and seven protected-write probes. Staged trees excluded the shadow
header; both voice objects referenced the genuine `sp_noise_compute`.

The unchanged 29-case packer proof was reused. Independent result SHA-256 values
were `c44d2a0f5bd3cc64f53996876bb455be9bf7d95258b46c8227c7122014cdd046`
(O2) and `f914647d9b0d7d0001a6ef40bfdca770f9ab9a42663216dccb2d807c6ee99454`
(sanitizer). These identify result artifacts, not a review report.

Acceptance covers the exact immutable alias boundary with the pinned shared
graph. It does not establish target storage layout, native resources, timing,
full-engine fidelity, device behavior or release acceptance. Generated tables,
audio and executables are excluded from this source delivery.
