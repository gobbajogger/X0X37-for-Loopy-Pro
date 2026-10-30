# Independent frozen storage follow-up

**PASS for bounded source/storage review. Prior mixed-format preservation HOLD cleared.**

Head `1d2418a26003a5aecadb4a8609520e6c507f35c6`, tree `15a071788d65513d795841f61cd2b22fa82c8428`. Compared to previously reviewed `6ddea31d6040d6735989e64adbe11630fc1ee7ec`. Source worktree clean and unmodified.

The production change retains an unsupported-format marker from a consumed continuation through decompression failure, returning -9 before the generic corruption fallback. Unused trailing objects are not scanned and remain irrelevant. Ordinary corruption remains -1 with the established fallback behavior.

Independently executed:

- New NOR storage suite with ASan+UBSan: **1314 checks, 0 failures**, exit0. This includes used continuation tags4/5/7 with staged/live refusal and unchanged protected/storage bytes; ordinary corrupt continuation tag0 still permits default fallback/save; unused stale unsupported tail objects do not block valid load/save; explicit validated reset/adoption still clears incompatibility.
- Original independent reproduction, also ASan+UBSan: valid6679-byte packed stream with2859-byte used OBJ_PAT1; changing only its header to5 now produces `load=-9 unsupported=1`, then fallback `save=-9` with **zero erases/programs/saves**. Previous exact probe on6ddea31d produced load=-1 and4saves/4erases.

Exact commands, source blobs, exits and log SHA256 are in `followup-receipt.json`. Prior broader REC tests/review remain evidence for unchanged recorder source. No native compilation, target budget, controller package, device operation, flash, publication or installability claim. Exact native qualification remains a separate gate.
