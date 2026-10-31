# Exact source manifest and incremental GitHub API recovery

This is the follow-up to [FULL-SOURCE-FETCH.md](FULL-SOURCE-FETCH.md). It describes **measured metadata and locally tested recovery code**, not a claim that the full source or either firmware target was built.

## Verified immutable inventory

- Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
- Integration commit at the inventory checkpoint: `b982c1eebc2ef731e0379103506aecb3df0b85e5` on `wip/one-build`.
- Git **root tree** (not the commit SHA): `44c17cf99617adce274c0a57156cf7a9c4176c7b`.
- Exact manifest: [`docs/manifests/one-build-b982c1ee.tsv`](manifests/one-build-b982c1ee.tsv), with 4,239 paths and 23,407,781 declared blob bytes, including 9 symlinks and 145 executable files. Tree listing was `truncated:false`.
- The manifest is GitHub API metadata. **Its existence does not establish that 4,239 blob bodies have been transferred**, and it must not be confused with a reconstructed source tree or a passing host suite.

The manifest has `# repository`, `# commit`, `# tree`, `# blobs`, `# bytes` metadata and tab-separated `path`, `mode`, `sha`, `size`. The offline validator computes the Git root tree from every row and rejects a tree-hash, count, size or duplicate-path mismatch. Each blob is independently checked using `SHA1("blob " + len + NUL + raw_bytes)`; this is the Git object identity algorithm, not a security attestation.

## Why fetch only changed blob SHAs

When a **byte-verified local checkout** exists, most files need no download after a source or PR change. Compare Git **blob SHA values**, not just filenames or GitHub's compare-file list. A rename, permission change or duplicate new path can reuse the same blob; a modified file needs new bytes. A three-dot compare is useful for review but its paginated/capped file list is not a complete recovery inventory. Use two full untruncated tree manifests and set subtraction on blob SHAs.

The command below verifies *all* blobs in the existing checkout before counting them as reusable. It creates a small JSON manifest of unique blobs missing from the target. If no old checkout exists, omit both `--base-*` arguments; all unique target blobs must be fetched.

```bash
python3 tools/source_tree_delta.py \
  --target-manifest docs/manifests/one-build-b982c1ee.tsv \
  --needed-json /mnt/data/one-build-needed.json
```

If updating an actual previously recovered tree, first generate an equally verified TSV manifest for the new immutable commit (same seven header lines and every Git blob entry), then run:

```bash
python3 tools/source_tree_delta.py \
  --base-manifest docs/manifests/one-build-b982c1ee.tsv \
  --base-root /mnt/data/one-build-b982c1ee \
  --target-manifest /mnt/data/next-one-build.tsv \
  --needed-json /mnt/data/next-one-build-needed.json
```

Never claim the prior checkout was complete just because a manifest or old source tarball exists. The tool reads each old file/symlink and verifies its SHA and size before reuse. It checks source-tree path safety and executable modes.

## Copyable connector-only delta batch producer

Run the following complete JavaScript in the connected Code Mode GitHub tool environment. The example compares the pinned `b982c1ee` manifest with the **currently verified** `docs/full-source-fetch` PR branch head (not with a guessed SHA). Each invocation retrieves the manifest, reads the immutable head commit/root tree and downloads at most eight missing blobs. The output is one JSONL receipt. Copy its complete JSON line to the execution machine and rerun with `START_INDEX` and `START_OFFSET` set to the prior returned `next`. Do not remove line wrapping from the enclosing JSON or truncate `b64`. Use a transferred target TSV created from that same immutable head tree, with exactly the same path order. The new head **cannot contain its own manifest of itself** because that would change its Git tree. Save a head manifest separately, referring to its pre-manifest commit SHA.

```javascript
const REPO = "gobbajogger/X0X37-for-Loopy-Pro";
const BASE_MANIFEST = "docs/manifests/one-build-b982c1ee.tsv";
const START_INDEX = 0;
const START_OFFSET = 0;
const MAX_CALLS = 8;
const MAX_CHARS = 6000;
const api = "https://api.github.com/repos/" + REPO;
const [headResponse, manifestResponse] = await Promise.all([
  tools.mcp__GitHub__fetch({url:api + "/git/ref/heads/docs/full-source-fetch"}),
  tools.mcp__GitHub__fetch_file({repository_full_name:REPO,
    path:BASE_MANIFEST,ref:"docs/full-source-fetch"})
]);
const targetCommit = JSON.parse(headResponse.result.content).object.sha;
const commitResponse = await tools.mcp__GitHub__fetch({url:api + "/git/commits/" + targetCommit});
const treeSha = JSON.parse(commitResponse.result.content).tree.sha;
const treeResponse = await tools.mcp__GitHub__fetch({url:api + "/git/trees/" + treeSha + "?recursive=1"});
const tree = JSON.parse(treeResponse.result.content);
if (tree.sha !== treeSha || tree.truncated !== false)
  throw Error("Missing/truncated or wrong Git tree");
const baseShas = new Set(manifestResponse.result.content.split("\n")
  .filter(s => s && !s.startsWith("# "))
  .map(s => s.split("\t")[2]));
const entries = tree.tree.filter(e => e.type === "blob")
  .sort((a,b)=>a.path.localeCompare(b.path,"en"));
const known = new Set(baseShas);
const needed = entries.filter(e => {
  if (known.has(e.sha)) return false;
  known.add(e.sha); return true;
});
const alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
function encodeLink(textValue) {
  const bytes = unescape(encodeURIComponent(textValue));
  let result = "";
  for (let i=0; i<bytes.length; i+=3) {
    const n = (bytes.charCodeAt(i)<<16) |
      ((i+1<bytes.length?bytes.charCodeAt(i+1):0)<<8) |
      (i+2<bytes.length?bytes.charCodeAt(i+2):0);
    result += alphabet[(n>>>18)&63] + alphabet[(n>>>12)&63] +
      (i+1<bytes.length?alphabet[(n>>>6)&63]:"=") +
      (i+2<bytes.length?alphabet[n&63]:"=");
  }
  return result;
}
let index=START_INDEX, offset=START_OFFSET, budget=MAX_CHARS, calls=0;
const chunks=[];
while(index<needed.length && calls<MAX_CALLS && budget>=4) {
  const e=needed[index];
  const response = e.mode === "120000"
    ? await tools.mcp__GitHub__fetch_blob({repository_full_name:REPO,blob_sha:e.sha})
    : await tools.mcp__GitHub__fetch_file({repository_full_name:REPO,
        path:e.path,ref:targetCommit,encoding:"base64"});
  const b64=e.mode === "120000"
    ? encodeLink(response.result.content)
    : response.result.content.replace(/\s/g,"");
  const claimedSha=e.mode === "120000"?e.sha:response.result.sha;
  const length=b64.length*3/4-(b64.endsWith("==")?2:b64.endsWith("=")?1:0);
  if (claimedSha!==e.sha || length!==e.size)
    throw Error("Blob SHA or size mismatch: " + e.path);
  let take=Math.min(b64.length-offset,budget);
  take-=take%4;
  if(take===0 && offset<b64.length)break;
  chunks.push({index,path:e.path,mode:e.mode,sha:e.sha,size:e.size,
    start:offset,total:b64.length,b64:b64.slice(offset,offset+take)});
  budget-=take;calls++;
  if(offset+take===b64.length){index++;offset=0;}
  else offset+=take;
}
text(JSON.stringify({target_commit:targetCommit,tree_sha:treeSha,
  truncated:false,total:needed.length,
  cursor:[START_INDEX,START_OFFSET],next:[index,offset],chunks}));
```

**Important:** The target commit can advance between invocations. Record the printed `target_commit` and `tree_sha` on the first call. If either changes on a later call, restart at `[0,0]` using the new commit, or pin the branch to the original commit in the producer instead of rereading the mutable branch. Never append receipts from different commits to the same JSONL. The receiver also checks the `tree_sha` for each batch.

For a complete fresh recovery with no previously verified checkout, replace the `baseShas` set with `new Set()` in the producer, and collect *every* unique blob. That still needs an actual transport channel or the explicit manual receipt-copy process. No API metadata receipt is a substitute for those bytes.

## Rebuild and verify without Git or network access

Once every missing blob's JSONL receipts has arrived on the native execution machine, run:

```bash
python3 tools/source_tree_delta.py \
  --base-manifest docs/manifests/one-build-b982c1ee.tsv \
  --base-root /mnt/data/one-build-b982c1ee \
  --target-manifest /mnt/data/next-one-build.tsv \
  --receipts /mnt/data/next-one-build-receipts.jsonl \
  --output /mnt/data/next-one-build
```

The script verifies the reused checkout, exact tree manifest and all downloaded bytes before creating a staging tree; it then writes permissions and symlink targets and atomically promotes the completed tree. A corrupt blob, missing receipt, unsafe link, wrong mode, changed target tree or existing output path is an error. For an initial full fetch, omit both `--base-*` arguments and supply complete receipts.

After the recovered target tree matches, run `X0X_TEST_JOBS=8 sh tests/run_tests.sh`, then the supported `BOARD=smk37 MODEL=elite sh build.sh` and `BOARD=smk37 MODEL=pro sh build.sh` using **the actual complete AC79NN SDK V1.2.1** and matching approved compiler/reference inputs. Preserve full logs, exit codes and artifact hashes. Do not use the uploaded V1.2.0 SDK as a V1.2.1 substitute. The owner's 11 pinned V1.2.1 files are a verified subset, not proof of an entire V1.2.1 SDK.

## Proof and limits at this checkpoint

- The GitHub API untruncated base tree and manifest inventory were checked at `b982c1ee` (4,239 blob entries, 23,407,781 declared bytes).
- Separate connector reads at this same pin matched tree SHA and declared length for 3/3 sample paths: a symlink, executable `tests/run_tests.sh` and binary `factory-break-pack.bin`. These are **not** 4,239 transferred files.
- The new offline incremental/full restore regression has 8/8 passing test cases, including reject paths. It does not exercise the firmware test runner.
- The pinned V1.2.1 kit verified 11/11 inputs (SHA-256 archive `2570c8f9472968afb16ea94372a05edda2e1f9c72d22b2431688c87d906c4805`). It is not the full SDK.
- **Exact base source checkout: NOT_RUN / NOT_MATERIALIZED. Full host suite and both firmware builds: NOT_RUN.** No source or firmware artifact has been falsely reported as passed.

Do not commit any source-transfer binary snapshot, private toolchain, SDK, token, raw hardware capture or generated firmware image. Keep packed snapshots, if ever produced, in the authorized private project file store.
