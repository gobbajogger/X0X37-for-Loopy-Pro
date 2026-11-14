# Independent source-projection review

Accepted for the exact bounded 14-file source projection described by
`SOURCE-ALLOWLIST.json`, SHA-256
`8d1c564ea939a3d4f2c9d7831341ad1019973584a0d0cf76949a079d9839dfd7`.
The parent is project `a0c8698f724e28ec6275307525933c195817b3c5`,
tree `e58592f678c804798aa2250da84f815520731ca5`. All allowlisted byte
lengths, SHA-256 hashes, Git blob identities and regular nonexecutable file modes
matched; the payload contained no extra files.

The six compiler-facing C++/map/test files remain unchanged from the initial
projection. The underlying accepted 41-file parameter-port source closure remains
exact at manifest SHA-256
`0cab53bcdb0fd490e4f65a452d72e176568d57f4e380a5253248b673224475fa`.
The reused DSP recurrence, numeric admission, parameter-144 current-output lag,
lazy coefficients and restore state have no semantic projection changes.

## Findings resolved

Three P2 findings were reproduced and repaired before this freeze:

- An unverified quoted header could override freshly generated source metadata
  while smoke passed. The runner now stages enumerated authenticated inputs,
  rejects unexpected project files and quoted-header neighbors, and checks each
  compiler dependency path/hash set. The original header probe now rejects with
  exit 2 before stage creation or compilation. Explicit validation remains enabled
  under normal and optimized Python.
- Mutation checks could count an unrelated cached test object as detection. The
  driver now compiles fresh attributable objects; mutation acceptance requires
  executing a passing fresh full baseline. Preflight explicitly provides neither
  full-baseline nor mutation acceptance.
- The authenticated verifier source could be bypassed by adjacent cached Python
  bytecode. All three explicit verifier loaders now compile source bytes directly.
  The build driver hashes and compiles the same buffer.

The independent final preflight installed both the unrelated legacy test object
and the unchecked-hash verifier cache that bypassed the earlier loader. From a
project/input path containing spaces, it created one real fresh stage, compiled
and linked five translation units, and executed only the six-frame smoke:
900 output values and 2,849 checks passed. The cached marker was absent; the fresh
test object differed from the legacy object. Actual staged dependency hashes,
object hashes, link inputs and executable hashes were independently checked. No
full-test output was produced; the receipt explicitly reports
`full_baseline_executed=false` and `mutation_acceptance=false`. A separate direct
metadata-generator probe also ignored the unverified bytecode and generated the
reference header from the pinned public source.

The producer's six positive/51 negative binding checks and two cache-loader
controls were inspected with their indexed evidence hashes. Those complete
batches were not independently replayed. Existing oracle/cache/graph/bank and
eleven-mutant results remain retained source-stage evidence, not new executions
of the portable full-test or mutation-acceptance modes.

## Public content and limits

The payload contains eligible source, tests, public dependency identities and
bounded technical documentation. Structural index/slot arrays derive from public
metadata. Reference value arrays are generated locally from explicitly verified
public source. No private banks, bank-derived parameter arrays, audio, generated
waveforms, binaries, SDK/toolchain content, internal owner identifiers or private
local paths occur in the reviewed payload. Pinned sibling MIT notices and
attribution remain intact.

Host compilers, system headers and libraries remain trusted toolchain inputs.
No native build, device action or complete engine qualification was performed.
Full frontend setter semantics, concurrency, graph/event/mono/LFO/FX integration,
preset mapping, target resource/CPU and whole-instrument acceptance remain open.
The repaired portable full-baseline/mutation acceptance mode was source-reviewed
and preflight-checked, but its complete runtime batch was deliberately not rerun.

**Verdict: accepted for this exact source-only projection. Actionable findings: none.**
