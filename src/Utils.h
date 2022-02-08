#ifndef MOLUMES_UTILS_H
#define MOLUMES_UTILS_H

#include <memory>
#include <chrono>

#include <glbinding/gl/types.h>
#include <glbinding/gl/enum.h>

#include <globjects/State.h>
#include <globjects/Program.h>

namespace molumes {
    /**
     * @brief Helper function that creates a guard object which reverts back to it's original OpenGL state when it exits the scope.
     */
    const auto stateGuard = []() {
        return std::shared_ptr<globjects::State>{globjects::State::currentState().release(), [](globjects::State *p) {
            globjects::Program::release();

            if (p != nullptr) {
                p->apply();
                delete p;
            }
        }};
    };



    // =========================== Helper Guard Objects ================================

    /// Guard helper object for bind()/unbind()
    template<typename T, typename I = void>
    class BindGuard {
    private:
        using U = typename T::element_type;
        U *const m_ref;
        I m_target;

        void bind() {
            if (m_ref) {
                if constexpr (std::is_void_v<I>) {
                    m_ref->bind();
                } else {
                    m_ref->bind(m_target);
                }
            }
        }

    public:
        BindGuard(const BindGuard &) = delete;

        explicit BindGuard(T &ptr) : m_ref{ptr.get()} { bind(); }

        explicit BindGuard(T &ptr, I target) : m_ref{ptr.get()}, m_target{target} { bind(); }

        ~BindGuard() {
            if (m_ref) {
                if constexpr (std::is_void_v<I>) {
                    m_ref->unbind();
                } else {
                    m_ref->unbind(m_target);
                }
            }
        }
    };

    /// Guard helper object for bindActive(index)/unbindActive(index)
    template<typename T, std::integral I = unsigned int>
    class BindActiveGuard {
    private:
        using U = typename T::element_type;
        U *const m_ref;
        I m_index;

    public:
        BindActiveGuard(const BindActiveGuard &) = delete;

        explicit BindActiveGuard(T &ptr, I index = 0) : m_ref{ptr.get()}, m_index{index} {
            if (m_ref)
                m_ref->bindActive(m_index);
        }

        ~BindActiveGuard() {
            if (m_ref)
                m_ref->unbindActive(m_index);
        }
    };

    /// Guard helper object for bindBase(target, location)/unbind(target)
    template<typename T, std::integral I = unsigned int>
    class BindBaseGuard {
    private:
        using U = typename T::element_type;
        U *const m_ref;
        const gl::GLenum m_target;
        I m_index;

    public:
        BindBaseGuard(const BindBaseGuard &) = delete;

        explicit BindBaseGuard(T &ptr, gl::GLenum target, I index = 0) : m_ref{ptr.get()}, m_target{target},
                                                                         m_index{index} {
            if (m_ref)
                m_ref->bindBase(m_target, m_index);
        }

        ~BindBaseGuard() {
            if (m_ref)
                m_ref->unbind(m_target, m_index);
        }
    };

    class Timer {
    protected:
        std::chrono::steady_clock::time_point m_last_t{std::chrono::steady_clock::now()};

    public:
        Timer() = default;

        void reset(std::chrono::steady_clock::time_point tp = std::chrono::steady_clock::now()) {
            m_last_t = tp;
        }

        template<typename D = std::chrono::milliseconds>
        auto elapsed() {
            const auto current_t = std::chrono::steady_clock::now();
            return std::chrono::duration_cast<D>(current_t - m_last_t).count();
        }

        // Returns the elapsed time since last reset and reset timer
        template<typename D = std::chrono::milliseconds>
        auto elapsed_reset() {
            const auto current_t = std::chrono::steady_clock::now();
            const auto el = std::chrono::duration_cast<D>(current_t - m_last_t).count();
            reset(current_t);
            return el;
        }
    };

    class DebugTimer : public Timer {
    private:
        DebugTimer() = default;

    public:
        static DebugTimer &get() {
            static DebugTimer instance{};
            return instance;
        }

        template<typename D = std::chrono::milliseconds>
        static auto elapsed() { return get().elapsed_reset<D>(); }
    };
}

#endif //MOLUMES_UTILS_H
