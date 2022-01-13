#ifndef MOLUMES_GEOMETRYUTILS_H
#define MOLUMES_GEOMETRYUTILS_H

#include <iterator>
#include <vector>
#include <algorithm>
#include <optional>
#include <memory>

#include <glm/vec4.hpp>

namespace globjects {
    class Buffer;
}

namespace molumes {
    template<std::forward_iterator It, typename F>
    auto filter(It
                begin,
                It end, F
                &&predicate) {
        using T = decltype(*(It{}));
        std::vector<std::remove_cvref_t<T>> out;
        out.
                reserve(std::distance(begin, end)
        );
        std::copy_if(begin, end, std::back_inserter(out), predicate
        );
        return
                out;
    }

    template<std::forward_iterator It, typename F>
    auto map(It
             begin,
             It end, F
             &&predicate) {
        using T = typename It::value_type; // Only works on iterators with value_type member
        using K = decltype(predicate(T{}));
        std::vector<std::remove_cvref_t<K>> out;
        out.
                reserve(std::distance(begin, end)
        );
        std::transform(begin, end, std::back_inserter(out), predicate
        );
        return
                out;
    }

    std::optional<std::vector<glm::vec4>>
    geometryPostProcessing(const std::vector<glm::vec4> &vertices, const std::weak_ptr<bool> &controlFlag);

    std::optional<std::vector<glm::uint>>
    getHexagonConvexHull(const std::vector<glm::vec4> &vertices, const std::weak_ptr<bool> &controlFlag,
                         float upperThreshold, float lowerThreshold);

    std::pair<std::vector<glm::vec4>, std::vector<unsigned int>>
    getVertexIndexPairs(const std::vector<glm::vec4> &vertices);
}

#endif //MOLUMES_GEOMETRYUTILS_H
