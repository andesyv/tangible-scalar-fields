#ifndef MOLUMES_CHANNEL_H
#define MOLUMES_CHANNEL_H

#include <mutex>
#include <deque>
#include <optional>
#include <memory>
#include <tuple>
#include <array>

/**
 * Go/Rust inspired method of handling direct data transfer between threads
 * Creation/copying of Channels is not thread safe and has to happen on the same thread.
 * Every member function is thread safe
 * @deprecated Use ReaderChannel and WriterChannel instead
 */
template<typename T>
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
    auto flush(std::deque<T> &vals) {
        std::vector<T> flushed{vals.begin(), vals.end()};
        vals.clear();
        return std::move(flushed);
    }

    std::vector<T> flush() { return flush(m_shared->vals); }

    auto get_lock(std::mutex &m) { return std::lock_guard{m}; }

    auto get_lock() { return get_lock(m_shared->mutex); }


public:
    Channel() : m_shared{std::make_shared<Container>()} {}

    Channel(const Channel &rhs) = delete;

    Channel(Channel &rhs) : m_shared{rhs.m_shared} {}

    Channel(Channel &&rhs) noexcept: m_shared{std::move(rhs.m_shared)} {}

    Channel &operator=(const Channel &rhs) = delete;

    Channel &operator=(Channel &rhs) {
        m_shared = rhs.m_shared;
        return *this;
    };

    Channel &operator=(Channel &&rhs) noexcept {
        m_shared = std::move(rhs.m_shared);
        return *this;
    };

    void send(const T &val) {
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
        auto&[mutex, vals] = *m_shared;
        const auto l = get_lock(mutex);

        return vals.empty() ? std::nullopt : std::make_optional(flush(vals).back());
    }

    // Returns the next value if it exists
    std::optional<T> try_get() {
        if (!m_shared) return std::nullopt;
        auto&[mutex, vals] = *m_shared;
        const auto l = get_lock(mutex);
        if (!vals.empty()) {
            auto ret = vals.front();
            vals.pop_front();
            return std::optional{std::move(ret)};
        }

        return std::nullopt;
    }
};

/**
 * (Probably) improved channel implementation
 * This version only synchronizes writing. It does this by using a double-buffer setup,
 * with the writer having access to both and switching, while locking, which buffer can
 * be read from. After writing to one buffer, it will switch the pointer to said buffer
 * such that consecutive reads will be safe. A race condition might occur if receiver is
 * slower than sender, in which case the sender might start writing to the buffer the
 * receiver is reading from.
 */
template<typename T, std::size_t BufferCount = 2>
class ChannelBase {
protected:
    struct SharedBlock {
        std::array<T, BufferCount> data;
        std::size_t index{BufferCount - 1};
        std::mutex mutex;
    };
    std::shared_ptr<SharedBlock> m_shared;

    // Protected constructor makes it only be constructable via base classes
    ChannelBase() : m_shared{std::make_shared<SharedBlock>()} {}

public:
    static constexpr auto buffer_count = BufferCount;

    ChannelBase(ChannelBase& rhs) : m_shared{rhs.m_shared} {}
    // Explicit move constructor
    ChannelBase(ChannelBase &&rhs) noexcept : m_shared{std::move(rhs.m_shared)} {}
    ChannelBase(const ChannelBase &) = delete;
};

/**
 * Reader channel that reads from a common ChannelBase
 * Does not perform any synchronization outside of construction. If data race is encountered
 * (for instance if reading thread is slower than the writing thread) the buffer count should be increased.
 */
template<typename T>
class ReaderChannel : public ChannelBase<T> {
public:
    ReaderChannel() = default;

    template<std::convertible_to<ChannelBase<T>> C>
    explicit ReaderChannel(C &rhs) : ChannelBase<T>{*static_cast<ChannelBase<T>*>(&rhs)} {}
    ReaderChannel(ReaderChannel&& rhs) noexcept : ChannelBase<T>{std::move(static_cast<ChannelBase<T>>(rhs))} {}


    ReaderChannel(const auto &) = delete;

    T get() { return this->m_shared->data.at(this->m_shared->index); }
};

/**
 * Writer channel that writes to a common ChannelBase
 * Performs synchronization on construction and writing. It's possible to have multiple writers,
 * in which case the writers will synchronize between themselves.
 * (one writer + any amount of readers needs no synchronization)
 */
template<typename T>
class WriterChannel : public ChannelBase<T> {
public:
    WriterChannel() = default;

    template<std::convertible_to<ChannelBase<T>> C>
    explicit WriterChannel(C &rhs) : ChannelBase<T>{*static_cast<ChannelBase<T>*>(&rhs)} {}
    WriterChannel(WriterChannel&& rhs) noexcept : ChannelBase<T>{std::move(static_cast<ChannelBase<T>>(rhs))} {}

    WriterChannel(const auto &) = delete;

    // No synchronization needed
    void write(const T &data) {
        std::lock_guard lock{this->m_shared->mutex};
        std::size_t next_index = (this->m_shared->index + 1) % this->buffer_count;
        this->m_shared->data.at(next_index) = data;
        this->m_shared->index = next_index;
    }
};

#endif //MOLUMES_CHANNEL_H
