#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""lane_files.py: the "Lane files" map of AGENTS_AND_ROOMS.md (checks 8 and 31 of rules v4, commit.md rule 10).

  lane_files.py map   [MAP] [--repo GITDIR]                       the map (lane, kind, pattern) as text
  lane_files.py cover [MAP] [--repo GITDIR] [--tree REF | --files LIST]  check 31: each tracked file of REF (or each
                                                                    line of the file LIST) has a lane
  lane_files.py lane  [MAP] [--repo GITDIR] --lane TAG --base B --head H    check 8: the lane changes only its files
  lane_files.py of    [MAP] [--repo GITDIR] PATH ...                the lanes of each PATH

MAP: --map-file FILE, or --map-ref REF [smk37] and --map-path PATH [docs/AGENTS_AND_ROOMS.md] in GITDIR
[X0X_REPO, else /workspace/x0x37.git]. REF of cover: [integration].

The section "## Lane files" has three tables (header row, then "|---|" row):
  1. owned rows:  | <lane> (<name>) | <pattern>, <pattern>, ... | <evidence> |   (only this lane)
  2. shared rows: | <file> | <lane>, <lane> | <evidence> |                     (each named lane)
  3. defaults:    | <pattern>, <pattern> | <lane> |
The tables are in that order; the header of each table tells its kind ("Owns", "Shared", "Default").
A pattern: "**" = any characters (also "/"), "*" = any characters but "/", "?" = one character but "/".
"dir/**" also matches dir itself. "<pattern> (but not <pattern>, ...)" excludes paths.
Lanes of a file = the lanes of each owned row and shared row that matches it. If none matches: the lanes of each default
row that matches it (a row in the first two tables wins over the default table). No lane: the file has no lane.

Output and exit code:
  cover: "LANES OK <n> of <n> files have a lane" (0), or "LANES FAIL <k> of <n> files map to no lane: <files>" (1).
         "LANES WARN: <pattern> (<lane>) matches no file" for an owned or shared pattern without a file (no failure).
  lane:  "LANE OK lane/<tag>: <n> changed files, ..." (0), or "LANE REFUSED lane/<tag>: files of another lane only:
         <file> (<lanes>) ..." (1). A changed file with no lane is not refused (check 31 reports it).
         The changed files: git diff --name-only B H. B "auto": the merge base of H with REF of --tree [integration],
         or with --map-ref [smk37] when that merge base is newer (a lane on smk37, for example lane/dk-rules-*).
  any:   "LANES ERROR: ..." (2): no map, no "Lane files" section, a table that the parser cannot read.
"""
import os
import re
import subprocess
import sys

LANES = ("mb", "sb", "opt", "btr", "dk")


def die(msg):
    print("LANES ERROR: " + msg)
    sys.exit(2)


def git(repo, *a):
    r = subprocess.run(["git", "--git-dir", repo] + list(a), capture_output=True, text=True, errors="replace")
    if r.returncode:
        raise RuntimeError("git %s: %s" % (" ".join(a), r.stderr.strip()[:200]))
    return r.stdout


def glob_re(p):
    out, i = "", 0
    while i < len(p):
        if p.startswith("/**", i) and i + 3 == len(p):
            out += "(/.*)?"; i += 3
        elif p.startswith("**", i):
            out += ".*"; i += 2
        elif p[i] == "*":
            out += "[^/]*"; i += 1
        elif p[i] == "?":
            out += "[^/]"; i += 1
        else:
            out += re.escape(p[i]); i += 1
    return re.compile("^" + out + "$")


def split_patterns(cell):
    """'a, b (but not c, d), e' -> [(a, []), (b, [c, d]), (e, [])]"""
    out, depth, cur = [], 0, ""
    for ch in cell:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        if ch == "," and depth == 0:
            out.append(cur); cur = ""
        else:
            cur += ch
    out.append(cur)
    res = []
    for item in out:
        item = item.strip().strip("`")
        if not item:
            continue
        m = re.match(r"^(\S+)\s*(?:\((.*)\))?$", item)
        if not m:
            die("cannot read the pattern %r" % item)
        exc = []
        if m.group(2):
            b = re.match(r"^\s*but not\s+(.*)$", m.group(2))
            if not b:
                die("cannot read %r (only \"(but not ...)\" is allowed)" % item)
            exc = [x.strip().strip("`") for x in b.group(1).split(",") if x.strip()]
        res.append((m.group(1).strip("`"), exc))
    return res


def lane_of(cell):
    m = re.match(r"^\s*`?(mb|sb|opt|btr|dk)`?\b", cell)
    return m.group(1) if m else None


def parse(text):
    """-> list of rows (kind, lane, pattern, excludes); kind = owned | shared | default"""
    lines = text.splitlines()
    try:
        s = next(i for i, l in enumerate(lines) if re.match(r"^#+\s+Lane files\s*$", l))
    except StopIteration:
        die('no "Lane files" section')
    rows, kind, sec = [], None, []
    for l in lines[s + 1:]:
        if re.match(r"^#{1,2}\s", l):
            break
        sec.append(l)
    for l in sec:
        if not l.startswith("|"):
            continue
        cells = [c.strip() for c in l.strip().strip("|").split("|")]
        if all(re.match(r"^:?-+:?$", c) for c in cells if c):
            continue
        h = " ".join(cells).lower()
        if cells[0].lower() == "lane" and "owns" in h:
            kind = "owned"; continue
        if cells[0].lower().startswith("shared"):
            kind = "shared"; continue
        if cells[0].lower().startswith("default"):
            kind = "default"; continue
        if kind is None:
            die("a table row before a known table header: %s" % l[:80])
        if kind == "owned":
            ln = lane_of(cells[0])
            if not ln or len(cells) < 2:
                die("owned row without a lane: %s" % l[:80])
            for p, exc in split_patterns(cells[1]):
                rows.append(("owned", ln, p, exc))
        elif kind == "shared":
            lanes = [lane_of(x) for x in cells[1].split(",")] if len(cells) > 1 else []
            if not lanes or None in lanes:
                die("shared row with a bad lane list: %s" % l[:80])
            for p, exc in split_patterns(cells[0]):
                for ln in lanes:
                    rows.append(("shared", ln, p, exc))
        else:
            ln = lane_of(cells[1]) if len(cells) > 1 else None
            if not ln:
                die("default row without a lane: %s" % l[:80])
            for p, exc in split_patterns(cells[0]):
                rows.append(("default", ln, p, exc))
    if not any(r[0] == "owned" for r in rows) or not any(r[0] == "default" for r in rows):
        die('the "Lane files" section has no owned table or no default table')
    return [(k, ln, p, [glob_re(e) for e in exc], glob_re(p)) for k, ln, p, exc in rows]


def lanes_for(rows, f):
    def hit(r):
        return r[4].match(f) and not any(e.match(f) for e in r[3])
    first = sorted(set(r[1] for r in rows if r[0] in ("owned", "shared") and hit(r)))
    if first:
        return first, "row"
    return sorted(set(r[1] for r in rows if r[0] == "default" and hit(r))), "default"


def main(argv):
    if len(argv) < 2 or argv[1] not in ("map", "cover", "lane", "of"):
        print(__doc__, file=sys.stderr); return 2
    cmd, a = argv[1], argv[2:]
    opt = {"--repo": os.environ.get("X0X_REPO", "/workspace/x0x37.git"), "--map-ref": "smk37",
           "--map-path": "docs/AGENTS_AND_ROOMS.md", "--map-file": None, "--tree": "integration",
           "--lane": None, "--base": None, "--head": None, "--files": None}
    rest = []
    while a:
        if a[0] in opt and len(a) > 1:
            opt[a[0]] = a[1]; a = a[2:]
        else:
            rest.append(a[0]); a = a[1:]
    repo = opt["--repo"]
    try:
        if opt["--map-file"]:
            text = open(opt["--map-file"], encoding="utf-8").read(); src = opt["--map-file"]
        else:
            src = "%s:%s" % (opt["--map-ref"], opt["--map-path"])
            text = git(repo, "show", src)
        rows = parse(text)
        if cmd == "map":
            for k, ln, p, exc, _ in rows:
                print("%-7s %-4s %s" % (k, ln, p))
            return 0
        if cmd == "of":
            for f in rest:
                ls, why = lanes_for(rows, f)
                print("%s: %s (%s)" % (f, " ".join(ls) or "no lane", why))
            return 0
        if cmd == "cover":
            if opt["--files"]:
                files = open(opt["--files"], encoding="utf-8").read().split("\n")
            else:
                files = git(repo, "ls-tree", "-r", "--name-only", "--full-tree", opt["--tree"]).split("\n")
            files = [f for f in files if f]
            none = [f for f in files if not lanes_for(rows, f)[0]]
            for k, ln, p, exc, rx in rows:
                if k != "default" and not any(rx.match(f) for f in files):
                    print("LANES WARN: %s (%s %s) matches no file" % (p, k, ln))
            if none:
                print("LANES FAIL %d of %d files map to no lane (map %s): %s" % (len(none), len(files), src, " ".join(none)))
                return 1
            print("LANES OK %d of %d files have a lane (map %s, %s)" % (len(files), len(files), src, opt["--files"] or "tree " + opt["--tree"]))
            return 0
        tag = opt["--lane"]
        if tag not in LANES or not opt["--base"] or not opt["--head"]:
            die("lane needs --lane <%s> --base B --head H" % "|".join(LANES))
        base = opt["--base"]
        if base == "auto":   # the merge base with integration, or with smk37 when that one is newer (a docs lane)
            base = git(repo, "merge-base", opt["--tree"], opt["--head"]).strip()
            try:
                md = git(repo, "merge-base", opt["--map-ref"], opt["--head"]).strip()
                if md != base and subprocess.run(["git", "--git-dir", repo, "merge-base", "--is-ancestor", base, md]).returncode == 0:
                    base = md
            except RuntimeError:
                pass
        ch = [f for f in git(repo, "diff", "--name-only", "--no-renames", base, opt["--head"]).split("\n") if f]
        bad, nolane, shared = [], [], 0
        for f in ch:
            ls, why = lanes_for(rows, f)
            if not ls:
                nolane.append(f)
            elif tag not in ls:
                bad.append("%s (%s)" % (f, " ".join(ls)))
            elif len(ls) > 1:
                shared += 1
        if bad:
            print("LANE REFUSED lane/%s: files of another lane only (map %s): %s" % (tag, src, " ".join(bad[:20]) + (" ..." if len(bad) > 20 else "")))
            return 1
        print("LANE OK lane/%s: %d changed files, %d shared, %d with no lane%s (map %s)" % (
            tag, len(ch), shared, len(nolane), (": " + " ".join(nolane[:10])) if nolane else "", src))
        return 0
    except (RuntimeError, OSError) as e:
        die(str(e))


if __name__ == "__main__":
    sys.exit(main(sys.argv))
