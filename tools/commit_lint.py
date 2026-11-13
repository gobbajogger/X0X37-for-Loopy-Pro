#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""commit_lint.py: commit checks 19, 20, 21, 22 and 24 of rules v4 (tools/quickcheck.sh runs it, commit step).

  commit_lint.py REPO BASE HEAD      REPO = a git worktree or bare repository; each commit in BASE..HEAD

Output: one line for each finding, "COMMIT FAIL <hash7>: ..." or "COMMIT WARN <hash7>: ...", then
"COMMIT <n> FAIL, <m> WARN, <k> commits". Exit code 1 if a FAIL, 0 if not (a WARN does not change it), 2 on an error.
  20 (rule 9, FAIL): the author of each commit (merges too) must be AUTHOR (genkigenki <3080081+...>).
  19 (rule 9, WARN): a commit (not a merge) changes a path with a folder "src/" and no file in tests/.
  21 (rule 12, WARN): a line that a commit adds to a *.md file has a time "YYYY-MM-DD HH:MM" (with an optional zone
      "+10", "+1000", "(+10)") later than the commit time (the commit zone if the text has no zone).
  22 (rule 12, WARN): a commit message (not a merge) names a BEHAVIOR.md ID and the commit does not change BEHAVIOR.md.
      An ID = <PREFIX>-<number>, where <PREFIX> is the prefix of an item line ("CLP-18. ...") of BEHAVIOR.md in the commit.
  24 (rule 14, WARN): a commit changes (adds, removes or edits) an item line of BEHAVIOR.md (not USE-: the rules of the
      file itself) and the message has no owner decision time ("owner ... HH:MM" in one line, for example "owner OK 02:32").
"""
import datetime
import re
import subprocess
import sys

AUTHOR = "genkigenki <3080081+genkigenki@users.noreply.github.com>"
ITEM = re.compile(r"^(?:- )?([A-Z]{2,5})-(\d+[a-z]?)\.\s")
TIME = re.compile(r"\b(\d{4})-(\d{2})-(\d{2})[ T](\d{2}):(\d{2})(?::\d{2})?(?:\s*\(?([+-])(\d{2}):?(\d{2})?\)?)?")
OWNER_TIME = re.compile(r"owner[^\n]{0,60}?\b\d{1,2}:\d{2}\b", re.I)


def git(repo, *a, ok=False):
    r = subprocess.run(["git", "-C", repo] + list(a), capture_output=True, text=True, errors="replace")
    if r.returncode and not ok:
        raise RuntimeError("git %s: %s" % (" ".join(a), r.stderr.strip()[:200]))
    return r.stdout if r.returncode == 0 else ""


def items(text):
    ids, pre = set(), set()
    for l in text.splitlines():
        m = ITEM.match(l)
        if m:
            ids.add("%s-%s" % m.groups()); pre.add(m.group(1))
    return ids, pre


def check(repo, c):
    out = []
    meta = git(repo, "log", "-1", "--format=%an <%ae>%x00%P%x00%cI%x00%B", c).split("\x00", 3)
    author, parents, ctime, msg = meta[0], meta[1].split(), meta[2], meta[3]
    h = c[:7]
    if author != AUTHOR:
        out.append(("FAIL", "%s: author %s, must be %s (rule 9)" % (h, author, AUTHOR)))
    if len(parents) != 1:
        return out
    files = git(repo, "diff-tree", "--no-commit-id", "-r", "--name-only", c).split()
    if any(re.search(r"(^|/)src/", f) for f in files) and not any(f.startswith("tests/") for f in files):
        out.append(("WARN", "%s: changes src/ and no tests/ file (rule 9: one host test for each change)" % h))
    ct = datetime.datetime.fromisoformat(ctime)
    for f in files:
        if not f.endswith(".md"):
            continue
        for l in git(repo, "diff", "-U0", c + "^", c, "--", f).splitlines():
            if not l.startswith("+") or l.startswith("+++"):
                continue
            for m in TIME.finditer(l):
                y, mo, d, hh, mm, sg, zh, zm = m.groups()
                tz = ct.tzinfo if not sg else datetime.timezone(
                    (1 if sg == "+" else -1) * datetime.timedelta(hours=int(zh), minutes=int(zm or 0)))
                try:
                    t = datetime.datetime(int(y), int(mo), int(d), int(hh), int(mm), tzinfo=tz)
                except ValueError:
                    continue
                if t > ct.replace(second=0, microsecond=0):
                    out.append(("WARN", "%s: %s names %s, later than the commit time %s (rule 12: do not invent data)"
                                % (h, f, m.group(0).strip(), ct.strftime("%Y-%m-%d %H:%M %z"))))
    bnew = git(repo, "show", c + ":BEHAVIOR.md", ok=True)
    bold = git(repo, "show", c + "^:BEHAVIOR.md", ok=True)
    ids_new, pre = items(bnew)
    named = sorted(set(i for i in re.findall(r"\b([A-Z]{2,5}-\d+[a-z]?)\b", msg) if i.split("-")[0] in pre))
    if named and "BEHAVIOR.md" not in files:
        out.append(("WARN", "%s: the message names %s and the commit does not change BEHAVIOR.md (rule 12)" % (h, " ".join(named))))
    if "BEHAVIOR.md" in files:
        lo = set(l for l in bold.splitlines() if ITEM.match(l))
        ln = set(l for l in bnew.splitlines() if ITEM.match(l))
        ch = sorted(set("%s-%s" % ITEM.match(l).groups() for l in lo ^ ln) - set(i for i in ids_new if i.startswith("USE-")))
        ch = [i for i in ch if not i.startswith("USE-")]
        if ch and not OWNER_TIME.search(msg):
            out.append(("WARN", "%s: changes BEHAVIOR.md items %s and the message has no owner decision time (rule 14: \"owner OK HH:MM\")"
                        % (h, " ".join(ch[:8]) + (" ..." if len(ch) > 8 else ""))))
    return out


def main(argv):
    if len(argv) != 4:
        print(__doc__, file=sys.stderr); return 2
    repo, base, head = argv[1:]
    try:
        cs = git(repo, "rev-list", "--reverse", "%s..%s" % (base, head)).split()
        res = [r for c in cs for r in check(repo, c)]
    except (RuntimeError, OSError) as e:
        print("COMMIT ERROR: %s" % e); return 2
    for k, t in res:
        print("COMMIT %s %s" % (k, t))
    nf = sum(1 for k, _ in res if k == "FAIL")
    print("COMMIT %d FAIL, %d WARN, %d commits" % (nf, len(res) - nf, len(cs)))
    return 1 if nf else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
