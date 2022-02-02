#ifndef MOLUMES_CHANNEL_H
#define MOLUMES_CHANNEL_H

#include <mutex>
#include <deque>
#include <optional>
#include <memory>

// Go/Rust inspired method of handling direct data transfer between threads
template <typename T>
class Channel {
private:
    std::atomic<std::shared_ptr<std::mutex>> m_mutex{};
    std::deque<T> m_vals;

    std::vector<T> flush() {
        std::vector<T> flushed{m_vals.begin(), m_vals.end()};
        m_vals.clear();
        return std::move(flushed);
    }

    auto get_lock() {
        return std::lock_guard{*m_mutex.load()};
    }

public:
    Channel() = default;
    Channel(const Channel& rhs) = delete;
    Channel(Channel& rhs) : m_mutex{rhs.m_mutex}, m_vals{rhs.m_vals} {}
    Channel(Channel&& rhs) noexcept : m_mutex{std::move(rhs.m_mutex)}, m_vals{std::move(m_vals)} {}

    Channel& operator=(const Channel& rhs) = delete;
    Channel& operator=(Channel&& rhs) noexcept {
        m_mutex = std::move(rhs.m_mutex);
        m_vals = std::move(rhs.m_vals);
        return *this;
    };

    void send(const T& val) {
        const auto l = get_lock();
        m_vals.push_back(val);
    }

    bool empty() {
        const auto l = get_lock();
        return m_vals.empty();
    }

    // Flushes the entire queue and returns the last value if it exists
    std::optional<T> try_get_last() {
        const auto l = get_lock();
        return m_vals.empty() ? std::nullopt : std::optional{flush().back()};
    }

    // Returns the next value if it exists
    std::optional<T> try_get() {
        const auto l = get_lock();
        if (m_vals.empty()) {
            auto ret = m_vals.front();
            m_vals.pop_front();
            return std::optional{std::move(ret)};
        }

        return std::nullopt;
    }
};

#endif //MOLUMES_CHANNEL_H
