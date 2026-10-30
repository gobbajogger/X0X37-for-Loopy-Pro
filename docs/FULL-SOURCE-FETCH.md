# Full-source fetch over the connected GitHub API

**Current update (2026-10-30):** A newer exact source inventory and SHA-based differential recovery tool are now in [FULL-SOURCE-FETCH-DELTA.md](FULL-SOURCE-FETCH-DELTA.md). The pinned `wip/one-build` checkpoint `b982c1eebc2ef731e0379103506aecb3df0b85e5` has root tree `44c17cf99617adce274c0a57156cf7a9c4176c7b` and a [4,239-blob manifest](manifests/one-build-b982c1ee.tsv). `tools/source_tree_delta.py` and `tests/test_source_tree_delta.py` have **8/8 measured local focused tests passing**. Three live connector reads of symlink, shell script, and binary matched SHA/size metadata. The pinned V1.2.1 **subset** checked 11/11. None of this is a complete source checkout, host-suite PASS or Elite/Pro build.

**The remainder of this document is a historical, independently pinned `cb5423c4` recovery report**, kept for its tested GitHub connector calls and explicit error messages. Do not follow its older branch SHA as though it were today's integration HEAD. To recover a newer commit, generate and verify its root-tree manifest, transfer every missing blob (or use a verified older checkout as a SHA-keyed cache), and run the applicable checks against that exact tree.

Status: **PARTIAL / NOT A COMPLETE CHECKOUT** (2026-10-30). This records actual connector calls, a reproducible byte-transfer protocol, a locally checked reconstruction tool, and the still-unmet source/test requirements. It must not be cited as a full-suite PASS.

## Frozen source and exact inventory

- Repository: gobbajogger/X0X37-for-Loopy-Pro
- Source branch at preflight: wip/one-build
- Source commit: cb5423c4aad7d9fc0c3e0a532c2aac080f3cfd21
- Git commit root tree: 3e2a673f97a0ae4fc4e3fd5457952fb6e3f7d6e8
- Recursive tree endpoint: https://api.github.com/repos/gobbajogger/X0X37-for-Loopy-Pro/git/trees/3e2a673f97a0ae4fc4e3fd5457952fb6e3f7d6e8?recursive=1
- GET result: sha equals root tree; truncated=false; 4,582 entries = 4,223 blobs + 359 directories; declared blob data 23,199,539 bytes.
- Blob modes: 4,069 regular 100644, 145 executable 100755, and 9 symlink 120000. There are no Git submodules in this tree.
- The API also accepted /git/trees/cb5423c4?recursive=1, but its result's sha was the commit SHA, not the root-tree SHA. Use the exact root tree URL above for independent identity checks.

The branch and commit were checked through the connected GitHub tool. Do not substitute a historical bootstrap or any other head. If the integration branch advances, this receipt remains about the pinned commit, not its successor.

## Result actually obtained

- Complete recursive metadata listing: **1/1 PASS**, 4,582 entries, not truncated.
- Corrected connector byte-fetch path: **43/43 PASS** for contiguous blob indices 0 through 42, comparing the returned Git blob identity and decoded byte count with the tree. Symlinks use fetch_blob by SHA. Additional independent binary and source samples were read successfully.
- Independently computed Git SHA-1 over decoded bytes for these three samples: **3/3 PASS**:
  - docs/img/new/1.0-303-steps.png = 2e8a3bbf3e2c2638784fa912db10fcc7c6af065b
  - assets/factory-breaks/factory-break-pack.bin = b0d465232e0ac6056900d8848b4bd26b65aabae9
  - tests/run_tests.sh = c47dcbe4c3842fff58523bc2feaf3014468b01e9
- Batched Code Mode reads: 18 entries + one tree request worked. Attempts with 26 or 35 file reads inside one invocation failed at its maximum-tool-call limit. Use at most 12 file reads per invocation to leave capacity.
- The exact full-source checkout was **NOT reconstructed**. All 4,223 blobs have not been transferred into the execution filesystem. The connector's byte-return memory and the local shell do not expose a common writable file transport here.
- Full host tests at cb5423c4: **NOT_RUN**, zero test cases executed, no legitimate full-suite PASS/FAIL count. No SDK package or device action was attempted.
- The offline pinned SDK V1.2.1 input kit was verified separately: 11/11 files PASS (details below). This is a pinned *subset*, not a complete SDK.

## Exact GitHub connector calls

Use the authenticated Code Mode tool interface, not a shell request, git clone/fetch/pull, or direct GitHub DNS.

1. Read the branch: tools.mcp__GitHub__fetch({url:"https://api.github.com/repos/gobbajogger/X0X37-for-Loopy-Pro/branches/wip/one-build"}). Confirm its commit is cb5423c4aad7d9fc0c3e0a532c2aac080f3cfd21.
2. Read the recursive tree: tools.mcp__GitHub__fetch({url:"https://api.github.com/repos/gobbajogger/X0X37-for-Loopy-Pro/git/trees/3e2a673f97a0ae4fc4e3fd5457952fb6e3f7d6e8?recursive=1"}). Parse result.content as JSON; require the recorded SHA, truncated=false, the exact entry counts and modes.
3. For an ordinary 100644 or 100755 blob (including binary and empty files), call tools.mcp__GitHub__fetch_file({repository_full_name:"gobbajogger/X0X37-for-Loopy-Pro",path:entry.path,ref:"cb5423c4aad7d9fc0c3e0a532c2aac080f3cfd21",encoding:"base64"}). Its result.content contains line-wrapped base64; remove whitespace. Compare result.sha with entry.sha and base64-decoded length with entry.size.
4. For a 120000 symlink, call tools.mcp__GitHub__fetch_blob({repository_full_name:"gobbajogger/X0X37-for-Loopy-Pro",blob_sha:entry.sha}). This returns the UTF-8 link-target text. Base64-encode those bytes for the same transfer format. The receiver independently checks the Git blob SHA.
5. Fetch in bounded batches, persist every returned byte chunk in order, and make the receiver reject missing/duplicate chunks. Do not silently skip API errors or save a partially reconstructed tree as the target revision.

Binary and empty files are included; neither Git SHA identity nor file count can be inferred from a pathname search. Keep executable and symlink modes.

## Copyable Code Mode producer (connector-only; no filesystem access)

This is the tested bounded connector pattern. Run the whole JavaScript body in functions.exec. For the first invocation, use START_INDEX=0 and START_OFFSET=0. Copy the returned single JSON object as one line into source-transfer.jsonl on the native execution machine. For each later invocation, set START_INDEX and START_OFFSET to the previous JSON object's next array, and append its entire JSON object as another single line. Continue until next is [4223,0]. Only then run the receiver.

Each invocation reads the pinned tree again, makes at most 12 blob calls, and emits at most 6,000 base64 characters plus JSON metadata. This avoids the observed tool-call ceiling and response truncation. The connected tool has no direct file-write bridge to the shell: **copying the batches is a required transfer step**, not something this document claims has already been automated.

~~~javascript
const REPO = "gobbajogger/X0X37-for-Loopy-Pro";
const COMMIT = "cb5423c4aad7d9fc0c3e0a532c2aac080f3cfd21";
const TREE = "3e2a673f97a0ae4fc4e3fd5457952fb6e3f7d6e8";
const START_INDEX = 0;
const START_OFFSET = 0;
const MAX_FILE_CALLS = 12;
const MAX_B64_CHARS = 6000;

function linkBase64(text) {
  const abc = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  const b = unescape(encodeURIComponent(text));
  let out = "";
  for (let i = 0; i < b.length; i += 3) {
    const n = (b.charCodeAt(i) << 16) |
      ((i + 1 < b.length ? b.charCodeAt(i + 1) : 0) << 8) |
      (i + 2 < b.length ? b.charCodeAt(i + 2) : 0);
    out += abc[(n >>> 18) & 63] + abc[(n >>> 12) & 63] +
      (i + 1 < b.length ? abc[(n >>> 6) & 63] : "=") +
      (i + 2 < b.length ? abc[n & 63] : "=");
  }
  return out;
}
const url = "https://api.github.com/repos/" + REPO +
  "/git/trees/" + TREE + "?recursive=1";
const remote = await tools.mcp__GitHub__fetch({ url });
const tree = JSON.parse(remote.result.content);
const files = tree.tree.filter(x => x.type === "blob");
if (tree.sha !== TREE || tree.truncated !== false || files.length !== 4223 ||
    tree.tree.length !== 4582)
  throw Error("Wrong or incomplete Git tree");

let index = START_INDEX;
let offset = START_OFFSET;
let budget = MAX_B64_CHARS;
let calls = 0;
const chunks = [];
while (index < files.length && calls < MAX_FILE_CALLS && budget > 0) {
  const entry = files[index];
  let b64, claimedSha = entry.sha;
  if (entry.mode === "120000") {
    const result = await tools.mcp__GitHub__fetch_blob({
      repository_full_name: REPO, blob_sha: entry.sha
    });
    b64 = linkBase64(result.result.content);
  } else {
    const result = await tools.mcp__GitHub__fetch_file({
      repository_full_name: REPO, path: entry.path,
      ref: COMMIT, encoding: "base64"
    });
    b64 = result.result.content.replace(/\s/g, "");
    claimedSha = result.result.sha;
  }
  const byteLength = b64.length * 3 / 4 -
    (b64.endsWith("==") ? 2 : b64.endsWith("=") ? 1 : 0);
  if (claimedSha !== entry.sha || byteLength !== entry.size)
    throw Error("Blob SHA or length mismatch at " + index + " " + entry.path);
  let take = Math.min(b64.length - offset, budget);
  take -= take % 4;
  if (take === 0 && offset < b64.length) break;
  chunks.push({
    index, path: entry.path, mode: entry.mode,
    sha: entry.sha, size: entry.size,
    start: offset, total: b64.length,
    b64: b64.slice(offset, offset + take)
  });
  budget -= take;
  calls += 1;
  if (offset + take === b64.length) {
    index += 1;
    offset = 0;
  } else {
    offset += take;
  }
}
text(JSON.stringify({
  tree_sha: TREE, truncated: false, total: files.length,
  cursor: [START_INDEX, START_OFFSET], next: [index, offset], chunks
}));
~~~

If a blob fails, keep the prior cursor and the exact failure text; retry that same cursor without appending a partial batch. The next cursor must strictly advance; empty files are represented by a single chunk with an empty b64 field. The emitter must not be called with a start offset outside the reported total.

The tiny Code Mode output limit makes this manual bridge suitable for evidence and a reproducible fallback, but inefficient for a 23 MB source tree. A genuine shared binary file transport would remove that overhead; none was proved in this run. Do not present an unperformed batch loop as a full fetch.

## Exact reconstruction on the native machine, with no Git or DNS

The companion tools/restore_github_api_batches.py reconstructs the entire source only after it has all 4,223 blobs. It verifies each Git SHA-1 of the uncompressed file bytes (the Git blob header is "blob " + decimal length + a NUL), sizes, ordered batch cursors, unique paths, executable bits, relative symlinks, total bytes and recomputed root Git tree SHA. It writes into a staging directory and atomically promotes it only after every check passes. It makes **no network calls** and never writes firmware.

After all JSON batches have been appended as single lines to source-transfer.jsonl, from the repo root containing the companion script run:

~~~bash
python3 tools/restore_github_api_batches.py \
  --input source-transfer.jsonl \
  --output x0x37-cb5423c4
~~~

Success must contain both of these exact facts:

~~~text
4223 verified blobs, 23199539 bytes
tree 3e2a673f97a0ae4fc4e3fd5457952fb6e3f7d6e8
~~~

Any missing chunk, incorrect SHA, unsafe path/symlink, mode collision, or root mismatch must fail. Do not run tests against an incomplete staging directory or a different historical archive.

The receiver was locally checked using a synthetic 3-blob Git tree (regular file, executable file, symlink): **3/3 PASS**, 34 bytes, expected synthetic tree SHA 075707e739861258c41c6204627759016e9217b4; executable mode 0755 and symlink preserved. An intentionally wrong 35-byte expected total failed with "FAIL: total byte count mismatch: 34". This is a receiver unit check, **not** a full live source checkout.

## Pinned SDK V1.2.1 input check

The owner's Project Library item x0x37-private-build-inputs-20261021.zip was placed outside the repository at /mnt/data/loopy-inputs/x0x37-private-build-inputs-20261021.zip. The SHA-256 and ZIP integrity were independently checked; the package contains 11 pinned AC79NN V1.2.1 input files. After safe extraction:

~~~bash
cd /mnt/data/loopy-inputs/sdk-kit/x0x37-private-build-inputs-20261021
python3 kit.py verify
~~~

Observed output (exit 0):

~~~text
Kit checksums and all 11 pinned SDK inputs verified.
~~~

Archive SHA-256: 2570c8f9472968afb16ea94372a05edda2e1f9c72d22b2431688c87d906c4805.

This kit is not a complete vendor SDK V1.2.1 tree. Do not relabel the separately uploaded AC79NN V1.2.0 archive as V1.2.1. Do not push or embed third-party SDK files, toolchains, credentials or sample backups in this PR.

## Full host-suite command and honest result

Only **after** the above receiver passes at the exact tree SHA, run the actual test runner on that reconstructed root (not on the bootstrap or a modified working tree). Provide the pinned references and full V1.2.1 dependencies if individual tests need them. Do not count optional dependency skips as PASS.

~~~bash
cd x0x37-cb5423c4
X0X_TEST_JOBS=8 sh tests/run_tests.sh > full-host-suite.log 2>&1
result=$?
grep -Ec '^  ok[[:space:]]+' full-host-suite.log || true
grep -Ec '^  FAIL[[:space:]]+' full-host-suite.log || true
grep -Ec '^  not-run[[:space:]]+' full-host-suite.log || true
printf 'suite exit=%s\n' "$result"
tail -40 full-host-suite.log
~~~

The suite was **NOT_RUN** in this task because exact source bytes were not reconstructed into the native environment. Actual host test counts: PASS 0, FAIL 0, NOT_RUN *all selected tests* (not enumerated because the runner was not invoked). No output above is presented as measured for this source. Test executions, log excerpts and fixes require a future run at the verified checkout.

## Exact errors seen and recovery

1. ZIP endpoint through the connector:

~~~text
INVALID_ARGUMENT: Error code: INVALID_ARGUMENT; Error: HTTPError: 400: GitHub Fetch URL is not an allowed public GitHub repository or search endpoint. (Response: None)
~~~

2. Binary data through the UTF-8-only fetch_blob action:

~~~text
UNKNOWN: Error code: UNKNOWN; Error: UnicodeDecodeError: 'utf-8' codec can't decode byte 0x89 in position 0: invalid start byte
~~~

The raw Git data fetch action also returned:

~~~text
INVALID_ARGUMENT: Error code: INVALID_ARGUMENT; Error: HTTPError: 400: GitHub Fetch only accepts UTF-8 text. (Response: None)
~~~

Use fetch_file with encoding:"base64" for binary, including WAV, PNG, BIN and other non-UTF-8 files.

3. Trying fetch_file by a symlink path:

~~~text
UNKNOWN: Error code: UNKNOWN; Error: InvalidInputException: GitHub path did not return readable file content: gobbajogger/X0X37-for-Loopy-Pro/.agents/skills/x0x37-check
~~~

Use fetch_blob at the symlink's recorded SHA and preserve mode 120000.

4. Oversized batches (26 and 35 file reads per Code Mode invocation):

~~~text
Script error:
Code Mode exceeded the maximum number of tool calls.
~~~

The tested small-batch workaround is MAX_FILE_CALLS=12, with a bounded response payload and an explicit resume cursor.

5. Calling /git/commits/cb5423c4 through the generic fetch endpoint returned:

~~~text
NOT_FOUND: Error code: NOT_FOUND; Error: GitHub API error 404: {"message":"Not Found","documentation_url":"https://docs.github.com/rest/git/commits#get-a-commit-object","status":"404"}
~~~

Read the full commit from the branch response (or use the full commit identifier), and the root tree through its canonical SHA.

## Exit criteria and follow-up

The full-source proof requires the receiver's **4,223/4,223** blob and exact root-tree PASS, an actual tests/run_tests.sh execution, the suite's PASS/FAIL/NOT_RUN inventory and preserved logs. Those conditions are not met. This documentation PR is for review of the exact API route and current transfer limitation; it is not a firmware change, a suite PASS, or permission to merge, deploy, publish or flash.
