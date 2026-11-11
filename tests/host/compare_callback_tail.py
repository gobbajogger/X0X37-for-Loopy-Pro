#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Reject PCM/control divergence in exact-source before/after host callback probes.

A change in p99 or maximum host time is reported, never asserted as a portable
real-time device safety threshold. Require proper experimental analysis.
"""
import argparse
import json
import statistics
from pathlib import Path


def read(path):
    records = {}
    for line in Path(path).read_text().splitlines():
        parts = dict(token.split("=", 1) for token in line.split())
        key = (int(parts["n"]), parts["direction"], int(parts["rate"]), int(parts["trial"]))
        if key in records:
            raise ValueError(f"duplicate key {key} in {path}")
        if parts["stopped_silent"] != "1" or int(parts["audible"]) < 1:
            raise ValueError(f"invalid behavior {key} in {path}")
        for name in ("p50", "p95", "p99", "max", "sum"):
            if int(parts[name]) < 1:
                raise ValueError(f"invalid {name}={parts[name]}")
        if int(parts["p50"]) > int(parts["p95"]) or int(parts["p95"]) > int(parts["p99"]) or int(parts["p99"]) > int(parts["max"]):
            raise ValueError(f"invalid quantiles {key}")
        records[key] = parts
    if len(records) != 76:
        # 65535 has 9 valid rates, 2116800 has 10, each × 2 directions × 2 trials.
        raise ValueError(f"missing/extra scenarios: {path} {len(records)} != 76")
    return records


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("baseline")
    parser.add_argument("optimized")
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    old = read(args.baseline)
    new = read(args.optimized)
    if old.keys() != new.keys():
        raise ValueError("workload key sets differ")
    summary = {}
    for (n, direction, rate, trial), before in old.items():
        after = new[(n, direction, rate, trial)]
        for field in ("hash", "audible", "frames", "blocks", "stopped_silent"):
            if before[field] != after[field]:
                raise ValueError(f"PCM/control mismatch {(n,direction,rate,trial)} {field}: {before[field]} vs {after[field]}")
        k=(n,direction,rate)
        summary.setdefault(k, []).append((int(before["p99"]),int(after["p99"]),int(before["sum"]),int(after["sum"])))
    result=[]
    for (n,direction,rate),values in sorted(summary.items()):
        base_p99 = statistics.median(v[0] for v in values)
        perf_p99 = statistics.median(v[1] for v in values)
        base_sum = statistics.median(v[2] for v in values)
        perf_sum = statistics.median(v[3] for v in values)
        result.append(dict(n=n,direction=direction,rate=rate,
                           median_p99_ns_baseline=base_p99,median_p99_ns_optimized=perf_p99,
                           p99_change_pct=(perf_p99 / base_p99 - 1)*100,
                           total_change_pct=(perf_sum / base_sum - 1)*100))
    payload=dict(scenarios=len(result),paired_trials=len(old),pcm_parity=True,
                 note="64-sample native host elapsed-time proxy; not target ISR, XRUN or deadline proof",
                 findings=result)
    Path(args.output).write_text(json.dumps(payload,indent=2,sort_keys=True)+"\n")
    print("PASS scenarios=%d paired_trials=%d PCM/control parity=100%%"%(len(result),len(old)))
    for r in result:
        if r["n"]==2116800 and r["direction"]=="R":
            print("long_reverse rate=%d p99_change=%+.1f%% total_change=%+.1f%%"%(r["rate"],r["p99_change_pct"],r["total_change_pct"]))

if __name__ == "__main__":
    main()
