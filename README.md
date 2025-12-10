Mini Redis Clone in Modern C++
================================

This repository is my hands-on implementation of a lightweight Redis-compatible server. The goal is to internalize how Redis behaves from the socket layer all the way up to RESP parsing, data-structure semantics, blocking list operations, and stream mechanics. Everything is written in modern C++23 with a modular architecture that mirrors real Redis subsystems.

Highlights
----------
- Fully functional event loop backed by `select()` that accepts multiple clients and routes requests without blocking.
- RESP protocol implementation with parser, encoder helpers, and precise error handling.
- RedisStore abstraction that persists strings, lists, and streams in a type-safe way with TTL metadata.
- Blocking list semantics (BLPOP) and the groundwork for stream consumers with proper timeout handling.
- Tests and benchmarks that can be executed end-to-end inside WSL with zero manual dependency wrangling.

Architecture at a Glance
------------------------
- **EventLoop** (`src/server/EventLoop.*`): Manages socket readiness, accepts clients, reads RESP payloads, and delegates execution. Timeouts for blocking commands are driven from here.
- **CommandHandler** (`src/commands`): Central dispatcher. Each RESP command maps to a member handler that returns RESP-encoded responses via `ExecResult`. A shared `RedisStore` reference keeps data manipulation consistent.
- **RedisStore** (`src/db`): Owns the in-memory dictionary of `RedisObj` variants. Provides helper methods to lazily construct lists/streams, manage expirations, and expose objects for type checks.
- **Data Structures** (`src/db/List.*`, `src/db/Stream.*`): Lean implementations of Redis’ list/stream behavior, including sequence validation, ID parsing, and range queries.
- **Utilities** (`src/utils/time.cpp`): Monotonic timing for deadlines and wall-clock timestamps for stream IDs.

Supported Commands
------------------
- **Connection/Utility**: `PING`, `ECHO`, `TYPE`
- **Strings**: `SET`, `SET key value PX ttl`, `GET`
- **Lists**: `LPUSH`, `RPUSH`, `LRANGE`, `LLEN`, `LPOP`, `BLPOP`
- **Streams**: `XADD` (explicit, auto-sequence, auto-generated IDs), `XRANGE`, `XREAD`

Folder Structure
----------------
```
src/
  commands/      CommandHandler core and per-type handlers
  db/            RedisStore plus concrete data structures
  protocol/      RESP parser
  server/        RedisServer + EventLoop
  utils/         Time helpers
tests/           GoogleTest suites covering store, lists, streams, handlers
benchmarks/      Micro-benchmark harness for critical operations
```

Build & Run
-----------
Dependencies are pulled automatically via CMake’s `FetchContent` (Asio and GoogleTest). Only CMake ≥ 3.13, a C++23 compiler, and Git are required on the system.

```bash
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
```

This produces:
- `redis` – the server executable
- `redis_tests` – GoogleTest binary
- `redis_bench` – benchmarking executable
- `build/compile_commands.json` – clangd/VS Code IntelliSense database (symlink it to the repo root if desired)

Testing
-------
All checks run cleanly under WSL:

```bash
ctest --test-dir build        # Aggregated CTest run
# or
./build/redis_tests           # Direct GoogleTest output
```

Benchmarking
------------
The benchmark harness reports throughput for representative workloads (SET/GET round-trip, list push/pop cycles, stream XADD):

```bash
./build/redis_bench
```

Sample output:
```
Redis micro-benchmarks (5000 iterations)
------------------------------------------------------------
Benchmark                             Throughput       Duration (ms)
SET+GET round-trip               255830.05 ops/s            39.088
List RPUSH+LPOP                  319438.50 ops/s            31.305
Stream XADD                      123725.31 ops/s            40.412
```

Development Notes
-----------------
- VS Code users can rely on `.vscode/c_cpp_properties.json`, which already includes `${workspaceFolder}/build/_deps/**`. After the initial configure step, FetchContent headers resolve automatically.
- The EventLoop periodically calls `CommandHandler::checkTimeouts()` and `checkXReadTimeouts()` to wake sleeping clients, so long-running tests should simulate deadlines as needed.
- RedisStore uses `std::variant` for type safety; adding a new data type means extending `RedisType`, the variant, and command handlers accordingly.

Acknowledgements
----------------
This project builds on the “Build Your Own Redis” challenge by [Codecrafters](https://codecrafters.io/challenges/redis). Their spec provided the behavioral contract; this repository captures my implementation journey with a strong emphasis on readability, tests, and a reproducible development environment.
