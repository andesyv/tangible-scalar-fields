#ifndef MOLUMES_DELEGATEUTILS_H
#define MOLUMES_DELEGATEUTILS_H

#include <typeinfo>
#include <functional>
#include "Viewer.h"

namespace molumes {
    template <typename C>
    auto& create_class_instance() {
        static C dummy{};
        return dummy;
    }

    /**
     * @brief Creates a hash of a member variable pointer.
     * It works by creating an empty dummy instance of whatever class it gets templated as, and hashing the
     * addresses of the local class. For a member variable to be hashable, it therefore needs to be default
     * constructable / destructable.
     * @param variable_ref Member pointer to variable address. (&Class::variable)
     * @return
     */
    template <typename T, typename C>
    auto get_member_variable_hash(T C::* variable_ref) {
        auto local_addr = &(create_class_instance<C>().*variable_ref);
        return std::hash<const unsigned char*>{}(static_cast<const unsigned char*>(static_cast<const void*>(local_addr)));
    }

    template <typename T>
    void subscribe(Viewer& viewer, const Viewer::BroadcastHashType& hash, std::function<void(T)>&& func) {
        static constexpr auto I = TupleIndex<T, Viewer::BroadcastTypes>::value;
        std::get<I>(viewer.m_subscribers[hash]).push_back(func);
    }

    // Helper function to type-safely subscribe to member variable
    template <typename T, typename C, typename F>
    void subscribe(Viewer& viewer, T C::* variable_ref, F&& func) {
        auto hash = get_member_variable_hash(variable_ref);
        subscribe<T>(viewer, hash, std::forward<std::function<void(T)>>(func));
    }

/**
 * @brief Broadcasting helper macro.
 *
 * Broadcasts the variable, indicated by a member address pointer, on the "this" instance.
 * Example: viewer()->BROADCAST(&ClassName::variable);
 *
 * https://en.cppreference.com/w/cpp/utility/functional/mem_fn
 */
#define BROADCAST(addr) broadcast(get_member_variable_hash(addr), this->*addr)
}

#endif //MOLUMES_DELEGATEUTILS_H
