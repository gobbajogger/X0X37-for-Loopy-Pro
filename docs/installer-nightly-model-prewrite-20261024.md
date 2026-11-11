# X0X37 NIGHTLY installer physical-model prewrite guard

Evidence revision: installer-nightly-physical-model-prewrite-20261024-r1 (2026-10-24).
Consumed own PR #102 at e8496c1b9beaff018738ce10cfdb5e2433281284, firmware repository ROOT 26027c2f6a297273dbe4ac3e686ad52cccc0ac0c, protected nightly 58b5b45a2dd90581de7a674c331ca5cbe8a67c7e, distinct compiled firmware SUBTREE 19840d87388fb1f5734d9765bf8a2c0b7d3ebaaf.

Finding: Updater.install and Updater.resume accept SMK-family aliases with sameKeyboard(), while published DEV Elite/Pro packages did not prompt the operator to verify the physical model against the freshly discovered device. Old installer had a confirmation only for manually imported stock/custom files. The result could be a cross-model installation despite publisher approval for one specific package/model.

Fix: On running-app installs, the exact sample-preservation helper checks the current device before reading slots or calling Updater.install; it denies missing devices, different families, and any declined physical model confirmation and closes the MIDI link on every acquired-device exit. Already-loader DEV recovery now prompts for actual Elite/Pro model before the separate sample-backup-risk consent and before Updater.resume. Added EN/JA instructions. Existing stock/custom local file, backup/restore, controller, SHA/catalog and permission paths remain.

Executed: connected exact Git helper V8 original negative -> an ignored declined model confirmation allowed one simulated writer call (EXPECTED OLD RED); edited helper synthetic V8 nine scoped positive/negative cases passed; inline HTML and full helper parsed in V8, proposed committed regression parsed. Native full Node suite, 320/390/1280 EN/JA complete browser, actual MIDI/device, immutable HTTP, and native Elite/Pro builds were NOT RUN here. The new regression is registered in the existing test_install_slots suite via its already-imported stock-model-preflight module.

Disposition: SOURCE COMPLETE / PRODUCER-CHECKED / READY FOR INDEPENDENT QA. No production, protected nightly, Pages, binaries, hosted links or device changes. Owner identity is a manual physical assertion, NOT a hardware/device PASS.

Next owner: Independent Firmware QA must run full exact-HEAD Node + Web MIDI/OTA/backup/model negative tests and Chromium, then ACK via Git. Sole firmware Safe Merge reconciles on its reviewed lane. Separate NIGHTLY website source owner must make equivalent changes on existing site PR #1; website QA, sole Main Merger and sole Pages publisher handle approval, legal immutable hosting, public HTTP and DEVICE NOT TESTED labels. Elite/Pro installability remains NO-GO; BUILD_ONLY can continue independently.
