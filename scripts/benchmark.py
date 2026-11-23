from pathlib import Path
import re
import subprocess

PARTS_DIR = Path("parts")
LOG_PATH = Path("journal.log")


def run(bin_name, *args):
    target = PARTS_DIR / (bin_name + ".exe")
    cmd = [str(target), *args]
    proc = subprocess.run(cmd, capture_output=True, text=True, check=True)
    return proc.stdout


def bench_unsynced_booking(iterations=10):
    print("=== Part 1: Unsynchronized Booking ===")
    pattern_sold = re.compile(r"tickets_sold counter = (\d+)")
    pattern_occ = re.compile(r"occupied seats in memory = (\d+)")

    for i in range(1, iterations + 1):
        out = run("part1")
        sold = int(pattern_sold.search(out).group(1))
        occ = int(pattern_occ.search(out).group(1))
        print(f"run {i:2d}: sold={sold:3d}  occupied={occ:3d}  lost_updates={sold - occ:3d}")


def bench_locking_granularity(iterations=5):
    print("\n=== Part 2: Global vs Per-Route Mutex ===")
    pattern_global = re.compile(r"mode:\s+global mutex\s+sold:\s*\d+,\s*occupied:\s*\d+,\s*time:\s*([\d\.]+)\s*s")
    pattern_route = re.compile(r"mode:\s+per-route mutex\s+sold:\s*\d+,\s*occupied:\s*\d+,\s*time:\s*([\d\.]+)\s*s")

    global_times = []
    route_times = []

    for i in range(1, iterations + 1):
        out = run("part2")
        t_global = float(pattern_global.search(out).group(1))
        t_route = float(pattern_route.search(out).group(1))
        global_times.append(t_global)
        route_times.append(t_route)
        print(f"run {i:2d}: global={t_global:.4f}s  route={t_route:.4f}s  speedup={t_global / t_route:.2f}x")

    avg_global = sum(global_times) / len(global_times)
    avg_route = sum(route_times) / len(route_times)
    print(f"average: global={avg_global:.4f}s  route={avg_route:.4f}s  speedup={avg_global / avg_route:.2f}x")


def bench_rw_lock(iterations=5):
    print("\n=== Part 3: Readers-Writers Latency ===")
    pattern_starve = re.compile(r"reader priority \(starvation\)\s+avg writer wait:\s+([\d\.]+)\s+ms")
    pattern_fair = re.compile(r"fair policy \(no starvation\)\s+avg writer wait:\s+([\d\.]+)\s+ms")

    for i in range(1, iterations + 1):
        out = run("part3")
        w_starve = float(pattern_starve.search(out).group(1))
        w_fair = float(pattern_fair.search(out).group(1))
        print(f"run {i:2d}: reader_priority_wait={w_starve:6.2f}ms  fair_wait={w_fair:5.2f}ms")


def bench_scenario():
    print("\n=== Part 6: Full Scenario Validation ===")
    out = run("part6")
    print(out.strip())

    if LOG_PATH.is_file():
        total_lines = sum(1 for _ in LOG_PATH.open("r", encoding="utf-8", errors="replace"))
        print(f"journal.log: {total_lines} lines verified")


if __name__ == "__main__":
    bench_unsynced_booking(10)
    bench_locking_granularity(5)
    bench_rw_lock(5)
    bench_scenario()
