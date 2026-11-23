# Bus Ticket Reservation System (POSIX Threads)

## Overview
This project simulates a multi-agency bus ticket booking system in C using POSIX threads (pthread) inside a single process. It solves common concurrency bugs like double-booked seats, slow read queries, deadlocks on round-trip tickets, and mixed-up log files.

- **Part 1**: Shows race conditions and lost updates without locks.
- **Part 2**: Compares a single global mutex against per-route mutexes.
- **Part 3**: Tests readers-writers locks (reader priority vs. fair queueing).
- **Part 4**: Implements a producer-consumer circular queue and a dedicated logger thread.
- **Part 5**: Prevents round-trip deadlocks using lock ordering and trylock with backoff.
- **Part 6**: Runs the full multi-agency simulation and checks data consistency.

---

## Project Structure

```text
OSProject/
├── Makefile                # Build script for all parts
├── README.md               # Documentation
├── journal.log             # Output log written by the logger thread
├── include/                # Header files
│   ├── booking.h           # Route data and booking functions
│   ├── log.h               # Central logger interface
│   ├── queue.h             # Bounded circular queue (producer-consumer)
│   ├── rw_lock.h           # Readers-writers lock definitions
│   └── types.h             # Shared structs and enums
├── src/                    # Shared source files
│   ├── booking.c           # Seat reservation and mutex logic
│   ├── log.c               # Background disk logger thread
│   ├── queue.c             # Circular queue with condition variables
│   └── rw_lock.c           # RW lock implementations
├── parts/                  # Programs for each milestone
│   ├── part1.c             # Unsynchronized simulation
│   ├── part2.c             # Global vs. per-route mutex benchmark
│   ├── part3.c             # Reader starvation vs. fair RW lock
│   ├── part4.c             # Producer-consumer queue and logging
│   ├── part5.c             # Deadlock reproduction and fixes
│   └── part6.c             # Full system stress test and integrity check
└── scripts/
    └── benchmark.py        # Python benchmark runner for Parts 1, 2, 3, and 6
```

---

## Build

Compile all targets:
```bash
make
```

Remove build artifacts and logs:
```bash
make clean
```

---

## Run

### Run Individual Parts

| Part | Description | Linux / macOS | Windows |
| :--- | :--- | :--- | :--- |
| **Part 1** | Race conditions demo | `./parts/part1` | `.\parts\part1.exe` |
| **Part 2** | Mutex granularity test | `./parts/part2` | `.\parts\part2.exe` |
| **Part 3** | Readers-writers latency | `./parts/part3` | `.\parts\part3.exe` |
| **Part 4** | Queue and logger thread | `./parts/part4` | `.\parts\part4.exe` |
| **Part 5** | Deadlock prevention | `./parts/part5` | `.\parts\part5.exe` |
| **Part 6** | Full system test | `./parts/part6` | `.\parts\part6.exe` |

### Run Automated Benchmark

Run the test suite across multiple iterations:
```bash
python scripts/benchmark.py
```

The script measures:
- **Part 1**: Lost ticket updates over 10 runs.
- **Part 2**: Speedup of per-route locks over a global lock (average of 5 runs).
- **Part 3**: Writer wait times under reader-priority vs. fair RW lock (5 runs).
- **Part 6**: Consistency check (verifies double bookings = 0 and audit line count in `journal.log`).
