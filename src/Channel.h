#ifndef MOLUMES_CHANNEL_H
#define MOLUMES_CHANNEL_H

#include <mutex>
#include <deque>
#include <optional>
#include <memory>
#include <tuple>

/**
 * Go/Rust inspired method of handling direct data transfer between threads
 * Creation/copying of Channels is not thread safe and has to happen on the same thread.
 * Every member function is thread safe
 */
template <typename T>
class Channel {
private:
    struct Container {
        std::mutex mutex;
        std::deque<T> vals;
    };

    /**
     * Shared data container
     * Note: Changing the pointer itself is not thread safe
     */
    std::shared_ptr<Container> m_shared{};

    // This function assumes context has been locked with a mutex (does no safety checks)
    auto flush(std::deque<T>& vals) {
        std::vector<T> flushed{vals.begin(), vals.end()};
        vals.clear();
        return std::move(flushed);
    }
    std::vector<T> flush() { return flush(m_shared->vals); }

    auto get_lock(std::mutex& m) { return std::lock_guard{m}; }
    auto get_lock() { return get_lock(m_shared->mutex); }


public:
    Channel() : m_shared{std::make_shared<Container>()} {}
    Channel(const Channel& rhs) = delete;
    Channel(Channel& rhs) : m_shared{rhs.m_shared} {}
    Channel(Channel&& rhs) noexcept : m_shared{std::move(rhs.m_shared)} {}

    Channel& operator=(const Channel& rhs) = delete;
    Channel& operator=(Channel& rhs) {
        m_shared = rhs.m_shared;
        return *this;
    };
    Channel& operator=(Channel&& rhs) noexcept {
        m_shared = std::move(rhs.m_shared);
        return *this;
    };

    void send(const T& val) {
        if (!m_shared) return;
        const auto l = get_lock();

        m_shared->vals.push_back(val);
    }

    bool empty() {
        if (!m_shared) return false;
        const auto l = get_lock();

        return m_shared->vals.empty();
    }

    // Flushes the entire queue and returns the last value if it exists
    std::optional<T> try_get_last() {
        if (!m_shared) return std::nullopt;
        auto& [mutex, vals] = *m_shared;
        const auto l = get_lock(mutex);

        return vals.empty() ? std::nullopt : std::make_optional(flush(vals).back());
    }

    // Returns the next value if it exists
    std::optional<T> try_get() {
        if (!m_shared) return std::nullopt;
        auto& [mutex, vals] = *m_shared;
        const auto l = get_lock(mutex);
        if (!vals.empty()) {
            auto ret = vals.front();
            vals.pop_front();
            return std::optional{std::move(ret)};
        }

        return std::nullopt;
    }
};

#endif //MOLUMES_CHANNEL_H
