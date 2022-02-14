#ifndef MOLUMES_PROFILE_H
#define MOLUMES_PROFILE_H

#ifndef NDEBUG

#include <iostream>
#include <format>
#include <map>

#include "Timer.h"

class ProfileBlock {
    private:
        Timer<std::chrono::high_resolution_clock> m_timer{};
        uint64_t& m_output;

    public:
        ProfileBlock() = delete;
        explicit ProfileBlock(uint64_t& nanosecond_output) : m_output{nanosecond_output} {}

        ~ProfileBlock() {
            m_output += m_timer.elapsed<std::chrono::nanoseconds>();
        }
    };

    /**
     * Singleton profiling manager helper class.
     * Holds a table of all profiling recordings
     */
    class Profiler {
    private:
        Profiler() = default;

    public:
        static Profiler& get() {
            static Profiler instance;
            return instance;
        }

        // Per string identifier, a pair containing the total time in nanoseconds and total increments
        std::map<std::string, std::pair<uint64_t, uint32_t>> m_time_table;

        static ProfileBlock profile(const std::string& key) {
            auto& [total, count] = get().m_time_table[key];
            ++count;
            return ProfileBlock{total};
        }

        void print() {
            std::cout << "============ Profiler results: ============" << std::endl;
            for (const auto& [key, data] : m_time_table) {
                const auto& [total, count] = data;
                std::cout << std::format("Key: {}, Total: {}ns, Average: {}ns", key, total, total / count) << std::endl;
            }
        }

        static void reset_profiler() {
            std::cout << "Profiler timings reset. Previous results:" << std::endl;
            get().print();
            auto& table = get().m_time_table;
            for (auto& t : table)
                t.second = {0u, 0u};
        }

        ~Profiler() {
            print();
        }
    };

/**
 * Profiling helper macro. Code only generated on non-release builds
 * It creates a profiling object that appends the time from creation to destruction to the "tag". To use: Add macro
 * inside a block scope with a unique identifier to that scope
 */
#define PROFILE(tag) auto _p = Profiler::profile(tag)
#else
#define PROFILE(tag)
#endif

#endif //MOLUMES_PROFILE_H
