# 909 tanh tip recut

The selected tip patch is 32802577dbf148adf472113316c6e636f880e08b from lane/build-25-all-opt-struct-20261106. The patch is recut onto a06e20189141ff5fb8766e2c6152be460f4356c3. Other lane changes are not imported. The first parent is the requested base; the old lane tip is retained as a second parent solely to permit a nonforced update.

The production change reads adjacent packed tanh entries with one run search. It preserves interpolation arithmetic, table bytes, engine state and PCM. Tests are pinned to the requested base, adapted to its pre-panic API and registered in the full suite. No main, nightly or .github source edit is performed. There is no device, bootloader, BTIF, USRTRIM, efuse or key_mac write. No binary or raw capture is delivered.

Both real SDK V1.2.1 Elite/Pro development builds pass. For each model, image size is 474840 -> 475000 B (+160); static app RAM is 95780 / 98304 B (delta 0; 2524 free); .ram_text is 2712 B and 844 instructions with no calls (delta 0); pool usage is 335464 / 344064 B (8600 free). No soft-double routines are linked. These are BUILD_ONLY results, without a device/install/release claim.

Each -O0 and -O2 regression compares 23280 blocks, all four PCM buses and all 7648 engine-state bytes against the exact base. The exhaustive check at each optimization compares all 1024 table pairs and all 4278190082 non-NaN binary32 inputs bit for bit. A separate ASan/UBSan check covers every paired read.

The portable host-only tanh microbenchmark and its measurements are in tanh-bench.c and tanh-benchmark.json. It does not measure full-engine performance or SMK37 cycles. The approximately 7256 cycles/sample budget at 44.1 kHz / 320 MHz remains unmeasured on device.

Full host suite: **333 PASS, 0 FAIL, 0 SKIP, 0 NOT_RUN**, exit code 0. The pristine run record names the actual checked local commit cc6566db154c57ff13f1cb8c03fbedeac61b1c20 and source tree bab80c4ba34093d106a8fcbfe509616c82a0fdfa. The final source commit is 22fe54c030ccbb10d1b25802f06a736d64130777, tree bbdeeb2f750d05c18386cdd041431705b1d8dc86. The sole post-run source difference removes one extra EOF blank line in the C fixture; fixture-format-equivalence.json proves identical preprocessor output and identical -O0/-O2 object bytes. All other checked inputs and native artifacts remain unchanged.
