#ifndef MOLUMES_UTILS_H
#define MOLUMES_UTILS_H

#include <memory>

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
    template<typename T, typename I>
    class BindTargetGuard {
    private:
        using U = typename T::element_type;
        U *const m_ref;
        I m_target;

        void bind() {
            if (m_ref)
                m_ref->bind(m_target);
        }

    public:
        BindTargetGuard(const BindTargetGuard &) = delete;

        explicit BindTargetGuard(T &ptr, I target) : m_ref{ptr.get()}, m_target{target} { bind(); }

        ~BindTargetGuard() {
            if (m_ref)
                m_ref->unbind(m_target);
        }
    };

    template<typename T>
    class BindGuard {
    private:
        using U = typename T::element_type;
        U *const m_ref;

        void bind() {
            if (m_ref)
                m_ref->bind();
        }

    public:
        BindGuard(const BindGuard &) = delete;

        explicit BindGuard(T &ptr) : m_ref{ptr.get()} { bind(); }

        ~BindGuard() {
            if (m_ref)
                m_ref->unbind();
        }
    };

    /// Guard helper object for bindActive(index)/unbindActive(index)
    template<typename T, std::integral I = unsigned int>
    class BindActiveGuard {
    private:
        using U = typename T::element_type;
        U* m_ref;
        I m_index;

    public:
        BindActiveGuard(const BindActiveGuard &) = delete;
        BindActiveGuard(BindActiveGuard&& rhs) noexcept : m_ref{rhs.m_ref}, m_index{rhs.m_index} {
            rhs.m_ref = nullptr;
            if (m_ref)
                m_ref->bindActive(m_index);
        }

        explicit BindActiveGuard(T &ptr, I index = 0) : m_ref{ptr.get()}, m_index{index} {
            if (m_ref)
                m_ref->bindActive(m_index);
        }

        void operator=(const BindActiveGuard&) = delete;
        void operator=(BindActiveGuard&&) = delete;

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

    template <typename F>
    void do_once(F&& func) {
        static bool done{false};
        if (!done) {
            func();
            done = true;
        }
    }
}

#endif //MOLUMES_UTILS_H
