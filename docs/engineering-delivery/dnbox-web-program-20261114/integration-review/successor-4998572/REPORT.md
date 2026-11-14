# Manual-destination successor requalification

**SCOPED ACCEPT** for candidate manifest `ed518381e86d16fa2a104d156912b5da11d4c180b9f02764e45d4c89554ee00a`. The original manual owner's source correction closes the earlier broken keyboard/task/panel-link finding for the current delivered manual. Both website manual buttons now use the corrected immutable delivery; the obsolete warning has been removed. No new actionable defect was found in these changed destinations or their layout.

This is a delta requalification. It reuses the independent integrated-experience acceptance at `cb7b5b4d18fa7f1656b6ec99fc163d444599c4fe474a0bd660c2a7771a08a5da` and does not restart its unchanged whole-experience checks. Only `journeys.js` and `provenance/integration-20261114.md` differ among the 69 manifest entries; the other 67 entries are byte-identical. The current site has 70 total files, including its manifest, and every entry was independently rehashed successfully.

The revised browser manual was actually read from `learning-intake/manual-successor-4998572/web-MANUAL.md`: 13,276 bytes, SHA-256 `51b2906afe94aa51ebf5cc1d94924d17bfcb214c24afa9fe5726f8035e17b89b`, Git blob `a7f36a186659304340bbdadb2338cd8b3c94702a`. A separate normal-trust HTTPS fetch of the raw source at commit `4998572bcc06676f08a7875ca49fb2a56358eff6` matched those exact bytes. Commit metadata confirms parent `a19da721c25ab2323bb818851ab4d92e8ca3aa15` and changes only `web/MANUAL.md` and its evidence document. The keyboard manual at the successor commit is byte-identical to the previously consumed keyboard source, Git blob `37e276be44337444d93fb6a26d1657061c7272bd`.

| Changed path | Actual result |
| --- | --- |
| Website keyboard and browser/file manual buttons | Both point to `4998572bcc06676f08a7875ca49fb2a56358eff6`; both exact GitHub destinations return HTTP 200 under normal TLS verification. |
| Browser manual keyboard links, lines 3 and 70 | Both use the immutable `a19da721…/docs/MANUAL.md`, which returns HTTP 200. |
| Browser manual task link, line 11 | Uses `#3-quick-start-real-musical-tasks`; the matching rendered GitHub anchor is present. |
| Browser manual panel link, line 11 | Uses `#2-the-panel`; the matching rendered GitHub anchor is present. |
| Legacy topology | Zero `legacy.html` occurrences in the consumed successor browser manual. The previous immutable manual remains historical evidence, not the current button destination. |
| Versions layout at 1440, 390 and 320 × 900 | Both correctly labelled manual buttons remain readable and keyboard-focusable. No horizontal overflow or page errors. The obsolete warning is absent. Desktop and phone screenshots were visually inspected. |
| Initialization and served source | Fresh page focus stays on BODY and first Tab reaches Skip to content at all three widths. Served `journeys.js` matches candidate SHA-256 `6a0d20b38a3f049f4320c16450c5ba11cdd0ca298f22027e2f2bba687a867299`. |

Browser checks used Chromium 154.0.8037.57 on the existing local server at `http://127.0.0.1:4323/`. Destination and fragment checks used ordinary trusted Python HTTPS and actual GitHub-rendered anchor IDs. No certificate setting changed and no TLS bypass was used. The previous Chromium external-certificate limitation remains; this follow-up does not claim visual acceptance of external applications or physical navigation on a device.

The historical [REPORT](../REPORT.md) and [EVIDENCE manifest](../EVIDENCE.sha256) remain byte-unchanged at SHA-256 `76179d206a5e7d6e956b63f9b02a505bcbf0323687527f8d046375864558fc0f` and `66621d9a2dbf81a101cb669578e54f2e7c3fd5cede0a3c4c02fc59734db7c88d`. The old report's manual-source-pending disposition is superseded only by this verified successor source fix; its broader acceptance limits remain applicable.

This result does not qualify firmware boot, hardware MIDI, component implementations, current tool/firmware format compatibility or publication. No component payload, device operation, source mutation, export, deployment or Git-ref action was performed. The existing writer, component and sole-publisher boundaries remain intact. No new worker was created.

Standard service and Sol 6.1/ultra reasoning were requested. This turn exposes no tier or model-routing control, so effective backend model/effort/tier is not asserted.

Evidence: [source identity, normal-trust HTTP and exact anchors](source-and-links.json), [all-entry intake and browser results](browser-delta.json), [desktop render](versions-1440.png), [phone render](versions-390.png), [small render](versions-320.png). The two accompanying scripts reproduce the bounded read-only checks.
