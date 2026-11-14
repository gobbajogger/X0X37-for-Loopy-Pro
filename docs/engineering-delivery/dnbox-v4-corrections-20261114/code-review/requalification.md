# Navigation fix requalification

PASS. Findings #1, #2 and #3 are resolved. No new actionable navigation regression was found.

Consumed `site/MANIFEST.sha256` SHA-256 `f93aa4c97fc5989cf4f7aae941814a1f3794b2529c06a0ddcab627d59c862c17`. Verified all 68 listed entries and the 69-file site count, including the manifest. Consumed and served `navigation.js` SHA-256 `d29344dd2f46a80e2ef0edfe30fb770e4ad7493bde12b8ce422bbd7175807485`. Both hashes and every manifest entry still matched after the checks.

- #1: Expanded bass details stay open before position restoration. Back restored scrollY exactly to 471, 857 and 1033 at widths 1440, 390 and 320.
- #2: A direct new fragment initializes its entry and focuses H1. Forward restores that heading.
- #3: Focus-only changes to the recovery selector survive Back/Forward without a scroll. No-ID buttons, summaries and repeated-href links also retain their intended focus.

Independent Chromium proof: 36/36 checks passed at 1440x900, 390x900 and 320x900, with zero JavaScript errors. Additional checks cover initial native Tab, repeated route entries, full-document Back and queued fragment events. The script requests reduced motion for deterministic position comparisons. `node --check navigation.js` passed. The parent test counts were not counted as this review's proof.

Manual diff scan covered locator encoding/lookup, cache resets, disclosure snapshot/restore order, null-state routes, generation guards, page lifecycle and full-document history. No new actionable defect was found.

Evidence: [browser observations](requalification-browser.json), [independent script](requalify_navigation.py), [reviewed navigation diff](navigation-requalification.diff), [machine receipt](requalification.json). The original review remains historical evidence of the previous candidate.

No source edits, Git writes, public writes, new review jobs, device actions or certificate changes. Journey and recording-label requalification remains with the full-product reviewer. Other browser engines and hardware/acoustic behavior were not qualified.
