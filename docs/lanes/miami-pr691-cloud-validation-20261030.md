# Miami PR 691 cloud test evidence (2026-10-30)

Owner-approved routine public test reporting under DnBOX rules 1k/1l.
Base: wip/one-build cb5423c4aad7d9fc0c3e0a532c2aac080f3cfd21.
Starting PR head: 956a533374883b015ce5c3db14e5c82ff01a4c33.

CURRENT-HEAD VERIFIED SOURCE BLOBS, isolated host checks:
GCC and Clang -Os/-O2 plus AddressSanitizer/UndefinedBehaviorSanitizer PASS.
Table interpolation: PASS 408,412 exact reference comparisons each run.
Idle clock: PASS 1,012,366 scalar comparisons each run.
Zero buffers: PASS 9,252 byte-exact guarded spans, three types.
Power-of-two modulation: PASS 6,000,000 random and 24 directed cases.
PCM comparator: PASS 8 unit tests, including negative refusals.

HISTORICAL SOURCE ONLY, commit 6ae0928, not the PR head:
Full host suite: EXIT 1; 229 checks pass, 84 fail, one skip.
Elite and Pro SDK V1.2.1 DEV builds: EXIT 0, both; historical only.
Historical suite raw log SHA256: e64fab63f2f5b5d48306880aaee721fe54e14c09e3a40c583bbe3b4a67971852.
Historical Elite build log SHA256: 721ddfc5ba0f518a65850403a84da96ae8c8510f2104db9602b1cfcbd0308b2c.
Historical Pro build log SHA256: 63312820fea0c93952316f7bd286758347577a9630f26e8d5a9cf5b0a9fa46b8.

GitHub API returned a complete untruncated metadata tree for the PR commit.
Reconstruction from the historical source still requires 253 different source blobs.
The connector's ZIP archive endpoint rejected the download request.
No complete current-HEAD local checkout or current-HEAD native build was obtained.
Full current host suite, genuine golden PCM/state comparisons (15 golden renders),
and active/idle CPU metrics are NOT RUN. The original golden set was unavailable.
No prior-source failures are attributed to the current PR.

Next: recover exact missing source blobs, verify the current tree, then run the full
host suite, Elite and Pro builds, 30-factory sound trace and 15 golden comparisons.
Keep PR 691 draft and unmerged. Held PR 712 untouched. No artifacts or private
inputs are being published.
