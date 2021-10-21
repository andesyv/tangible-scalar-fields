#ifndef MOLUMES_GEOMETRYUTILS_H
#define MOLUMES_GEOMETRYUTILS_H

#include <utility>
#include <iterator>
#include <type_traits>
#include <vector>
#include <algorithm>
#include <optional>
#include <memory>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace globjects {
    class Buffer;
}

namespace molumes {
    template<typename Iter>
    class cyclic_iterator {
    public:
        using difference_type = typename Iter::difference_type;

    private:
        const Iter _beg;
        const Iter _end;
        difference_type _rangeSize;

    public:
        difference_type i = 0;

        cyclic_iterator(Iter range_begin, Iter range_end, const difference_type &offset)
                : _beg{std::move(range_begin)}, _end{std::move(range_end)}, i{offset},
                  _rangeSize{std::distance(_beg, _end)} {}

        cyclic_iterator(const cyclic_iterator &) = default;

        auto operator++() {
            ++i;
            return *this;
        }

        auto operator--() {
            --i;
            return *this;
        }

        auto get_iterator() {
            return (i < 0 ? _end : _beg) + static_cast<difference_type>(i % _rangeSize);
        }

        auto operator*() {
            return *get_iterator();
        }

        auto operator+(const difference_type &diff) {
            return cyclic_iterator{_beg, _end, i + diff};
        }

        auto operator-(const difference_type &diff) {
            return *this + (-diff);
        }
    };

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

    template<std::forward_iterator It, typename F>
    auto filter_map(It begin, It end, F &&func) {
        using T = typename It::value_type; // Only works on iterators with value_type member
        using K = decltype(*func(T{}));
        std::vector<std::remove_cvref_t<K>> out;
        out.reserve(std::distance(begin, end));

        for (auto it{begin}; it != end; ++it) {
            auto&& v = func(*it);
            if (v)
                out.push_back(*v);
        }

        return out;
    }

    // Graham scan implementation (first point should be start point / bounds max)
    std::vector<glm::vec2> createConvexHull(const std::vector<glm::dvec2>& points, glm::dvec2 boundingCenter = {}, const std::weak_ptr<bool>& controlFlag = {});

    std::optional<std::pair<std::vector<glm::vec4>, std::vector<glm::vec4>>>
    geometryPostProcessing(const std::vector<glm::vec4> &vertices, const std::weak_ptr<bool>& controlFlag,
                           float tileHeight);
}

#endif //MOLUMES_GEOMETRYUTILS_H
