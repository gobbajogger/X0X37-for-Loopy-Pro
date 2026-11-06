# Arrow-event assertion derivation

No test rerun. The modified expected total is derived from script durations and production input timing, not solely observed output.

110 - 33 - 8 + 1 = 70. The 3000ms left hold formerly repeats at 6257..8817ms in 80ms increments (33 repeats); new section exit consumes the held press. The 1000ms RESET-row left hold formerly repeats at 13665..14225ms (8 repeats), now consumed. The former 8850..8852ms release contains no 16ms UI frame, so its immediately following left tap had no observed fresh edge; new SEL/ALGO re-entry allows an observed release, adding one retained tap.

Tail: 70 + 1 right tap + 2 events from the 450ms right hold + 1 + 1 left-back presses = 75. Clip without a menu and HOME-held routing still add zero.

Source: firmware/src/app/smk_arrows.h:18-22 defines 400/80ms; :44-64 detects edges/repeats; :70-74 clears repetition. firmware/src/app/ui.c:3658-3663 consumes section navigation. host/x0x_host.c:333-337 counts nonzero events/rearms; :559-562 ticks at16ms; :957-965 gives up/down2ms and tap40+40ms.

Independent review remains pending. All original functional assertions remain; these are the navigation-count changes required by the intended back/entry contract.
