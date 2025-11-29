#pragma once
#include <chrono>


/*
------------------------------------------------------------------------------
  CLOCK TYPES IN C++ <chrono>
------------------------------------------------------------------------------

1) std::chrono::steady_clock
   - Monotonic clock: always moves forward, never adjusted backwards.
   - Immune to system time changes (NTP sync, user changing clock, DST shifts).
   - Suitable for measuring durations, latency, and timeouts.
   - NOT suitable for generating real-world timestamps.

2) std::chrono::system_clock
   - Represents the actual wall-clock (real-world) time.
   - Can move forwards or backwards if the OS clock changes.
   - Used for generating Unix timestamps and time-based event IDs.
   - Equivalent to "current time since 1970-01-01 00:00:00 UTC".

In summary:
   steady_clock → for measuring *intervals*.
   system_clock → for reading *real time*.

------------------------------------------------------------------------------
*/

/**
 * @brief Returns the current monotonic time in milliseconds.
 *
 * Uses std::chrono::steady_clock, which guarantees:
 *   - monotonic progression (no jumps backwards),
 *   - no sensitivity to system time adjustments.
 *
 * This function is ideal for:
 *   - timeout calculations,
 *   - profiling,
 *   - rate limiting,
 *   - retry logic or latency measurements.
 *
 * IMPORTANT:
 *   The returned value is NOT a timestamp and has no relation
 *   to Unix epoch. Only differences between two calls are meaningful.
 */
inline uint64_t current_time_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}


/**
 * @brief Returns the current Unix timestamp in milliseconds.
 *
 * Uses std::chrono::system_clock, which represents wall-clock time
 * and can be converted to calendar dates. This is the correct clock
 * type for any operation that requires real-world timestamps, such as:
 *   - generating Redis-like stream IDs (ms-seq),
 *   - logging,
 *   - XRANGE/Time-based queries,
 *   - persistence and event metadata.
 *
 * NOTE:
 *   Since system_clock can jump forward/backward (NTP, manual change),
 *   it should NOT be used for measuring durations.
 */
inline long long getUnixTimeMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()
    ).count();
}