#!/usr/bin/env python3
import json
import sys


def load(path):
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def main() -> int:
    if len(sys.argv) < 3:
        print("usage: check_bench_regression.py <baseline.json> <current.json> [threshold_percent]", file=sys.stderr)
        return 2

    baseline = load(sys.argv[1]).get("benchmarks", {})
    current = load(sys.argv[2]).get("benchmarks", {})
    threshold = float(sys.argv[3]) if len(sys.argv) > 3 else 3.0

    regressions = []
    for name, baseline_data in baseline.items():
        if name not in current:
            regressions.append(f"missing benchmark in current run: {name}")
            continue
        base_ns = float(baseline_data["ns_per_op"])
        cur_ns = float(current[name]["ns_per_op"])
        if base_ns <= 0:
            continue
        delta_percent = ((cur_ns - base_ns) / base_ns) * 100.0
        if delta_percent > threshold:
            regressions.append(
                f"{name}: regression {delta_percent:.2f}% (baseline {base_ns:.1f} ns/op, current {cur_ns:.1f} ns/op)"
            )

    if regressions:
        print("Performance regression detected:")
        for item in regressions:
            print(f"- {item}")
        return 1

    print(f"No regressions above {threshold:.2f}% threshold.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
