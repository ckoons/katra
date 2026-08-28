#!/usr/bin/env python3
"""memory_search — query the katra memory corpus instead of loading an index of it.

The memory directory is already a database: one fact per file, each carrying
frontmatter with `name`, `description` and `metadata.type`. MEMORY.md is a
hand-maintained catalog over that database, and a catalog has two problems the
database does not — it is loaded whole into every session (so it has a size
ceiling), and it drifts (entries go stale, duplicate, or are never added at all).

This tool reads the database directly. Nothing to maintain, no ceiling.

    memory_search.py sweep negative          # rank by name/description/body
    memory_search.py --type feedback clock   # restrict to one memory type
    memory_search.py --orphans               # files MEMORY.md does not link
    memory_search.py --stats                 # corpus shape
    memory_search.py --show <name>           # print one memory whole

Scoring is deliberately dumb: a name hit outranks a description hit outranks a
body hit. If that ever stops being good enough, the fix is a better ranker, not
a bigger index.

Author: Keeper, 2026-08-28. Built after the index went over its read limit and
the last four entries began loading invisibly.
"""

import argparse
import os
import sys

DEFAULT_DIR = os.path.expanduser(
    "~/.claude/projects/-Users-cskoons-projects-github/memory"
)

W_NAME, W_DESC, W_BODY = 10, 4, 1


def parse(path):
    """Return (name, description, type, body). Missing fields come back empty."""
    try:
        with open(path, encoding="utf-8") as fh:
            text = fh.read()
    except OSError:
        return None

    name = desc = mtype = ""
    body = text
    if text.startswith("---"):
        end = text.find("\n---", 3)
        if end != -1:
            front, body = text[3:end], text[end + 4:]
            for line in front.splitlines():
                if line.startswith("name:"):
                    name = line[5:].strip()
                elif line.startswith("description:"):
                    desc = line[12:].strip()
                elif line.strip().startswith("type:"):
                    mtype = line.split("type:", 1)[1].strip()
    if not name:
        name = os.path.basename(path)[:-3]
    return name, desc, mtype, body


def load(mem_dir):
    out = []
    for fn in sorted(os.listdir(mem_dir)):
        if not fn.endswith(".md") or fn == "MEMORY.md" or ".bak-" in fn:
            continue
        rec = parse(os.path.join(mem_dir, fn))
        if rec:
            out.append((fn,) + rec)
    return out


def score(rec, terms):
    _fn, name, desc, _mtype, body = rec
    name_l, desc_l, body_l = name.lower(), desc.lower(), body.lower()
    total = 0
    for t in terms:
        hit = 0
        if t in name_l:
            hit += W_NAME
        if t in desc_l:
            hit += W_DESC
        if t in body_l:
            hit += W_BODY
        if hit == 0:
            return 0  # every term must appear somewhere
        total += hit
    return total


def main():
    ap = argparse.ArgumentParser(add_help=True)
    ap.add_argument("terms", nargs="*", help="search terms (all must match)")
    ap.add_argument("--dir", default=DEFAULT_DIR)
    ap.add_argument("--type", dest="mtype", help="user | feedback | project | reference")
    ap.add_argument("-n", type=int, default=12, help="max results")
    ap.add_argument("--orphans", action="store_true",
                    help="memories MEMORY.md does not link")
    ap.add_argument("--stats", action="store_true")
    ap.add_argument("--show", help="print one memory whole, by name or filename")
    args = ap.parse_args()

    if not os.path.isdir(args.dir):
        sys.exit(f"no memory directory at {args.dir}")
    records = load(args.dir)

    if args.stats:
        by_type = {}
        for r in records:
            by_type[r[3] or "(none)"] = by_type.get(r[3] or "(none)", 0) + 1
        idx = os.path.join(args.dir, "MEMORY.md")
        idx_sz = os.path.getsize(idx) if os.path.exists(idx) else 0
        print(f"memories: {len(records)}")
        for k in sorted(by_type):
            print(f"  {k:<12} {by_type[k]}")
        print(f"index MEMORY.md: {idx_sz} bytes "
              f"({'OVER' if idx_sz > 24400 else 'under'} the 24.4KB read limit)")
        return

    if args.orphans:
        idx = os.path.join(args.dir, "MEMORY.md")
        text = open(idx, encoding="utf-8").read() if os.path.exists(idx) else ""
        missing = [r for r in records if f"({r[0]})" not in text]
        print(f"{len(missing)} of {len(records)} memories are NOT linked from MEMORY.md\n")
        for fn, name, desc, mtype, _ in missing:
            print(f"  [{mtype or '?':<9}] {fn}")
            if desc:
                print(f"              {desc[:100]}")
        return

    if args.show:
        key = args.show.lower()
        for fn, name, _d, _t, _b in records:
            if key in (fn.lower(), name.lower(), fn[:-3].lower()):
                print(open(os.path.join(args.dir, fn), encoding="utf-8").read())
                return
        sys.exit(f"no memory matching {args.show!r}")

    if not args.terms:
        ap.print_help()
        return

    terms = [t.lower() for t in args.terms]
    hits = []
    for r in records:
        if args.mtype and r[3] != args.mtype:
            continue
        s = score(r, terms)
        if s:
            hits.append((s, r))
    hits.sort(key=lambda x: (-x[0], x[1][0]))

    if not hits:
        print(f"no memory matches {' '.join(args.terms)!r} "
              f"(searched {len(records)} files)")
        return

    print(f"{len(hits)} match{'es' if len(hits) != 1 else ''} "
          f"of {len(records)} memories\n")
    for s, (fn, name, desc, mtype, _b) in hits[:args.n]:
        print(f"[{s:>3}] [{mtype or '?'}] {name}")
        if desc:
            print(f"      {desc}")
        print(f"      --show {fn[:-3]}\n")
    if len(hits) > args.n:
        print(f"... {len(hits) - args.n} more (raise with -n)")


if __name__ == "__main__":
    main()
