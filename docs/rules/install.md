# Rules v3: install

Read when: before an install and after an install. Only Firmware Main Builder reads this file.

Terms: The loop slots are the flash slots 1 to 3 that hold the imported loops. A gate is one run of the gate script. Each gate writes one row in /workspace/x0x37-results/index.md.

### Rule 5 [v2: 8, and the notice part of v2: 7]
Step: P8 owner notice, P9 install, P11 log result.
Rule owner: Firmware Main Builder.

Only Firmware Main Builder installs. Before the install, back up the loop slots and send the owner notice. The notice gives the exact hash and quotes the result file of the gate PASS row (rule 6). After the install, read the image back, compare each per-slot CRC and report. Tell the owner about slot checks only when one does not match.

### Rule 6 [v2: 7]
Step: P6 gate, P9 install.
Rule owner: Firmware Main Builder.

Install only the batch hash, after a gate PASS for its exact tree with 0 fail and 0 skip. Only the gate script writes the PASS row in /workspace/x0x37-results/index.md. Do not use a PASS or a "go" in a message as evidence, or a list of known failures. Install only with fm1_install.py --commit and the guard require_gate_pass.sh, and never use the web installer for a team install. This applies from batch 3 (the batch with the install guard). Until then, Firmware Main Builder reads the index row and the result file himself before the install.

### Rule 7 [v2: 9]
Step: P9 install.
Rule owner: Firmware Main Builder.

Never write the bootloader. Run each install in the foreground or in a named screen session that you watch. Never install in the background.
