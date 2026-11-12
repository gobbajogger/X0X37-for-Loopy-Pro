# Bundled NIGHTLY packaging source evidence

Base: firmware nightly `54f2ffbc3124f7449a69067dc8215a2d91a2e675`, root
`3b5cadafba33f4c1b116660bc9d19a27a892337c`. No application/controller source or
compiler input changed. Existing accepted model bytes were reused without recompilation.
This lane owns packaging and documentation only; concurrent SMC work is excluded.

Producer synthetic Elite/Pro tests and original atomic writer regression pass. Independent
review reproduced malformed chip-key/app-area inputs, verified the repaired clear refusal
and output preservation, and accepted the final exact DEV identity, range, fixed slotmap,
CRC/hash/padding/composition boundaries. See validation.json for source blobs and log hashes.
The final identity delta reused unchanged broad evidence.

The byte receipts identify two owner-local compositions of the existing accepted public
packages with the exact controller. Both were reopened, checked by Python and the browser
parser/download verifier, and the original blank slots were independently refused. These
are not new native builds, independent full firmware QA, hardware PASS, rights clearance,
public hosting, catalog qualification or publication. No binary is in this commit.

Licensing findings: [dated component evidence](../controller-licensing-evidence-20261025.md)
and [SDK pins](../controller-sdk-pins-20261025.json). No verified vendor prohibition was
established, but runtime/source/notice mapping is incomplete; public and private conveyance
need applicable conditions. Next: existing independent rights/release owner completes
component mapping/notices, then existing firmware integrator/QA and website publisher own
qualification and immutable-host/catalog adoption. This worker does not merge or publish.
