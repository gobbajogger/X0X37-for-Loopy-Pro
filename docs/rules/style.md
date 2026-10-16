# Rules v3: style and terms

Read when: before you write a file, a commit, a checklist or an agent message.

### Rule 15 [v2: 1, and the style scope line of the v2 header]
Step: each text (P1 to P11).
Rule owner: Docs Keeper.

Write files (docs and logs), commits and checklists with the asd-ste skill. Write agent-to-agent messages, including task messages, with the caveman skill. If a text is in the scope of asd-ste, asd-ste wins. For messages to the owner, see owner-messages.md (rule 13).

## Terms (these are not rules)

- owner: the person who owns the project. Do not write "user" for this person.
- Orchestrator: the agent that routes the tasks and relays the messages to the owner. Do not write "parent agent".
- task agent: the agent that owns a task in the lanes table (AGENTS_AND_ROOMS.md).
- rule owner: the agent role that keeps a rule current. The rule owner is not the owner of the project.
- loop slots: the flash slots that hold the imported loops (slots 1 to 3).
- gate: one run of the gate script. Each gate writes one row in /workspace/x0x37-results/index.md.
- Bluetooth (BLE): Bluetooth Low Energy MIDI. In text, write "Bluetooth". Use "BLE" only in names, for example ble-a74, BLE-16 and BLE_ATTEMPTS.md.
- behavior: use this US spelling in all files.
- pairing: the Bluetooth security step between two devices. It includes the bonds. Do not write "bonding".
- colour: British spelling. Use "color" only in code names. Keep "colour" in text.
- owner quotes: Owner quotes stay word for word inside quotation marks. Quotes are exempt from STE.

## Rule owners

- Merge Integrator: rules 1, 10 and 11.
- Orchestrator: rules 2, 3, 4, 4B, 13 and 14.
- Firmware Main Builder: rules 5 to 9.
- Docs Keeper: rules 12 and 15.
- Build and Test Runner: the gate run section of integration.md.

## Pipeline steps (the "Step" values)

- P1. Record the bug (INLOG).
- P2. Find the cause with data.
- P3. Make the host test.
- P4. Make the lane commit.
- P5. Merge onto integration.
- P6. Run one gate for each batch.
- P7. Write the owner test checklist.
- P7a. Wait for the full set: all queued changes are complete and have one gate PASS together (owner 2026-10-17 01:57).
- P8. Send the owner notice.
- P9. Install.
- P10. The owner tests.
- P11. Log the result.

## Skills and versions

The project skills are asd-ste (/home/box/agent-data/workflows/asd-ste/SKILL.md), caveman (/home/box/agent-data/workflows/caveman/SKILL.md) and x0x37-builder-loop (/home/box/agent-data/workflows/x0x37-builder-loop/SKILL.md). USER_PREFERENCES.md holds the owner decisions.

Version 3 (2026-10-17 00:00, owner approved) has the rule files in this folder. Each rule gives its version 2 number in brackets. Version 2 (15 rules) is docs/agent-rules.md at smk37 09ff57e. Version 1 (41 rules) is agent-rules-v1-2026-10-16.md on the Mac. It is not in git.
