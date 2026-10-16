#!/usr/bin/env python3
# Source: /workspace/x0x37-orchestrator/ste_check.py (Orchestrator), copied 2026-10-16, sha256 8e78d6a9b4bd. No other change.
"""ste_check.py: measurable ASD-STE100 checks for x0x37 docs, skills and commit messages.

Usage:
  ste_check.py FILE [FILE ...]            check Markdown or text files
  ste_check.py --git REPO REV N           check the last N commit messages of REV
  ste_check.py --from-git REPO REV FILE   check FILE as it is in git revision REV
  ste_check.py --rule passive FILE        print each sentence that breaks one check
  options: --examples K (worst sentences per file), --json, --strict

Checks (counted as violations, exit code 1 if any count > 0):
  long_proc   procedural (imperative) sentence > 20 words
  long_desc   descriptive sentence > 25 words
  contraction it's, don't, can't, ... (possessive 's is not counted)
  semicolon   ';' in prose (inline code and code blocks are skipped)
  modal       may, might, should, would, could, shall
  passive     is/are/was/were/be/been/being + past participle
  unapproved  words from UNAPPROVED (heuristic list, see below)
  long_para   paragraph or list item with more than 6 sentences
Review counts (reported, counted as violations only with --strict):
  perfect     has/have/had + past participle (not a simple tense)
  progressive is/are/was/were + -ing
  ing         other -ing words not in ING_OK (many are allowed nouns; check by hand)
  idiom       phrases from IDIOMS
  no_verb     sentence of 3+ words with no verb from a small lexicon: a sign of telegraph style
  review_word filler words and words to check by hand (REVIEW_WORDS)
  Text in "double quotes" is a mention or an example: word checks skip it (--keep-quotes to check it).
Limits of the method: sentence split and the imperative test are heuristics.
Technical names in `code` are skipped. A word count includes hashes and numbers.
"""
import argparse, json, re, subprocess, sys

SHOW_RULE = None
KEEP_QUOTES = False  # text in "double quotes" is a mention or example; word checks skip it

# Hard list: words named in asd-ste SKILL.md Words 1, words named in the audit task, and
# common words that the STE100 dictionary lists as not approved, with the approved form.
UNAPPROVED = {
    "ensure": "make sure", "ensures": "makes sure", "ensured": "made sure",
    "utilize": "use", "utilise": "use", "utilizes": "uses", "utilized": "used",
    "initiate": "start", "initiates": "starts", "initiated": "started",
    "commence": "start", "commences": "starts",
    "terminate": "stop", "terminates": "stops", "terminated": "stopped",
    "indicate": "show", "indicates": "shows", "indicated": "showed",
    "perform": "do", "performs": "does", "performed": "did",
    "via": "through / with", "prior": "before", "subsequent": "next / after",
    "subsequently": "then", "facilitate": "help", "leverage": "use",
    "approximately": "about", "additional": "more", "additionally": "also",
    "numerous": "many", "obtain": "get", "obtains": "gets",
    "assist": "help", "sufficient": "enough",
    "etc": "(list all items)", "e.g": "for example", "i.e": "that is", "vs": "and / compared with",
}
# Soft list: filler words and words to check by hand (approval status not checked here).
REVIEW_WORDS = {"just", "basically", "simply", "actually", "really", "please",
                "verify", "verified", "verifies", "handle", "handles", "enforce", "enforces", "enforced"}
MODALS = {"may", "might", "should", "would", "could", "shall"}
IRREG_PP = set("""done made written built given taken shown sent kept held set put found
cut lost left seen broken chosen known drawn begun split synced fed led met paid said sold
told thought brought bought caught taught meant hidden run stuck struck spent""".split())
NOT_PP = set("""need needed? speed feed seed indeed bed red shed hundred naked wicked sacred
embed""".replace("?", "").split())
ING_OK = set("""thing things nothing something anything everything string strings bring brings
ring rings during morning evening ceiling spring swing sing wing sting ping king bring
warning warnings setting settings building ending meaning helping working pending""".split())
# 'warning', 'setting', 'building' are allowed nouns here only as technical names; listed
# so the 'ing' review count shows verb uses first.
IDIOMS = ["drawing board", "at once", "keep caches warm", "keeps caches warm", "red-flag",
          "big beat", "go straight to", "out of the box", "on the fly", "in a row",
          "under the hood", "make noise", "for now"]
IMPERATIVE = set("""use write put make run fetch change give install back send tell read
compare report log mark build name get stop never do ask decide keep update find test accept
split commit start check record go leave add let see search create point follow attach drop
post obey merge push copy sync bring turn capture explain talk lead optimize retry notify
strip touch delete open set select describe quote cut sort show save mirror register offer
remove type press enter hold wait fix measure prove move call skip list clear verify
include describe pick split bisect review package stay""".split())
VERBS = IMPERATIVE | set("""own play work need hold come take show reach go mean learn lose help agree
apply count act appear belong cover pass fail print exist latch light clear dim wrap return happen
open close step load store keep reset toggle mute sound filter map lift fill glide carry drop wait
follow grow shrink disappear cost refuse block allow differ jump hang end begin answer route relay
watch suggest package handle request apply say ask confirm approve replace become seem win share
edit trim conflict break""".split())
AUX = set("is are was were be been am has have had do does did can must will cannot isn't aren't doesn't don't won't".split())


def has_verb(s):
    for t in re.findall(r"[A-Za-z][A-Za-z'’]*", s):
        l = t.lower()
        if l in AUX or l in VERBS:
            return True
        for suf, add in (("ies", "y"), ("es", ""), ("s", ""), ("ed", ""), ("ed", "e"), ("d", ""), ("ied", "y")):
            if l.endswith(suf) and (l[: -len(suf)] + add) in VERBS:
                return True
        if l in IRREG_PP or l in {"ran", "gave", "took", "went", "came", "saw", "wrote", "held", "knew", "began", "got"}:
            return True
    return False


COND = re.compile(r"^(if|when|before|after|while|until|with|in|on|for|at|to|once|unless)\b[^,]{0,120},\s*", re.I)
WORD = re.compile(r"[A-Za-z0-9][A-Za-z0-9'’\-_/.:+#@%]*")


def strip_md(text, is_commits=False):
    """Return list of (lineno, segment_text) prose segments: paragraphs, list items, table cells."""
    lines = text.split("\n")
    segs, buf, start = [], [], 0
    in_code = False
    in_fm = lines and lines[0].strip() == "---"
    fm_done = not in_fm

    def flush():
        nonlocal buf
        if buf:
            segs.append((start, " ".join(buf)))
        buf = []

    for i, raw in enumerate(lines, 1):
        line = raw.rstrip()
        if not fm_done:
            if i > 1 and line.strip() == "---":
                fm_done = True
                flush()
                continue
            if i == 1:
                continue
            m = re.match(r"^\s*(name|description):\s*(>-)?\s*(.*)$", line)
            if m and m.group(1) == "name":
                continue
            txt = m.group(3) if m else line.strip()
            if txt:
                if not buf:
                    start = i
                buf.append(txt)
            continue
        if line.strip().startswith("```"):
            in_code = not in_code
            flush()
            continue
        if in_code:
            continue
        s = line.strip()
        if not s:
            flush(); continue
        if s.startswith("#") and not is_commits:  # heading: not a sentence
            flush(); continue
        if s.startswith("|"):
            flush()
            if re.match(r"^\|[\s\-:|]+\|?$", s):
                continue
            for cell in s.strip("|").split("|"):
                if cell.strip():
                    segs.append((i, "\x00" + cell.strip()))  # \x00 marks a table cell
            continue
        if s.startswith("[![") or s.startswith("<"):
            flush(); continue
        m = re.match(r"^([-*+]|\d+[a-z]?\.)\s+(.*)$", s)
        if not m and re.match(r"^[A-Z]{2,4}-\d+[a-z]?\.\s", s):  # BEHAVIOR.md item: own paragraph
            m = re.match(r"^()(.*)$", s)
        if m:
            flush(); start = i; buf = [m.group(2)]; continue
        if s.startswith(">"):
            s = s.lstrip("> ").strip()
        if not buf:
            start = i
        buf.append(s)
    flush()
    return segs


def clean(seg):
    seg = re.sub(r"`[^`]*`", "CODE", seg)
    seg = re.sub(r"!\[[^\]]*\]\([^)]*\)", "", seg)
    seg = re.sub(r"\[([^\]]*)\]\([^)]*\)", r"\1", seg)
    seg = re.sub(r"<https?://[^>]*>", "URL", seg)
    seg = re.sub(r"https?://\S+", "URL", seg)
    seg = re.sub(r"\*\*|__", "", seg)
    return seg.strip()


ABBR = re.compile(r"\b(e\.g|i\.e|etc|vs|approx|no|cf|Fig|ca)\.$", re.I)


def sentences(seg):
    out, cur = [], ""
    parts = re.split(r"(?<=[.!?])(\s+)", seg)
    for p in parts:
        if not p.strip():
            cur += p; continue
        cur += p
        if re.search(r"[.!?][\"')\]]*$", cur) and not ABBR.search(cur.rstrip("\"')]")):
            out.append(cur.strip()); cur = ""
    if cur.strip():
        out.append(cur.strip())
    merged = []
    for x in out:
        if merged and re.fullmatch(r"(\[[^\]]*\]\s*)+[.]?", x):  # source tags like [MANUAL §7]
            merged[-1] += " " + x
        elif merged and re.fullmatch(r"[A-Z]{2,4}-\d+[a-z]?\.|\d+[a-z]?\.", merged[-1]):  # item ID
            merged[-1] = x
        else:
            merged.append(x)
    return [s for s in merged if WORD.search(s)]


def words(s):
    return [w for w in WORD.findall(s) if w != "CODE" or True]


def is_procedural(s):
    t = re.sub(r"^[A-Za-z0-9 ./()'-]{1,30}:\s+", "", s)  # leading label like "Install:"
    t = COND.sub("", t)
    t = re.sub(r"^(also|only|then|first|always)\s+", "", t, flags=re.I)
    w = WORD.findall(t)
    if not w:
        return False
    if re.match(r"(Merge Integrator|Build and Test Runner|Release Packager|Test Plan Writer|Second Builder|Firmware Main Builder|Docs Keeper)\b", t):
        return False  # agent name, not an imperative verb
    f = w[0].lower().strip("'’")
    if f in ("do", "don't", "never") or f in IMPERATIVE:
        # "Do" + noun can be descriptive, but rare in these files
        if f in ("back",) and len(w) > 1 and w[1].lower() != "up":
            return False
        return True
    return False


def check_sentence(s):
    s = re.sub(r"\[(CODE|TEST|MANUAL|SMK37|APC|PADMAP|DESIGN|USER)[^\]]*\]", "", s).strip()
    s = re.sub(r"^[A-Z]{2,4}-\d+[a-z]?\.\s+", "", s)
    v = {}
    q = s if KEEP_QUOTES else re.sub(r'"[^"]*"|“[^”]*”', "Q", s)
    low = q.lower().replace("’", "'")
    toks = [t.lower().strip(".,:()\"'") for t in re.findall(r"[A-Za-z][A-Za-z'’\-.]*", q)]
    n = len(WORD.findall(s))
    proc = is_procedural(s)
    if proc and n > 20:
        v["long_proc"] = 1
    if not proc and n > 25:
        v["long_desc"] = 1
    c = len(re.findall(r"\b\w+n't\b|\b\w+'(re|ve|ll|d|m)\b|\b(it|that|there|what|here|let|he|she|who)'s\b", low))
    if c: v["contraction"] = c
    if ";" in q: v["semicolon"] = q.count(";")
    m = sum(1 for t in toks if t in MODALS)
    if m: v["modal"] = m
    pas = 0; perf = 0; prog = 0
    for a, b, c2 in re.findall(r"\b(is|are|was|were|be|been|being|am|isn't|aren't|wasn't|weren't|has|have|had)\s+(?:not\s+|also\s+|only\s+|now\s+|still\s+|already\s+|always\s+|never\s+|\w+ly\s+)?(([a-z]+ed)|[a-z]+)\b", low):
        bw = b
        pp = (bw.endswith("ed") and bw not in NOT_PP and len(bw) > 3) or bw in IRREG_PP
        if a in ("has", "have", "had"):
            if pp: perf += 1
        else:
            if pp: pas += 1
            elif bw.endswith("ing") and bw not in ING_OK and a not in ("be", "been"):
                prog += 1
    if pas: v["passive"] = pas
    if perf: v["perfect"] = perf
    if prog: v["progressive"] = prog
    un = [t for t in toks if t.rstrip(".") in UNAPPROVED]
    if un: v["unapproved"] = len(un)
    rw = [t for t in toks if t.rstrip(".") in REVIEW_WORDS]
    if rw: v["review_word"] = len(rw)
    ing = [t for t in toks if t.endswith("ing") and len(t) > 4 and t not in ING_OK and "-" not in t]
    if ing: v["ing"] = len(ing)
    if n >= 3 and not has_verb(q):
        v["no_verb"] = 1
    idi = sum(low.count(p) for p in IDIOMS)
    if idi: v["idiom"] = idi
    return v, n, proc, un


HARD = ["long_proc", "long_desc", "contraction", "semicolon", "modal", "passive", "unapproved", "long_para"]
SOFT = ["perfect", "progressive", "ing", "idiom", "review_word", "no_verb"]


def analyse(name, text, is_commits=False):
    segs = strip_md(text, is_commits)
    counts = {k: 0 for k in HARD + SOFT}
    ex, nsent, nwords, words_found = [], 0, 0, {}
    for ln, seg in segs:
        cell = seg.startswith("\x00")
        ss = sentences(clean(seg.lstrip("\x00")))
        if len(ss) > 6:
            counts["long_para"] += 1
            if SHOW_RULE == "long_para":
                print(f"{name}:{ln} [long_para {len(ss)} sentences] {seg[:200]}")
        for s in ss:
            v, n, proc, un = check_sentence(s)
            if "no_verb" in v and (cell or re.match(r"^[\w /&().-]{1,30}:\s+\S+(\s+\S+){0,3}\.?$", s)):
                del v["no_verb"]  # table cell or 'Label: short value' data line
            nsent += 1; nwords += n
            for k, c in v.items():
                counts[k] += c
            if SHOW_RULE and SHOW_RULE in v:
                print(f"{name}:{ln} [{SHOW_RULE}] {s}")
            for u in un:
                words_found[u] = words_found.get(u, 0) + 1
            score = sum(v.get(k, 0) for k in HARD) + max(0, n - (20 if proc else 25)) / 5
            if any(k in v for k in HARD):
                ex.append((score, ln, n, "proc" if proc else "desc", sorted(v), s))
    ex.sort(key=lambda e: -e[0])
    return {"file": name, "sentences": nsent, "words": nwords, "counts": counts,
            "unapproved_found": words_found, "worst": ex}


def git_commits(repo, rev, n):
    out = subprocess.run(["git", "-C", repo, "log", f"-{n}", "--format=%h%n%B%n@@END@@", rev],
                         capture_output=True, text=True, check=True).stdout
    return out


def commits_text(raw):
    # each commit: drop hash line, treat subject + body lines as paragraphs; add '.' to subject if absent
    out = []
    for block in raw.split("@@END@@"):
        lines = [l for l in block.strip().split("\n")]
        if not lines or not lines[0].strip():
            continue
        body = lines[1:]
        if body:
            subj = body[0].strip()
            subj = re.sub(r"^[A-Za-z0-9 _-]{1,25}:\s+", "", subj)  # area prefix 'docs:'
            if not re.search(r"[.!?]$", subj):
                subj += "."
            out.append(subj)
            out.append("")
            out.extend(body[1:])
            out.append("")
    return "\n".join(out)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("files", nargs="*")
    ap.add_argument("--git", nargs=3, metavar=("REPO", "REV", "N"), action="append")
    ap.add_argument("--from-git", nargs=2, metavar=("REPO", "REV"),
                    help="read FILE paths from this git revision (git show REV:FILE), not from disk")
    ap.add_argument("--examples", type=int, default=0)
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--strict", action="store_true")
    ap.add_argument("--keep-quotes", action="store_true", help="also check words inside double quotes")
    ap.add_argument("--rule", help="print each sentence that breaks this check (for example passive)")
    a = ap.parse_args()
    global KEEP_QUOTES, SHOW_RULE
    KEEP_QUOTES = a.keep_quotes
    SHOW_RULE = a.rule
    results = []
    for f in a.files:
        if a.from_git:
            txt = subprocess.run(["git", "-C", a.from_git[0], "show", f"{a.from_git[1]}:{f}"],
                                 capture_output=True, text=True, check=True).stdout
            f = f"{a.from_git[1]}:{f}"
        else:
            txt = open(f, encoding="utf-8", errors="replace").read()
        if "@@END@@" in txt:
            results.append(analyse(f, commits_text(txt), True))
        else:
            results.append(analyse(f, txt))
    for repo, rev, n in a.git or []:
        results.append(analyse(f"git:{rev}~{n}", commits_text(git_commits(repo, rev, int(n))), True))
    if not results:
        ap.error("give at least one file or --git")
    keys = HARD + (SOFT if a.strict else [])
    bad = False
    if a.json:
        for r in results:
            r["worst"] = [dict(zip(["score", "line", "words", "type", "rules", "text"], e)) for e in r["worst"][:a.examples]]
        print(json.dumps(results, indent=1))
    else:
        cols = HARD + SOFT
        print("file".ljust(34) + "sent  " + " ".join(c[:11].rjust(11) for c in cols))
        for r in results:
            print(r["file"][-34:].ljust(34) + str(r["sentences"]).ljust(6) +
                  " ".join(str(r["counts"][c]).rjust(11) for c in cols))
        for r in results:
            if r["unapproved_found"]:
                print(f"{r['file']}: unapproved words: " + ", ".join(f"{k} x{v}" for k, v in sorted(r['unapproved_found'].items(), key=lambda x: -x[1])))
            for e in r["worst"][:a.examples]:
                print(f"  {r['file']}:{e[1]} [{e[3]} {e[2]}w {','.join(e[4])}] {e[5][:300]}")
    for r in results:
        if any(r["counts"][k] for k in keys):
            bad = True
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
