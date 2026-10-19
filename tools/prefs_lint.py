#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""prefs_lint.py: each owner-rule entry of USER_PREFERENCES.md must name a rule that exists in docs/rules/,
and that rule must quote or cover the entry (check 27 of rules v4, owner 02:37: rules = code).

Usage:
  prefs_lint.py [--rules DIR] FILE        FILE: USER_PREFERENCES.md, DIR: the rule files [FILE folder/rules]
  prefs_lint.py --git REPO REV            REV:docs/USER_PREFERENCES.md and REV:docs/rules/*.md
  options: --json, --sections "A,B" (the sections with owner rules, default below), --min-words N [3]

Only the current record before "## Historical preference record" is checked.
Older entries remain intact as evidence and do not reactivate superseded procedures.

Owner-rule entry: a top-level list item ("- ") that starts with a date (YYYY-MM-DD), in one of the sections
"How to work with the owner", "Installs and safety", "Repositories and remotes", "Team rules". Its indented
sub-items are part of the entry. The other sections hold product decisions (BEHAVIOR.md), not agent rules.
Rule reference (the "step" of the entry), in the entry text:
  "<file>.md rule <ID>"   the rule <ID> in docs/rules/<file>.md
  "core rule <ID>"        the numbered line <ID>. of docs/rules/core.md
  "agent rule <ID>"       the rule <ID> in any rule file ("### Rule <ID>" or a core line "<ID>.")
  A list is allowed: "agent rules 6 and 9 to 11", "rules 8, 13". <ID> is a number with an optional capital letter.
  "owner rule 01:57" is a time, not a rule reference.
A reference is valid if the rule exists. The entry is covered if one valid rule quotes the time of the entry
(HH:MM) or shares at least N content words (4 letters or more, not common words) with the entry.
Output: "PREFS OK <n> entries" (exit 0), or "PREFS FAIL <k> of <n> entries" and one line per entry (exit 1):
  "NO STEP <line> <time>: <text>"       no valid rule reference (a hint names a rule that quotes the time)
  "NOT COVERED <line> <time>: <rules>"  valid rules, but none quotes or covers the entry
Bad arguments or files: exit 2.
"""
import argparse, glob, json, os, re, subprocess, sys

SECTIONS = ["How to work with the owner", "Installs and safety", "Repositories and remotes", "Team rules"]
STOP = set("""this that with from have when then than they them their there these those each only also into
over under what which while will must does done make made more most some such very about after before other
owner owners agent agents rule rules entry file files line lines same does just like your yours""".split())
ID = r"\d+[A-Z]?"
LIST = rf"{ID}(?:\s*(?:,|and|to|or)\s*{ID})*"
REF = re.compile(rf"(?:\b([\w-]+\.md)\s+|\b(core|agent)\s+)rules?\s+({LIST})\b")


def parse_rules(files):
    """files: {name: text} -> {(name, id): section text}"""
    rules = {}
    for name, txt in files.items():
        if name == "core.md":
            for m in re.finditer(rf"^({ID})\.\s+(.*)$", txt, re.M):
                rules[(name, m.group(1))] = m.group(2)
        parts = re.split(r"^### ", txt, flags=re.M)
        for p in parts[1:]:
            m = re.match(rf"Rule ({ID})\b", p)
            if m:
                rules[(name, m.group(1))] = p
    return rules


def expand(lst):
    ids, toks = [], re.findall(rf"{ID}|to", lst)
    i = 0
    while i < len(toks):
        if toks[i] == "to":
            i += 1; continue
        if i + 2 < len(toks) and toks[i + 1] == "to" and toks[i].isdigit() and toks[i + 2].isdigit():
            ids += [str(x) for x in range(int(toks[i]), int(toks[i + 2]) + 1)]; i += 3
        else:
            ids.append(toks[i]); i += 1
    return ids


def entries(text, sections):
    out, sec, cur = [], None, None
    for ln, line in enumerate(text.split("\n"), 1):
        # The archived preference record is evidence, not current instructions.
        if line.startswith("## Historical preference record"):
            break
        if line.startswith("## "):
            sec = line[3:].strip(); cur = None; continue
        if sec not in sections:
            cur = None; continue
        m = re.match(r"^- (\d{4}-\d{2}-\d{2})(?:\s+(\d{1,2}:\d{2}|\d{1,2}:\dx))?", line)
        if m:
            cur = {"line": ln, "date": m.group(1), "time": m.group(2) or "", "text": line[2:]}; out.append(cur)
        elif line.startswith("- ") or not line.strip():
            cur = None
        elif cur is not None and line.startswith(" "):
            cur["text"] += " " + line.strip()
    return out


def words(s):
    return set(w.lower().rstrip("s") for w in re.findall(r"[A-Za-z]{4,}", s) if w.lower() not in STOP)


def lint(prefs, rule_files, sections=SECTIONS, min_words=3):
    rules = parse_rules(rule_files)
    res = []
    for e in entries(prefs, sections):
        refs = []
        for m in REF.finditer(e["text"]):
            f = m.group(1) if m.group(1) else ("core.md" if m.group(2) == "core" else None)
            for i in expand(m.group(3)):
                refs += [k for k in rules if k[1] == i and (f is None or k[0] == f)]
        e["rules"] = sorted(set("%s rule %s" % k for k in refs))
        if not refs:
            e["status"] = "NO STEP"
            q = sorted("%s rule %s" % k for k in rules if e["time"] and re.search(r"\b%s\b" % re.escape(e["time"]), rules[k]))
            e["hint"] = ("; quoted by " + ", ".join(q)) if q else ""
        else:
            ew = words(e["text"])
            cov = [k for k in refs if (e["time"] and e["time"] in rules[k]) or len(ew & words(rules[k])) >= min_words]
            e["status"] = "OK" if cov else "NOT COVERED"
        res.append(e)
    return res


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("file", nargs="?")
    ap.add_argument("--rules")
    ap.add_argument("--git", nargs=2, metavar=("REPO", "REV"))
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--sections")
    ap.add_argument("--min-words", type=int, default=3)
    a = ap.parse_args()
    try:
        if a.git:
            repo, rev = a.git
            def show(p):
                return subprocess.run(["git", "-C", repo, "show", f"{rev}:{p}"], capture_output=True, text=True, check=True).stdout
            prefs = show("docs/USER_PREFERENCES.md")
            names = subprocess.run(["git", "-C", repo, "ls-tree", "--name-only", f"{rev}:docs/rules"],
                                   capture_output=True, text=True, check=True).stdout.split()
            rf = {n: show(f"docs/rules/{n}") for n in names if n.endswith(".md")}
        elif a.file:
            prefs = open(a.file, encoding="utf-8").read()
            d = a.rules or os.path.join(os.path.dirname(os.path.abspath(a.file)), "rules")
            rf = {os.path.basename(p): open(p, encoding="utf-8").read() for p in glob.glob(os.path.join(d, "*.md"))}
        else:
            ap.error("give FILE or --git REPO REV")
    except (OSError, subprocess.CalledProcessError) as ex:
        print(f"PREFS ERROR {ex}"); sys.exit(2)
    if not rf:
        print("PREFS ERROR no rule files"); sys.exit(2)
    secs = [s.strip() for s in a.sections.split(",")] if a.sections else SECTIONS
    res = lint(prefs, rf, secs, a.min_words)
    bad = [e for e in res if e["status"] != "OK"]
    if a.json:
        print(json.dumps(res, indent=1))
    elif bad:
        print(f"PREFS FAIL {len(bad)} of {len(res)} entries")
        for e in bad:
            what = e["text"][:100] + e.get("hint", "") if e["status"] == "NO STEP" else ", ".join(e["rules"])
            print(f"  {e['status']} line {e['line']} {e['date']} {e['time']}: {what}")
    else:
        print(f"PREFS OK {len(res)} entries")
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
