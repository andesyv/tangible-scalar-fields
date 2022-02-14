#ifndef MOLUMES_DELEGATEUTILS_H
#define MOLUMES_DELEGATEUTILS_H

#include <typeinfo>
#include <functional>
#include "Viewer.h"

namespace molumes {
    template <typename T>
    std::size_t getTypeHash(const T& t) {
        return typeid(t).hash_code();
    }

    template <typename T>
    void subscribe(Viewer& viewer, const std::size_t& hash, std::function<void(T)>&& func) {
        static constexpr auto I = TupleIndex<T, Viewer::BroadcastTypes>::value;
        std::get<I>(viewer.m_subscribers[hash]).push_back(func);
    }

    // Helper function to type-safely subscribe to member variable
    template <typename T, typename C, typename F>
    void subscribe(Viewer& viewer, T C::* variable_ref, F&& func) {
        const auto hash = getTypeHash(variable_ref);
        subscribe<T>(viewer, hash, std::forward<std::function<void(T)>>(func));
    }
}

#endif //MOLUMES_DELEGATEUTILS_H
