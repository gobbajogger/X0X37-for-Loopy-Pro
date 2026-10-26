#!/bin/bash
# x0x37 status: writes /workspace/x0x37-orchestrator/status.md and LIVE_BRANCHES.
# Read-only. It reads /workspace/x0x37.git with "git --git-dir" only (no fetch, no push, no change),
# result files, file times and /proc. It writes only status.md and LIVE_BRANCHES (temp file + mv).
# Made by Build and Test Runner for Orchestrator (audit item 4), 2026-10-16. Run time target: < 20 s.
# Source: tools/status.sh in the box repository. Live copy: /workspace/x0x37-orchestrator/status.sh (copy it when it does not run).
# Environment for a test (defaults = the live paths): X0X_REPO, X0X_STATUS_OUT, X0X_STATUS_TMP, X0X_RESULTS_DIR, X0X_GATE_LOCK.
# Output folder: "status.sh --out DIR" or X0X_STATUS_OUT=DIR (old name X0X_STATUS_OUTDIR).
#   The default is the live folder /workspace/x0x37-orchestrator (for cron). With an output folder that is not the live
#   folder, the temp files go to DIR/.status-tmp (X0X_STATUS_TMP sets another folder). All files that this script writes
#   are in the output folder or in the temp folder.
#   X0X_TEST_RUN not empty (tests/run_tests.sh sets it): the script refuses (exit 4) to write the live folder.
#   X0X_STATUS_LIVEDIR sets the path of the live folder for this guard (only for its test).
# Agent output: the commits on the lane branches lane/<agent>-* of each agent count
# (mb Firmware Main Builder, sb Second Builder, opt Optimizer, btr Build and Test Runner, dk Docs Keeper).
# Check 29: ROADMAP INSTALLED MISMATCH: the installed hash of smk37:docs/ROADMAP.md (the first "Installed = batch N (<hash>)"
#   or "The keyboard runs [integration] <hash>") is not the hash of the last row of INSTALLED.tsv.
#   BEHAVIOR ITEM MISSING: an integration commit after the last install (INSTALLED.tsv) names an ID (for example CLP-99)
#   with the prefix of a BEHAVIOR.md item, and integration:BEHAVIOR.md has no item line "<ID>. ..." for it.
#   X0X_DOCS_BRANCH [smk37].
set -u
export TZ=Asia/Vladivostok LC_ALL=C
REPO=${X0X_REPO:-/workspace/x0x37.git}
LIVEDIR=${X0X_STATUS_LIVEDIR:-/workspace/x0x37-orchestrator}
OUTDIR=${X0X_STATUS_OUT:-${X0X_STATUS_OUTDIR:-}}
while [ $# -gt 0 ]; do
  case "$1" in
    --out) [ -n "${2:-}" ] || { echo "status.sh: --out needs a folder" >&2; exit 2; }; OUTDIR=$2; shift 2;;
    --out=*) OUTDIR=${1#--out=}; shift;;
    *) echo "status.sh: unknown argument: $1 (use: status.sh [--out DIR])" >&2; exit 2;;
  esac
done
OUTDIR=${OUTDIR:-$LIVEDIR}
rp() { realpath -m -- "$1" 2>/dev/null || echo "$1"; }
if [ -n "${X0X_TEST_RUN:-}" ] && [ "$(rp "$OUTDIR")" = "$(rp "$LIVEDIR")" ]; then
  echo "status.sh: REFUSED: a test run (X0X_TEST_RUN) must not write the live folder $LIVEDIR. Use --out DIR or X0X_STATUS_OUT." >&2
  exit 4
fi
OUT=$OUTDIR/status.md
LIVEF=$OUTDIR/LIVE_BRANCHES
if [ "$(rp "$OUTDIR")" = "$(rp "$LIVEDIR")" ]; then
  TMPD=${X0X_STATUS_TMP:-/workspace/x0x37-build-and-test-runner/.status-tmp}   # same file system as OUTDIR: mv is atomic
else
  TMPD=${X0X_STATUS_TMP:-$OUTDIR/.status-tmp}
fi
RESULTS=${X0X_RESULTS_DIR:-/workspace/x0x37-results}
GLOCK=${X0X_GATE_LOCK:-/workspace/x0x37-refs/gate.lock}
LIVE_H=24          # live = tip commit time in the last 24 h
STALL_MIN=45       # stall = queued work and no output for more than 45 min
G() { git --git-dir="$REPO" "$@"; }
SELFD=$(cd "$(dirname "$0")" && pwd)   # the folder of this script
T0=$(date +%s.%N); NOW=$(date +%s)
mkdir -p "$OUTDIR" "$TMPD"
TMP=$(mktemp "$TMPD/status.md.XXXXXX"); TMPL=$(mktemp "$TMPD/LIVE_BRANCHES.XXXXXX")
trap 'rm -f "$TMP" "$TMPL"' EXIT
fmt() { [ -n "${1:-}" ] && [ "$1" != 0 ] && date -d "@$1" '+%m-%d %H:%M' || echo "-"; }
age() { [ -n "${1:-}" ] && [ "$1" != 0 ] || { echo "-"; return; }; local s=$((NOW-$1))
  if ((s<-60)); then echo "FUTURE +$(((-s)/60)) min"; elif ((s<3600)); then echo "$((s<0?0:s/60)) min"
  elif ((s<172800)); then echo "$((s/3600)) h $(((s%3600)/60)) min"; else echo "$((s/86400)) d"; fi; }
max() { local m=0 v; for v in "$@"; do [ -n "$v" ] && ((v>m)) && m=$v; done; echo $m; }
gtree() { G ls-tree -r --full-tree "$1" 2>/dev/null | awk -F '\t' '$2 !~ /\.md$/' | sha1sum | cut -c1-40; }
esc() { sed 's/|/\\|/g'; }

# ---------- branches ----------
declare -A TIP CT AT SUBJ
BR=(); LIVE=()
while IFS='|' read -r n h ct at s; do
  TIP[$n]=$h; CT[$n]=$ct; AT[$n]=$at; SUBJ[$n]=$s; BR+=("$n")
  case "$n" in archive/*) continue;; esac
  (( NOW-ct <= LIVE_H*3600 )) && LIVE+=("$n")
done < <(G for-each-ref refs/heads --sort=-committerdate --format='%(refname:short)|%(objectname:short)|%(committerdate:unix)|%(authordate:unix)|%(subject)')
INT=integration; INTH=${TIP[$INT]:-}

# ---------- gate results ----------
declare -A GRES GTIME GSRC
addg() { # commit7 result time(YYYY-mm-dd HH:MM) source
  local c=${1:0:7} t=$3
  if [ -z "${GTIME[$c]:-}" ] || [[ "$t" > "${GTIME[$c]}" ]] || [ "$t" = "${GTIME[$c]}" ]; then GRES[$c]=$2; GTIME[$c]=$t; GSRC[$c]=$4; fi; }
if [ -f "$RESULTS/index.md" ]; then
  while IFS='|' read -r _ c _ _ r f t _; do
    c=$(echo $c | awk '{print $1}'); [[ "$c" =~ ^[0-9a-f]{7,40}$ ]] || continue
    r=$(echo $r | sed 's/^ *//;s/ *$//'); t=$(echo $t | grep -oE '[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}')
    [ -n "$t" ] && addg "$c" "$r" "$t" "index.md"
  done < "$RESULTS/index.md"
fi
for f in "$RESULTS"/[0-9a-f]*.md; do
  [ -f "$f" ] || continue
  r=$(sed -n 's/^Result: //p' "$f" | head -1); c=$(sed -n 's/^- Commit: \([0-9a-f]*\).*/\1/p' "$f" | head -1)
  t=$(sed -n 's/^- Time: \([0-9-]* [0-9:]*\).*/\1/p' "$f" | head -1)
  [ -n "$c" ] && [ -n "$t" ] && addg "$c" "$r" "$t" "$(basename "$f")"
done
# KILLED runs write no result file; find them in the gate logs.
while read -r lg; do
  grep -q 'KILLED' "$lg" 2>/dev/null || continue
  c=$(basename "$(dirname "$lg")" .logs); addg "$c" "KILLED (no result)" "$(date -d @"$(stat -c %Y "$lg")" '+%Y-%m-%d %H:%M')" "$lg"
done < <(find /workspace /home/box/.cache/x0x37-gate -maxdepth 4 -path '*.logs/gate.log' 2>/dev/null)

# ---------- gate processes and lock ----------
LOCKTXT=free; QUEUED=(); RUNC=""  # QUEUED: "agent|commit|pid"
agent_of_path() { case "$1" in
  */x0x37-firmware-main-builder*|/workspace/a[0-9]*|/workspace/gate-a*) echo "Firmware Main Builder";;
  */x0x37-second-builder*|*/x0x37-apc-*|*/x0x37-ckfix*) echo "Second Builder";;
  */x0x37-optimizer*) echo "Optimizer";; */x0x37-build-and-test-runner*) echo "Build and Test Runner";;
  */x0x37-docs-keeper*) echo "Docs Keeper";; */x0x37-release-packager*) echo "Release Packager";;
  */x0x37-test-plan-writer*) echo "Test Plan Writer";; */x0x37-orchestrator*) echo "Orchestrator";;
  *) echo "unknown";; esac; }
GPROC=""
for p in $(pgrep -f 'x0x37_gate\.sh|tools/gate\.sh' 2>/dev/null); do
  [ "$p" = $$ ] && continue
  cmd=$(tr '\0' ' ' < /proc/$p/cmdline 2>/dev/null) || continue
  case "$cmd" in *x0x37_gate.sh*|*tools/gate.sh*) ;; *) continue;; esac
  case "$cmd" in *pgrep*|*status.sh*) continue;; esac
  # the main gate shell only (its parent is not a gate shell too)
  pp=$(awk '{print $4}' /proc/$p/stat 2>/dev/null); pcmd=$(tr '\0' ' ' < /proc/$pp/cmdline 2>/dev/null)
  case "$pcmd" in *x0x37_gate.sh*|*tools/gate.sh*) continue;; esac
  arg=$(echo "$cmd" | awk '{for(i=1;i<=NF;i++) if($i ~ /gate\.sh$/){print $(i+1); exit}}')
  wk=$(tr '\0' '\n' < /proc/$p/environ 2>/dev/null | sed -n 's/^X0X_GATE_WORK=//p'); cwd=$(readlink /proc/$p/cwd)
  wk=${wk:-$cwd}; ag=$(agent_of_path "$wk")
  st=$(( NOW - $(stat -c %Y /proc/$p) ))
  holds=no
  for fd in /proc/$p/fd/*; do [ "$(readlink "$fd" 2>/dev/null)" = "$GLOCK" ] && grep -q '^lock:.*FLOCK' /proc/$p/fdinfo/${fd##*/} 2>/dev/null && ! grep -q -- '->' /proc/$p/fdinfo/${fd##*/} && holds=yes; done
  GPROC+="| $p | ${arg:-?} | $ag | $wk | $((st/60)) min | $holds |"$'\n'
  RUNC+=" $(G rev-parse --short=7 "${arg:-x}" 2>/dev/null || echo "${arg:0:7}")"
  if [ $holds = yes ]; then LOCKTXT="held by gate PID $p, commit ${arg:-?}, agent $ag, work dir $wk, running $((st/60)) min"
  else QUEUED+=("$ag|${arg:-?}|$p"); fi
done
if [ "$LOCKTXT" = free ]; then  # a non-gate holder?
  for p in $(fuser "$GLOCK" 2>/dev/null); do for fd in /proc/$p/fd/*; do
    [ "$(readlink "$fd" 2>/dev/null)" = "$GLOCK" ] && grep -q '^lock:' /proc/$p/fdinfo/${fd##*/} 2>/dev/null && \
      LOCKTXT="held by PID $p ($(tr '\0' ' ' < /proc/$p/cmdline | cut -c1-60), cwd $(readlink /proc/$p/cwd), agent $(agent_of_path "$(readlink /proc/$p/cwd)"), parent $(awk '{print $4}' /proc/$p/stat): $(tr '\0' ' ' < /proc/$(awk '{print $4}' /proc/$p/stat)/cmdline | cut -c1-60)) (not a gate process)"; done; done
fi

# ---------- installed firmware ----------
# Primary: $RESULTS/INSTALLED.tsv (time, hash, fwsc, ctl crc, gate tree, result; one row per install,
# Firmware Main Builder appends). The newest row by time wins.
# Fallback: the ledger rows "| <any id> (INSTALLED hh:mm ...)" or "| <id> (installed) | date | Installed hh:mm"
# in BLE_ATTEMPTS.md on all branches. The newest by date + install time wins (ties: the later row in the file).
INSTTSV=$RESULTS/INSTALLED.tsv
INST="unknown"; INSTSRC=""; INSTROW=""; INSTWHY=""
if [ -s "$INSTTSV" ]; then
  best=""; bestk=0
  while IFS=$'\t' read -r t h f c gt r _; do
    case "$t" in ''|\#*|time|Time) continue;; esac
    k=$(date -d "$t" +%s 2>/dev/null) || continue
    if (( k >= bestk )); then bestk=$k; best="$t"$'\t'"$h"$'\t'"$f"$'\t'"$c"$'\t'"$gt"$'\t'"$r"; fi
  done < "$INSTTSV"
  if [ -n "$best" ]; then IFS=$'\t' read -r t h f c gt r <<<"$best"
    INST="${h:0:7} (installed $(date -d @$bestk '+%Y-%m-%d %H:%M'))"; INSTSRC="$INSTTSV (primary)"
    INSTROW=$(echo "$best" | tr '\t' ' '); INSTWHY="newest row by time in INSTALLED.tsv"; fi
fi
if [ "$INST" = unknown ]; then
  LROWS=$(for b in "${BR[@]}"; do
    G show "$b:BLE_ATTEMPTS.md" 2>/dev/null | awk -v br="$b" '/^\| *[A-Za-z]*[0-9]+[^|]*\((INSTALLED|installed)/ {print br "\t" NR "\t" $0}'
  done)
  best=""; bestk=-1; bestn=0
  while IFS=$'\t' read -r b n line; do
    [ -n "$line" ] || continue
    id=$(echo "$line" | sed -n 's/^| *\([A-Za-z]*[0-9][0-9]*\).*/\1/p')
    d=$(echo "$line" | awk -F'|' '{print $3}' | grep -oE '[0-9]{4}-[0-9]{2}-[0-9]{2}' | head -1)
    tm=$(echo "$line" | awk -F'|' '{print $2}' | grep -oE '[0-9]{1,2}:[0-9]{2}' | head -1)
    [ -n "$tm" ] || tm=$(echo "$line" | grep -oE '(Installed|INSTALLED|Install at|Time) [0-9]{1,2}:[0-9]{2}' | head -1 | grep -oE '[0-9]{1,2}:[0-9]{2}')
    h=$(echo "$line" | grep -oE 'commit [0-9a-f]{7,40}' | head -1 | awk '{print $2}')
    [ -n "$h" ] || h=$(echo "$line" | grep -oE 'Install of [A-Za-z0-9/_.-]+ [0-9a-f]{7,40}' | head -1 | awk '{print $NF}')
    [ -n "$h" ] && [ -n "$d" ] || continue
    k=$(date -d "$d ${tm:-00:00}" +%s 2>/dev/null) || continue
    if (( k > bestk || (k == bestk && n > bestn) )); then bestk=$k; bestn=$n
      best="$id|$h|$d ${tm:-?}|$b|$n|$(echo "$line" | cut -c1-240)"; fi
  done <<<"$LROWS"
  if [ -n "$best" ]; then IFS='|' read -r id h dt b n rest <<<"$best"
    INST="${h:0:7} (ledger row $id, installed $dt)"
    INSTSRC="fallback: BLE_ATTEMPTS.md line $n on branch $b ($INSTTSV is missing or empty)"
    INSTROW=$(echo "$best" | cut -d'|' -f6-)
    INSTWHY="newest install time (date column + INSTALLED/Installed/Time hh:mm) of all INSTALLED rows of any id on all branches"
  else INST="unknown (source: no INSTALLED.tsv and no ledger row with an install time and a hash)"; fi
fi

# ---------- NO GATE ROW: installs and gate runs that have no gate row in index.md ----------
# Index rows: commit7|tree12|result|epoch
IROWS=$(awk -F'|' 'NF>6 { c=$2; gsub(/^ +| +$/,"",c); split(c,a," "); t=$3; gsub(/^ +| +$/,"",t); split(t,b," ");
  r=$5; gsub(/^ +| +$/,"",r); tm=$7; gsub(/^ +| +$/,"",tm); if (a[1] ~ /^[0-9a-f]+$/) print a[1] "|" b[1] "|" r "|" tm }' "$RESULTS/index.md" 2>/dev/null |
  while IFS='|' read -r c t r tm; do e=$(date -d "$(echo "$tm" | grep -oE '[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}')" +%s 2>/dev/null || echo 0); echo "$c|$t|$r|$e"; done)
gatepass_tree() { # tree12 -> 0 if a gate-script PASS row exists (PASS, PASS (result used again...), PASS (force: ...))
  echo "$IROWS" | awk -F'|' -v t="$1" '$2==t && ($3=="PASS" || $3 ~ /^PASS \(result used again/ || $3 ~ /^PASS \(force: /) {f=1} END {exit !f}'; }
NGR=""; NGRN=0; INSTCHK=""
# Allowlist "pre-guard, historical" (Orchestrator, 2026-10-16 23:19 +10): installs and gate runs from before
# the install guard (tools/require_gate_pass.sh). They go to the historical list, not to NO GATE ROW.
HIST_INST=" d9844a4 d184a64 ca1d4d2 94285b5 9a733fe "      # ledger rows 63 64 65 67, and b1 (9a733fe)
HIST_RUN=" 4bfb07b=/workspace/gate-a76pad/4bfb07b.logs 6b5bc43=/workspace/x0x37-build-and-test-runner/gate-killtest/6b5bc43.logs "
hist_why() { case "$1" in 6b5bc43) echo "test fixture (KILLED on purpose; no result by design)";; *) echo "pre-guard";; esac; }  # Orchestrator 23:26 +10
HISTL=""; HISTN=0
# installs: INSTALLED.tsv rows, and ledger INSTALLED rows of the last 24 h
ILIST=""
if [ -s "$INSTTSV" ]; then
  while IFS=$'\t' read -r t h _; do case "$t" in ''|\#*|time|Time) continue;; esac
    k=$(date -d "$t" +%s 2>/dev/null) || continue; ILIST+="INSTALLED.tsv $t|${h:0:7}|$k"$'\n'; done < "$INSTTSV"
fi
[ -n "${LROWS:-}" ] || LROWS=$(for b in "${BR[@]}"; do
    G show "$b:BLE_ATTEMPTS.md" 2>/dev/null | awk -v br="$b" '/^\| *[A-Za-z]*[0-9]+[^|]*\((INSTALLED|installed)/ {print br "\t" NR "\t" $0}'; done)
while IFS=$'\t' read -r b n line; do
  [ -n "$line" ] || continue
  id=$(echo "$line" | sed -n 's/^| *\([A-Za-z]*[0-9][0-9]*\).*/\1/p')
  d=$(echo "$line" | awk -F'|' '{print $3}' | grep -oE '[0-9]{4}-[0-9]{2}-[0-9]{2}' | head -1)
  tm=$(echo "$line" | awk -F'|' '{print $2}' | grep -oE '[0-9]{1,2}:[0-9]{2}' | head -1)
  [ -n "$tm" ] || tm=$(echo "$line" | grep -oE '(Installed|INSTALLED|Install at|Time) [0-9]{1,2}:[0-9]{2}' | head -1 | grep -oE '[0-9]{1,2}:[0-9]{2}')
  h=$(echo "$line" | grep -oE 'commit [0-9a-f]{7,40}' | head -1 | awk '{print $2}')
  [ -n "$h" ] || h=$(echo "$line" | grep -oE 'Install of [A-Za-z0-9/_.-]+ [0-9a-f]{7,40}' | head -1 | awk '{print $NF}')
  [ -n "$h" ] && [ -n "$d" ] || continue
  k=$(date -d "$d ${tm:-00:00}" +%s 2>/dev/null) || continue
  ILIST+="ledger row $id ($d ${tm:-?})|${h:0:7}|$k"$'\n'
done <<<"$LROWS"
while IFS='|' read -r src h k; do
  [ -n "$h" ] || continue; (( NOW - k <= 86400 )) || continue
  full=$(G rev-parse --verify --quiet "$h^{commit}" 2>/dev/null)
  if [ -z "$full" ]; then st="**NO GATE ROW** (commit not in the box repo)"
  else t12=$(gtree "$full" | cut -c1-12)
    if gatepass_tree "$t12"; then st="gate PASS row (tree $t12)"; else st="**NO GATE ROW** (tree $t12)"; fi; fi
  if [[ "$HIST_INST" == *" ${h:0:7} "* ]]; then HISTN=$((HISTN+1)); HISTL+="| install ${h:0:7} | $src | ${st//\*\*NO GATE ROW\*\*/no gate row} |"$'\n'; st="historical (pre-guard): ${st//\*\*NO GATE ROW\*\*/no gate row}"
  else case "$st" in *"NO GATE ROW"*) NGRN=$((NGRN+1)); NGR+="install ${h:0:7} ($src), ";; esac; fi
  INSTCHK+="| ${h:0:7} | $src | $st |"$'\n'
done < <(echo -n "$ILIST" | sort -s -t'|' -k2,2 -u | sort -t'|' -k3,3n)
# gate runs: <commit>.logs/gate.log written in the last 24 h. VOID runs stop before they make a logs folder.
# A run matches an index row of its commit with a time from 15 min before to 5 min after its last gate.log write.
RUNCHK=""
while read -r lg; do
  m=$(stat -c %Y "$lg"); (( NOW - m <= 86400 )) || continue
  c=$(basename "$(dirname "$lg")" .logs); c7=${c:0:7}
  if [[ " $RUNC " == *" $c7 "* ]] && (( NOW - m < 900 )); then RUNCHK+="| $c7 | $(dirname "$lg") | $(fmt "$m") | running, not checked |"$'\n'; continue; fi
  hit=$(echo "$IROWS" | awk -F'|' -v c="$c7" -v m="$m" '$1==c && $4>=m-900 && $4<=m+300 {print $3; exit}')
  if [ -n "$hit" ]; then RUNCHK+="| $c7 | $(dirname "$lg") | $(fmt "$m") | row: $(echo "$hit" | cut -c1-50 | esc) |"$'\n'
  elif [[ "$HIST_RUN" == *" $c7=$(dirname "$lg") "* ]]; then HISTN=$((HISTN+1)); HISTL+="| gate run $c7 | $(dirname "$lg") | no index row; reason: $(hist_why "$c7") |"$'\n'
    RUNCHK+="| $c7 | $(dirname "$lg") | $(fmt "$m") | historical ($(hist_why "$c7")): no index row |"$'\n'
  else why="no index row"; grep -q 'KILLED' "$lg" && why="no index row (KILLED: no result by design)"
    RUNCHK+="| $c7 | $(dirname "$lg") | $(fmt "$m") | **NO GATE ROW**: $why |"$'\n'; NGRN=$((NGRN+1)); NGR+="gate run $c7 ($(dirname "$lg")), "; fi
done < <(find /workspace /home/box/.cache/x0x37-gate -maxdepth 4 -path '*.logs/gate.log' 2>/dev/null)

# ---------- agents ----------
# name;branch regex (box repo);folders (space separated);result files/dirs
AGENTS=(
"Firmware Main Builder;^(ble-|trial-a76|lane/mb-);/workspace/x0x37-firmware-main-builder /workspace/x0x37-firmware-main-builder-in /workspace/a[0-9]*wt /workspace/a[0-9]*trial /workspace/a[0-9]*-gate* /workspace/gate-a[0-9]* /workspace/padinv /workspace/b1wt;"
"Second Builder;^(clip-keys|apc-colors|wip/clip-keys|lane/sb-);/workspace/x0x37-second-builder;$RESULTS/second-builder"
"Merge Integrator;^integration$;-;$RESULTS/trial-merge-state.txt $RESULTS/LIVE_BRANCHES"
"Optimizer;^(opt|trial-a76-m|lane/opt-);/workspace/x0x37-optimizer;"
"Build and Test Runner;^(test-speed$|lane/btr-);/workspace/x0x37-build-and-test-runner;"
"Docs Keeper;^(smk37$|lane/dk-);/workspace/x0x37-docs-keeper;"
"Test Plan Writer;^NONE$;/workspace/x0x37-test-plan-writer;"
"Release Packager;^NONE$;/workspace/x0x37-release-packager;"
"Orchestrator;^NONE$;/workspace/x0x37-orchestrator;"
)
newest() { # newest file mtime under the dirs, without .git, build, caches and the files this script writes
  local m=0 v d
  for d in "$@"; do [ -e "$d" ] || continue
    v=$(find "$d" -xdev \( -name .git -o -name build -o -name node_modules -o -name __pycache__ -o -name .status-tmp \) -prune -o -type f \
        ! -path "$OUT" ! -path "$LIVEF" ! -path "$OUTDIR/status.sh" ! -name 'status-cron.log' ! -name 'status-loop.pid' -printf '%T@\n' 2>/dev/null | sort -n | tail -1)
    v=${v%.*}; [ -n "$v" ] && ((v>m)) && m=$v; done; echo $m; }
worktrees_of() { local d=$1 g; [ -d "$d/.git/worktrees" ] || return; for g in "$d"/.git/worktrees/*/gitdir; do [ -f "$g" ] && dirname "$(cat "$g")"; done; }
gitact() { local d=$1 m=0 v f gd=""; [ -f "$d/.git" ] && gd=$(sed -n 's/^gitdir: //p' "$d/.git")
  for f in "$d"/.git/logs/HEAD "$d"/.git/worktrees/*/logs/HEAD ${gd:+"$gd/logs/HEAD"}; do [ -f "$f" ] && v=$(stat -c %Y "$f") && ((v>m)) && m=$v; done; echo $m; }

# ---------- branches that must not merge (never queued work) ----------
declare -A NOMERGE
NOMERGE[smk37]="docs base branch"; NOMERGE[refs-oscuro]="never merge"
if [ -f "$RESULTS/LIVE_BRANCHES" ]; then
  while read -r n rest; do
    echo "$rest" | grep -qiE 'do not merge|watch only' || continue
    if [ -n "${TIP[$n]:-}" ]; then NOMERGE[$n]="do not merge (results/LIVE_BRANCHES)"
    else  # a list line, e.g. "Side builds (..., do not merge): a b, c d." -> the names after the colon, up to the first full stop
      for w in $(echo "$rest" | sed -n 's/^[^:]*: *\([^.]*\)\..*/\1/p' | tr ',' ' '); do [ -n "${TIP[$w]:-}" ] && NOMERGE[$w]="do not merge (results/LIVE_BRANCHES list)"; done
    fi
  done < <(grep -v '^#' "$RESULTS/LIVE_BRANCHES")
fi

# ---------- behavior diff (file content) ----------
# Method: compare smk37 BEHAVIOR.md (docs/BEHAVIOR.md, else BEHAVIOR.md) with integration:BEHAVIOR.md.
# Ignore (1) the header: all lines before the first "## " heading, and (2) the sync note paragraph
# that starts with "Reference copy:" (metadata, not behaviour). Trailing spaces are ignored.
bnorm() { awk 'f==0 && /^## /{f=1} f==0{next} /^Reference copy:/{skip=1} skip && /^[[:space:]]*$/{skip=0; next} skip{next} {sub(/[ \t]+$/,""); print}'; }
BSRC=""; for pth in docs/BEHAVIOR.md BEHAVIOR.md; do G cat-file -e "smk37:$pth" 2>/dev/null && { BSRC="smk37:$pth"; break; }; done
BDIFF=""; BNOTE=""
if [ -z "$BSRC" ] || ! G cat-file -e "$INT:BEHAVIOR.md" 2>/dev/null; then BNOTE="cannot compare: ${BSRC:-smk37 has no BEHAVIOR.md}, $INT:BEHAVIOR.md $(G cat-file -e "$INT:BEHAVIOR.md" 2>/dev/null && echo present || echo missing)"
else
  BDIFF=$(diff -U1 --label "$BSRC" --label "$INT:BEHAVIOR.md" <(G show "$BSRC" | bnorm) <(G show "$INT:BEHAVIOR.md" | bnorm))
  BRAW=$(diff <(G show "$BSRC") <(G show "$INT:BEHAVIOR.md") | grep -c '^[<>]')
  BREAL=$(echo "$BDIFF" | grep -cE '^[-+][^-+]|^[-+]$')
fi

# ================= write =================
# ---------- check 27: docs lint; check 29: ROADMAP installed hash, BEHAVIOR items since the last install ----------
DOCB=${X0X_DOCS_BRANCH:-smk37}
INSTLAST=""; [ -f "$RESULTS/INSTALLED.tsv" ] && INSTLAST=$(awk -F '\t' 'NR > 1 && $2 ~ /^[0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f]/ {h = $2} END {print h}' "$RESULTS/INSTALLED.tsv")
RMH=$(G show "$DOCB:docs/ROADMAP.md" 2>/dev/null | grep -oiE 'installed = batch [0-9]+ \([0-9a-f]{7,40}\)|the keyboard runs (integration )?[0-9a-f]{7,40}' | head -1 | grep -oE '[0-9a-f]{7,40}' | tail -1)
if [ -z "$INSTLAST" ]; then L29A="- ROADMAP installed (check 29): not checked (no hash row in $RESULTS/INSTALLED.tsv)."
elif [ -z "$RMH" ]; then L29A="- **ROADMAP INSTALLED MISMATCH** (check 29): ROADMAP.md on $DOCB names no installed hash, the last INSTALLED.tsv row is ${INSTLAST:0:7}."
elif [ "${INSTLAST#"$RMH"}" = "$INSTLAST" ]; then L29A="- **ROADMAP INSTALLED MISMATCH** (check 29): ROADMAP.md on $DOCB names $RMH, the last INSTALLED.tsv row is ${INSTLAST:0:7}."
else L29A="- ROADMAP installed (check 29): ${INSTLAST:0:7} = the last INSTALLED.tsv row."; fi
L29B="- BEHAVIOR items since the last install (check 29): not checked (no install hash in the repository)."
if [ -n "$INSTLAST" ] && G cat-file -e "$INSTLAST^{commit}" 2>/dev/null; then
  BDEF=$(G show "$INT:BEHAVIOR.md" 2>/dev/null | grep -oE '^(- )?[A-Z]{2,4}-[0-9]+[a-z]?\.' | sed 's/^- //; s/\.$//' | sort -u)
  BPRE=$(echo "$BDEF" | cut -d- -f1 | sort -u)
  BNAMED=$(G log --format='%s%n%b' "$INSTLAST..$INT" 2>/dev/null | grep -oE '\b[A-Z]{2,4}-[0-9]+[a-z]?\b' | sort -u | while read -r id; do echo "$BPRE" | grep -qx "${id%%-*}" && echo "$id"; done)
  BMISS=$(for id in $BNAMED; do echo "$BDEF" | grep -qx "$id" || echo "$id"; done | paste -sd, - | sed 's/,/, /g')
  NN=$(echo "$BNAMED" | grep -c .)
  if [ -n "$BMISS" ]; then L29B="- **BEHAVIOR ITEM MISSING** (check 29): $BMISS (named by integration commits after the install ${INSTLAST:0:7}, no item in $INT:BEHAVIOR.md)."
  else L29B="- BEHAVIOR items since the last install (check 29): $NN named, all in BEHAVIOR.md."; fi
fi
{
echo "# x0x37 status"
echo
echo "Made $(date '+%Y-%m-%d %H:%M:%S') VLAT (UTC+10) by status.sh (Build and Test Runner). Every 5 min by cron. Read-only."
echo
echo "Definitions:"
echo "- **Live branch:** a branch in /workspace/x0x37.git, not archive/*, with a tip commit time in the last ${LIVE_H} h. LIVE_BRANCHES in this folder has their names."
echo "- **Age:** the time from the tip commit (committer time) to now."
echo "- **Stall:** an agent with queued work (a live lane branch with commits that are not in integration, or a gate that waits for the lock) and no output (commit, reflog or file write in its folder) for more than ${STALL_MIN} min."
echo "- **Gate status of a tip:** the result file for the tree of the tip (the gate tree key: ls-tree without *.md). Same tree = same result."
echo
echo "## Summary"
echo
echo "- Integration: ${INTH:-missing} \"${SUBJ[$INT]:-}\" ($(age "${CT[$INT]:-0}") old)."
echo "- Installed firmware: $INST. Source: ${INSTSRC:-none}."
echo "- Gate lock: $LOCKTXT."
if [ "$NGRN" -gt 0 ]; then echo "- **NO GATE ROW** ($NGRN): ${NGR%, }. Historical (pre-guard, allowlisted): $HISTN."; else echo "- NO GATE ROW: 0 (each install and each gate run of the last 24 h has a gate row, or is on the pre-guard allowlist: $HISTN historical)."; fi
if [ -n "$BNOTE" ]; then echo "- BEHAVIOR diff: $BNOTE."
elif [ -n "$BDIFF" ]; then echo "- **BEHAVIOR DIFF:** $BSRC and $INT:BEHAVIOR.md differ in $BREAL text lines (header and sync note ignored). See below."
else echo "- BEHAVIOR diff: none. $BSRC = $INT:BEHAVIOR.md in text (header and sync note ignored; raw diff $BRAW lines)."; fi
echo "$L29A"
echo "$L29B"
STALLS=""
echo
echo "## Live branches (tip commit in the last ${LIVE_H} h)"
echo
echo "| Branch | Tip | Subject | Author time | Commit time | Age | Gate for tip tree |"
echo "|---|---|---|---|---|---|---|"
for n in "${LIVE[@]}"; do
  echo "$n" >> "$TMPL"
  tr=$(gtree "${TIP[$n]}"); g="-"
  if [ -f "$RESULTS/$tr.md" ]; then g="$(sed -n 's/^Result: //p' "$RESULTS/$tr.md" | head -1) (tree ${tr:0:12})"; elif [ -n "${GRES[${TIP[$n]}]:-}" ]; then g="${GRES[${TIP[$n]}]} (${GTIME[${TIP[$n]}]})"; else g="no result"; fi
  echo "| $n | ${TIP[$n]} | $(echo "${SUBJ[$n]}" | cut -c1-70 | esc) | $(fmt "${AT[$n]}") | $(fmt "${CT[$n]}") | $(age "${CT[$n]}") | $g |"
done
na=0; for n in "${BR[@]}"; do case "$n" in archive/*) na=$((na+1));; esac; done
nold=$(( ${#BR[@]} - na - ${#LIVE[@]} ))
echo
echo "Not live: $nold branches older than ${LIVE_H} h, $na archive/* branches."
echo
echo "## Integration and each lane"
echo
echo "Lane-only = commits on the lane that are not in integration. Not-merged patches = lane-only commits with no same patch in integration (git cherry). Integration ahead = commits on integration that are not on the lane."
echo
echo "| Lane | Lane-only commits | Not-merged patches | Integration ahead | Merge note | Lane-only commits (max 5) |"
echo "|---|---|---|---|---|---|"
declare -A OPENW
for n in "${LIVE[@]}"; do
  [ "$n" = "$INT" ] && continue
  a=$(G rev-list --count "$INT..$n"); b=$(G rev-list --count "$n..$INT")
  if (( a > 0 && a <= 60 )); then c=$(G cherry "$INT" "$n" 2>/dev/null | grep -c '^+'); else c=$([ "$a" = 0 ] && echo 0 || echo "n/a (>60)"); fi
  l=$(G log --no-merges --format='%h %s' "$INT..$n" 2>/dev/null | head -5 | cut -c1-60 | esc | paste -sd ';' | sed 's/;/; /g')
  echo "| $n | $a | $c | $b | ${NOMERGE[$n]:-open} | ${l:--} |"
  [ "$c" != 0 ] && [ -z "${NOMERGE[$n]:-}" ] && OPENW[$n]=$c
done
echo
echo "## Last gate result for each commit (newest 20)"
echo
echo "Sources: $RESULTS/index.md, the result files <tree>.md, and gate.log files with KILLED (KILLED writes no result file)."
echo
echo "| Commit | Result | Time | Source |"
echo "|---|---|---|---|"
for c in "${!GTIME[@]}"; do echo "${GTIME[$c]}|$c"; done | sort -r | head -20 | while IFS='|' read -r t c; do
  echo "| $c | $(echo "${GRES[$c]}" | cut -c1-70 | esc) | $t | ${GSRC[$c]} |"; done
echo
echo "## Gate lock and gate runs"
echo
echo "- Lock file: $GLOCK. Holder: $LOCKTXT."
if [ -n "$GPROC" ]; then echo; echo "| PID | Commit | Agent | Work dir | Running | Holds lock |"; echo "|---|---|---|---|---|---|"; printf '%s' "$GPROC"; else echo "- No gate process runs."; fi
echo
echo "## NO GATE ROW check (last 24 h)"
echo
echo "An install needs a gate-script PASS row in index.md for the tree of its commit (PASS, PASS (result used again), PASS (force: ...); not TRIAL, VOID, FAIL, KILLED or a hand-written PASS). A gate run (a <commit>.logs/gate.log) needs an index row of its commit, written from 15 min before to 5 min after its last gate.log write. VOID setup runs make no logs folder, so they are not in this list. Running gates are not checked."
echo
echo "| Install | Source | Gate row |"; echo "|---|---|---|"; printf '%s' "${INSTCHK:-| - | no install in the last 24 h | - |
}"
echo
echo "| Gate run | Logs | Last gate.log write | Index row |"; echo "|---|---|---|---|"; printf '%s' "${RUNCHK:-| - | no gate run in the last 24 h | - | - |
}"
echo
echo "### Historical (pre-guard)"
echo
echo "Allowlisted by Orchestrator (2026-10-16 23:19 +10 and 23:26 +10): from before the install guard, or a test fixture. Not counted as NO GATE ROW."
echo
echo "| Item | Source | Gate row |"; echo "|---|---|---|"; printf '%s' "${HISTL:-| - | none in the last 24 h | - |
}"
echo
echo "## Installed firmware"
echo
echo "- Hash: $INST."
echo "- Source: ${INSTSRC:-none}."
echo "- Why this row: ${INSTWHY:--}."
echo "- Source row: \`$(echo "${INSTROW:--}" | sed 's/`/'"'"'/g')\`"
echo "- Method: primary source $INSTTSV (newest row by time). If it is missing or empty: the newest INSTALLED row of any id (a74, b1, ...) in BLE_ATTEMPTS.md on all branches, by date + install time. The device itself is not read."
echo
echo "## Agents"
echo
echo "Output = the newest of: last commit on its box branches, its local git reflog, a file write in its folder or its result files."
echo
echo "| Agent | Last commit (box branch) | Local git activity | Last file / result write | Output age | Queued work | Flag |"
echo "|---|---|---|---|---|---|---|"
for a in "${AGENTS[@]}"; do
  IFS=';' read -r name rx folders rfiles <<<"$a"
  lc=0; lb=""
  for n in "${BR[@]}"; do case "$n" in archive/*) continue;; esac; [[ "$n" =~ $rx ]] || continue; (( ${CT[$n]} > lc )) && { lc=${CT[$n]}; lb=$n; }; done
  if [ "$name" = "Merge Integrator" ]; then lc=$(G log -1 --merges --format=%ct "$INT" 2>/dev/null || echo 0); lb="merge on integration"; fi
  dirs=(); ga=0
  if [ "$folders" != - ]; then for d in $folders; do [ -d "$d" ] || continue; dirs+=("$d"); for w in $(worktrees_of "$d"); do dirs+=("$w"); done; v=$(gitact "$d"); ((v>ga)) && ga=$v
      for sub in "$d"/*/; do [ -d "$sub/.git" ] && v=$(gitact "${sub%/}") && ((v>ga)) && ga=$v; done; done; fi
  fw=$(newest "${dirs[@]}" $rfiles)
  out=$(max "$lc" "$ga" "$fw")
  q=""
  for n in "${!OPENW[@]}"; do [[ "$n" =~ $rx ]] && [ "$n" != "$INT" ] && [ "$name" != "Merge Integrator" ] && q+="$n (${OPENW[$n]}), "; done
  for e in "${QUEUED[@]:-}"; do [ -n "$e" ] || continue; IFS='|' read -r qa qc qp <<<"$e"; [ "$qa" = "$name" ] && q+="gate $qc waits (PID $qp), "; done
  q=${q%, }
  flag=ok
  if [ -n "$q" ] && (( NOW - out > STALL_MIN*60 )); then flag="**STALL**"; STALLS+="$name, "; fi
  (( out > NOW + 60 )) && flag="**FUTURE TIME**"
  echo "| $name | $(fmt "$lc") ${lb:+($lb)} | $(fmt "$ga") | $(fmt "$fw") | $(age "$out") | ${q:--} | $flag |"
done
echo "| Firmware Q&A | - | - | - | - | - | no box folder, not watched |"
echo
if [ -n "$STALLS" ]; then echo "**STALL FLAGS:** ${STALLS%, }."; else echo "Stall flags: none."; fi
echo
echo "## BEHAVIOR diff (file content)"
echo
echo "Compares ${BSRC:-smk37 BEHAVIOR.md} with $INT:BEHAVIOR.md. Method: ignore the header (all lines before the first \"## \" heading), the sync note paragraph that starts with \"Reference copy:\", and trailing spaces. Flag only when other text differs."
echo
if [ -n "$BNOTE" ]; then echo "$BNOTE."
elif [ -n "$BDIFF" ]; then echo "**BEHAVIOR DIFF** ($BREAL text lines differ; first 20 diff lines):"; echo; echo '```diff'; echo "$BDIFF" | head -22 | cut -c1-200; echo '```'
else echo "No text diff."; fi
echo
echo "Run time: RUNTIME_PLACEHOLDER s."
} > "$TMP"
RT=$(echo "$(date +%s.%N) - $T0" | bc 2>/dev/null || python3 -c "print(round($(date +%s.%N)-$T0,1))")
sed -i "s/RUNTIME_PLACEHOLDER/$(printf '%.1f' "$RT")/" "$TMP"
chmod 644 "$TMP" "$TMPL"
mv -f "$TMPL" "$LIVEF" && mv -f "$TMP" "$OUT"
trap - EXIT
echo "status.sh ok $(date '+%F %T') ${RT}s"
