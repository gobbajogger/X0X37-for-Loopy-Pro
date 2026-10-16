# Install

Read when: before and after an install. Only Firmware Main Builder installs.

### Rule 5
Step: P8 owner notice, P9 install, P11 log result. Rule owner: Firmware Main Builder.

Copy the built files to the Mac immediately before the install. Before the install, back up the loop slots and send the owner notice. The owner notice and the report after the install have no hash, no CRC and no IDs (rule 13). Write the commit hash and the result file of the gate PASS row (rule 6) only in the ledger (BLE_ATTEMPTS.md). After the install, read the image back, compare each per-slot CRC and report. Tell the owner about slot checks only when one does not match.

Sync box commits to the Mac only with a git bundle (same hashes, no patches).

### Rule 6
Step: P6 gate, P7a full set, P9 install. Rule owner: Firmware Main Builder.

Make one install for each finished set (core rule 6A). A gate PASS on one batch does not start an install. Install only the gated hash of the last batch in the finished set. That hash needs a gate PASS for its exact tree, with 0 fail and 0 skip. Only the gate script writes the PASS row in /workspace/x0x37-results/index.md. Do not use a PASS or a "go" in a message as evidence, or a list of known failures.

Install only with fm1_install.py --commit and the guard require_gate_pass.sh. Never use the web installer for a team install.

### Rule 7
Step: P9 install. Rule owner: Firmware Main Builder.

Never write the bootloader. Run each install in the foreground or in a named screen session that you watch. Never install in the background (no nohup, no &).
