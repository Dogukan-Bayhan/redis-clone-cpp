#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "../src/commands/CommandHandler.hpp"
#include "../src/db/RedisStore.hpp"
#include "../tests/TestHelpers.hpp"

struct BenchmarkResult {
    std::string name;
    size_t operations;
    double duration_ms;
};

BenchmarkResult benchSetGet(size_t iterations) {
    RedisStore store;
    CommandHandler handler(store);

    auto start = std::chrono::steady_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        std::string key = "key:" + std::to_string(i);
        std::string value = "value:" + std::to_string(i);

        auto setArgs = makeArgs(std::vector<std::string>{"SET", key, value});
        handler.execute(setArgs.views, 1);

        auto getArgs = makeArgs(std::vector<std::string>{"GET", key});
        handler.execute(getArgs.views, 1);
    }
    auto end = std::chrono::steady_clock::now();

    double duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
    return {"SET+GET round-trip", iterations * 2, duration_ms};
}

BenchmarkResult benchListPushPop(size_t iterations) {
    RedisStore store;
    CommandHandler handler(store);

    auto start = std::chrono::steady_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        std::string payload = "job:" + std::to_string(i);

        auto pushArgs = makeArgs(std::vector<std::string>{"RPUSH", "jobs", payload});
        handler.execute(pushArgs.views, 1);

        auto popArgs = makeArgs(std::vector<std::string>{"LPOP", "jobs"});
        handler.execute(popArgs.views, 1);
    }
    auto end = std::chrono::steady_clock::now();

    double duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
    return {"List RPUSH+LPOP", iterations * 2, duration_ms};
}

BenchmarkResult benchStreamXadd(size_t iterations) {
    RedisStore store;
    CommandHandler handler(store);

    auto start = std::chrono::steady_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        std::string value = "reading:" + std::to_string(i);

        auto args = makeArgs(std::vector<std::string>{"XADD", "telemetry", "*", "sensor", value});
        handler.execute(args.views, 1);
    }
    auto end = std::chrono::steady_clock::now();

    double duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
    return {"Stream XADD", iterations, duration_ms};
}

int main() {
    const size_t iterations = 5000;
    std::vector<BenchmarkResult> results;
    results.push_back(benchSetGet(iterations));
    results.push_back(benchListPushPop(iterations));
    results.push_back(benchStreamXadd(iterations));

    std::cout << "Redis micro-benchmarks (" << iterations << " iterations)" << std::endl;
    std::cout << "------------------------------------------------------------" << std::endl;
    std::cout << std::left << std::setw(30) << "Benchmark"
              << std::right << std::setw(18) << "Throughput"
              << std::setw(20) << "Duration (ms)" << std::endl;

    for (const auto& res : results) {
        double ops_per_sec = res.operations / (res.duration_ms / 1000.0);
        std::cout << std::left << std::setw(30) << res.name
                  << std::right << std::setw(12) << std::fixed << std::setprecision(2)
                  << ops_per_sec << " ops/s"
                  << std::setw(18) << std::setprecision(3) << res.duration_ms
                  << std::endl;
    }

    return 0;
}
