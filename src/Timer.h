#ifndef MOLUMES_TIMER_H
#define MOLUMES_TIMER_H

#include <chrono>

template <class Clock = std::chrono::steady_clock>
class Timer {
protected:
    std::chrono::time_point<Clock> m_last_t{Clock::now()};

public:
    Timer() = default;

    void reset(const std::chrono::time_point<Clock>& tp = Clock::now()) {
        m_last_t = tp;
    }

    template<typename D = std::chrono::milliseconds>
    auto elapsed() {
        const auto current_t = Clock::now();
        return std::chrono::duration_cast<D>(current_t - m_last_t).count();
    }

    // Returns the elapsed time since last reset and reset timer
    template<typename D = std::chrono::milliseconds>
    auto elapsed_reset() {
        const auto current_t = std::chrono::time_point<Clock>::now();
        const auto el = std::chrono::duration_cast<D>(current_t - m_last_t).count();
        reset(current_t);
        return el;
    }
};

#endif //MOLUMES_TIMER_H
