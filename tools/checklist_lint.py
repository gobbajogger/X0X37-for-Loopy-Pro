#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""checklist_lint.py: lint an owner test checklist (check 23 of rules v4, docs.md rule 12). Docs Keeper runs it.

Usage: checklist_lint.py FILE [FILE ...]

Checks:
  Header (the lines before the first "## " heading), each line as "Label: value" or "- Label: value":
    "Build:"                      names the batch number ("batch <N>") and the gated hash (7 to 40 hex characters)
    "Replaces:"                   not empty (the earlier checklist, or "none")
    "Checked against decisions:"  has a time HH:MM (in the line or in its indented sub-items)
  Tasks: the top-level numbered lines ("<N>. ", no indent) in the "## Tasks" section (if the file has no "## Tasks"
  section: in the whole file). 10 tasks or fewer. Each task text starts with "Goal:".
Output: "CHECKLIST OK <n> tasks <file>" (exit 0), or "CHECKLIST FAIL <file>" and one line per issue (exit 1).
"""
import re, sys

LABELS = ["Build:", "Replaces:", "Checked against decisions:"]


def lint(text):
    lines = text.split("\n")
    first = next((i for i, l in enumerate(lines) if l.startswith("## ")), len(lines))
    head, bad = lines[:first], []
    for lab in LABELS:
        idx = [i for i, l in enumerate(head) if re.match(r"^(- )?" + re.escape(lab), l.strip())]
        if not idx:
            bad.append('no header line "%s"' % lab); continue
        i = idx[0]
        val = re.sub(r"^(- )?" + re.escape(lab), "", head[i].strip()).strip()
        j = i + 1
        while j < len(head) and head[j].startswith((" ", "\t")) and head[j].strip():
            val += " " + head[j].strip(); j += 1
        if not val:
            bad.append('header line "%s" is empty' % lab)
        elif lab == "Build:":
            if not re.search(r"\bbatch \d+", val, re.I): bad.append("Build line has no batch number (batch <N>)")
            if not re.search(r"\b[0-9a-f]{7,40}\b", val): bad.append("Build line has no gated hash")
        elif lab == "Checked against decisions:" and not re.search(r"\b\d{1,2}:\d{2}\b", val):
            bad.append('"Checked against decisions" has no time (HH:MM)')
    ts = next((i for i, l in enumerate(lines) if re.match(r"^## Tasks\b", l)), None)
    body = lines[ts + 1:] if ts is not None else lines
    if ts is not None:
        end = next((i for i, l in enumerate(body) if l.startswith("## ")), len(body)); body = body[:end]
    tasks = [m for m in (re.match(r"^(\d+)\.\s+(.*)$", l) for l in body) if m]
    if len(tasks) > 10:
        bad.append("%d tasks (max 10)" % len(tasks))
    for m in tasks:
        if not m.group(2).startswith("Goal:"):
            bad.append('task %s does not start with "Goal:": %s' % (m.group(1), m.group(2)[:60]))
    return len(tasks), bad


def main():
    if len(sys.argv) < 2:
        print(__doc__); sys.exit(2)
    rc = 0
    for f in sys.argv[1:]:
        try:
            n, bad = lint(open(f, encoding="utf-8").read())
        except OSError as e:
            print("CHECKLIST ERROR %s: %s" % (f, e)); rc = 2; continue
        if bad:
            print("CHECKLIST FAIL %s" % f); [print("  " + b) for b in bad]; rc = max(rc, 1)
        else:
            print("CHECKLIST OK %d tasks %s" % (n, f))
    sys.exit(rc)


if __name__ == "__main__":
    main()
