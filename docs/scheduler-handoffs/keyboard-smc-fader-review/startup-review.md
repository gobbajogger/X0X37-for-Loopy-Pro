# Independent cold-start pickup review

Result: PASS, source and host behavior only. Source base installed `e75fb2a584aa22978f352697a5e36fc9f886c3c5`. Reviewed runtime file `firmware/src/app/main_fm1.c` Git blob `aeb7861c24f6383784b2fc6350a06e768a8a33fb`, SHA256 `7c3748bc563ecf7b21eac213c2c07721bf2128f55cf10a073ab9cfb64329767b`.

Defect independently reproduced with the actual production UI/engine fixture: first keyboard fader scan at frame206, physical decoded100, saved level64. Installed source publishes one engine write, changes saved64→100 and actual mixer gain0.4096→1.0 despite no physical crossing. Fixed source seeds the pending physical history and returns: zero writes, saved64, gain0.4096. Installed negative exits failure at the no-write assertion. Logs: `keyboard/cold-baseline.log`, `keyboard/cold-fixed.log`.

Startup path review confirms timer5 starts before 30ms plus300ms delays and the first smk_frame. Scanner increments frames each16 ticks at10kHz, so the <200-frame settling path cannot be relied on to seed the first application fader scan. Additional setup may make that first call later. This establishes a reachable software defect, not the explanation for every intermittent later observation.

The fix reuses existing held-byte bit1 as the one-time unknown-history marker (`held=2`). First call or project busy seeds all four pending samples and consumes the current bank-button state. Subsequent held values are Boolean0/1, so the marker cannot accidentally repeat. The existing target check, two-count pickup window, crossing rule, per-bank mapping, engine write and motion-touch code remain unchanged. No extra variable/static storage was introduced. Changing an existing zero initializer to2 can affect target image sections; native size must be measured separately.

Independent verification:

- Copied real UI/engine fixture to this review directory to avoid concurrent scratch writes. `sh keyboard/run.sh`: PASS96 mapping/direction/navigation/lock sequences,14908 checks, including two real motion-lane holds and exact acquisitions. Engine write recorder forwards to real engine. Stored value, formatted display message, motion effective value and actual mixer gain are separately asserted/traced. This reuses another worker's fixture but executes it independently.
- Independently authored `python3 keyboard/entry_test.py`: PASS48 process-fresh combinations: initial frames0/199/200/206 × initial FADER BANK released/held × project busy/no-busy × physical20/64/100 with saved64. Every case checks first-call zero writes, no parked replay or initial held-bank toggle, fresh pending/acquisition movement, and a subsequent release/new press that switches bank without an unrelated parameter write. Clang ASan/UBSan passed. Fixture inherited two nonfatal macOS sprintf-deprecation warnings; no runtime sanitizer failures.
- Negative control compiled the same production fixture with the installed smk_faders extracted function. Regular matrix passed while cold-entry assertion failed, showing the new test discriminates the actual missing-initial-history behavior rather than duplicating old checks.
- `git diff --check`: PASS.

No hardware, GUI, source merge, publication or binary upload. Target native build/size and both-model package qualification are outside this review.
