# Owned-link security callback

The dedicated central DEV profile retains the pinned vendor encryption/SM engine:
controller LE feature mask 1, SM enabled, stack modules 4. Encryption start,
encryption-change/refresh and key-distribution paths are linked. No input/output,
authentication 0, key sizes 7–16 and no automatic master pairing request remain
unchanged. Persistent bond storage is not implemented; this is not a promise of
bond persistence across reboot.

The exact SDK emits a synchronous 16-byte JustWorks request (event length 14).
Its public central example registers a callback and confirms that request. The
engine itself leaves user response pending; the Hortator application omitted
this callback. Its successful notification test therefore did not prove pairing.
Our callback confirms only the currently requested SMC peer/type/handle, while
radio is wanted and the link is neither connecting nor retiring. Handle zero is
valid. Connection completion must also match the requested peer and central
role. No unsolicited pairing or unrelated-link confirmation is added.

Every disconnect request retires ownership before calling the vendor, including
BUSY failures. Retiring links cannot become ready from late notifications, send
feedback, or receive JustWorks confirmation. Native profile regressions exercise
malformed requests, wrong peer, handle zero, radio-off and busy disconnect.

Strict resolved target link after 71be1b2: 104400 loaded bytes; primary 93604 of
94208, low RAM 26352 of 26368, high CPU RAM 6448 of 7168. Private ELF SHA-256:
7f3d21742a33d24ca326026ab7eb943fa6fbd73ddb2ec6072d92384735a7976d.
The build record and fully padded region hashes are pinned in the DEV builder.
All maps, ELF and exact SDK IR remain private, not published in source.

This establishes software capability and bounded ownership, not successful SMC
pairing, startup stack/heap high-water, or physical mixer operation. Those remain
hardware tests. Default ABI2 firmware is unchanged by these conditional sources.
