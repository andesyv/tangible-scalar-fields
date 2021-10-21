#include "GeometryUtils.h"

#include <deque>
#include <tuple>
#include <iostream>
#include <map>

#include <glm/glm.hpp>

#include <globjects/Buffer.h>

using namespace glm;

constexpr double EPS = 0.000001;

namespace molumes {
    auto scalarCross2D(const dvec2 &a, const dvec2 &b) {
        const auto c = cross(dvec3{a, 0.0}, dvec3{b, 0.0});
        return c.z;
    }

    bool ccw(const dvec2 &a, const dvec2 &b, double epsilon = 0.001f) {
        return epsilon <= scalarCross2D(a, b);
    }

    double dotBetween(const dvec2 &a, const dvec2 &b) {
        // -0.25 * [1,-1] + 0.25 -> [0,0.5]
        // 0.25 * [-1,1] + 0.75 -> [0.5,1]
        return ccw(a, b) ? (-0.25 * dot(a, b) + 0.25) : (0.25 * dot(a, b) + 0.75);
    }

    // Hull is in clockwise order, meaning any points inside the hull should make a clockwise turn in respect to the hull
    // Any point who doesn't make a clockwise turn is outside the hull
    bool insideHull(const vec2 &p, const std::vector<vec2> &convexHull) {
        for (auto it{convexHull.begin()}; it != convexHull.end(); ++it) {
            const auto next = it + 1 == convexHull.end() ? convexHull.begin() : it + 1;

            const auto a = *it - p;
            const auto b = *next - *it;

            if (!ccw(a, b, EPS))
                return false;
        }
        return true;
    }

    template<typename It>
    bool includes(It beg, It end, const typename std::iterator_traits<It>::value_type &val) {
        return std::find(beg, end, val) != end;
    }

    std::optional<std::pair<std::vector<vec4>, std::vector<vec4>>>
    geometryPostProcessing(const std::vector<vec4> &vertices, const std::weak_ptr<bool>& controlFlag,
                           float tileHeight) {
        // If we at this point don't have a buffer, it means it got recreated somewhere in the meantime.
        // In which case we don't need this thread anymore.
        if (controlFlag.expired() || vertices.size() < 2)
            return std::nullopt;

        // Filter out empty vertices
        const auto filterEmpties = [](const auto& points) {
            std::vector<vec4> data;
            data.reserve(points.size());
            for (auto it{points.begin()};
                 it != points.end() && it + 1 != points.end() && it + 2 != points.end(); it += 3)
                if (std::all_of(it, it + 3, [](const auto &p) { return EPS < p.w; }))
                    data.insert(data.end(), it, it + 3);

            return data;
        };


        if (controlFlag.expired()) return std::nullopt;

        auto [uniqueVs, indices] = getVertexIndexPairs(filterEmpties(vertices));

        // Find the points which has a non-zero value of z (z height is hex-value, meaning empty ones are empty hexes)
        std::vector<std::pair<dvec2, uint>> nonEmptyValues;
        uint first = 0;
        dvec2 mi{std::numeric_limits<float>::max()}, ma{std::numeric_limits<float>::min()};
        nonEmptyValues.reserve(indices.size());
        for (uint i = 0; i < indices.size(); ++i) {
            const auto &v = uniqueVs.at(indices.at(i));
            const auto w = dvec2{v.x, v.y};
            mi = min(w, mi);
            ma = max(w, ma);

            if (-tileHeight < v.z) {
                nonEmptyValues.emplace_back(w, i);
                // Also find smallest (for graham scan later)
                const auto &oldSmallest = std::get<0>(nonEmptyValues[first]);
                if (w.y < oldSmallest.y || w.y < oldSmallest.y + EPS && w.x < oldSmallest.x) // y' < y && x' <= x
                    first = static_cast<uint>(nonEmptyValues.size() - 1);
            }
        }
        const auto boundingCenter = 0.5 * mi + 0.5 * ma;

        if (controlFlag.expired() || nonEmptyValues.size() < 2)
            return std::nullopt;

        // Make sure smallest is first
        std::swap(nonEmptyValues[0], nonEmptyValues[first]);

        // Create convex hull
        const auto convexHull = createConvexHull(map(nonEmptyValues.begin(), nonEmptyValues.end(), [](const auto &t) {
            return std::get<0>(t);
        }), boundingCenter, controlFlag);

        if (controlFlag.expired()) return std::nullopt;

        // Filter out all points outside of hull from empty values
        // Non-empty values will always be inside the hull
        for (int i{static_cast<int>(indices.size()) - 3}; -1 < i; i -= 3) {
            const auto it = indices.begin() + i;
            if (std::all_of(it, it + 3, [&convexHull, &uniqueVs = uniqueVs](const auto &j) { // &uniqueVs = uniqueVs is a temporary fix until C++23 fixes structured bindings
                const auto p = vec2{uniqueVs.at(j)};
                return !insideHull(p, convexHull);
            }))
                indices.erase(it, it + 3);
        }

        uniqueVs = map(indices.begin(), indices.end(), [&uniqueVs = uniqueVs](auto i){ return uniqueVs.at(i); });

        if (controlFlag.expired()) return std::nullopt;

        const auto hullListConverted = map(convexHull.begin(), convexHull.end(), [tileHeight](const auto &v) {
            return vec4{v.x, v.y, -tileHeight, 1.f};
        });


        return controlFlag.expired() ? std::nullopt : std::make_optional(std::make_pair(uniqueVs, hullListConverted));
    }

    std::vector<glm::vec2> createConvexHull(const std::vector<glm::dvec2> &points, glm::dvec2 boundingCenter,
                                            const std::weak_ptr<bool>& controlFlag) {
        // Set comparison direction, the start of the polar angle circle, to be the first point
        const auto compareDir = normalize(points[0] - boundingCenter);

        if (controlFlag.expired()) return {};

        std::vector<std::pair<dvec2, double>> nonEmptyValuesPolarAngled;
        nonEmptyValuesPolarAngled.reserve(points.size());
        nonEmptyValuesPolarAngled.emplace_back(points.at(0), 0.0);
        for (auto it{points.begin() + 1}; it != points.end(); ++it) {
            const auto &p = *it;
            const auto dir = p - boundingCenter;
            const auto dir_len = length(dir);
            if (EPS < dir_len) {
                const auto pa = dotBetween(compareDir, dir * (1. / dir_len));
                nonEmptyValuesPolarAngled.emplace_back(p, pa);
            }
        }

        if (controlFlag.expired()) return {};

        // Sort points on polar angle
        std::sort(nonEmptyValuesPolarAngled.begin(), nonEmptyValuesPolarAngled.end(), [](const auto &a, const auto &b) {
            return std::get<1>(a) < std::get<1>(b);
        });

        if (controlFlag.expired()) return {};

        // Graham scan implementation:
        std::deque<dvec2> convexHullStack{};
        for (const auto&[p, polar_angle]: nonEmptyValuesPolarAngled) {
            // Skip empty/near-empty increments:
            if (!convexHullStack.empty()) {
                const auto last = convexHullStack[0];
                const auto b = p - last;

                if (length(b) < EPS)
                    continue;
            }

            while (2 <= convexHullStack.size()) {
                const auto last = convexHullStack[0];
                const auto second_last = convexHullStack[1];
                auto a = last - second_last;
                auto b = p - last;

                // If the new point forms a clockwise turn with the last points in the stack,
                // pop the last point from the stack
                if (ccw(a, b, EPS))
                    break;
                else
                    convexHullStack.pop_front();
            }

            // Since points are sorted on polar coordinates and empty increments are skipped,
            // no point that arrives here should give an empty increment
            convexHullStack.push_front(p);

            if (controlFlag.expired()) return {};
        }

        return map(convexHullStack.rbegin(), convexHullStack.rend() - 1,
                   [](const auto &p) {
                       return glm::vec2{p.x, p.y};
                   });
    }

    template <typename T, int N>
    std::weak_ordering compareVec(const vec<N, T>& as, const vec<N, T>& bs, unsigned int i = 0, float epsilon = 0.0001f) {
        return N <= i ? std::weak_ordering::equivalent : (as[i] < bs[i] + epsilon && bs[i] < as[i] + epsilon) ?
            compareVec(as, bs, i+1, epsilon) :
            ((as[i] < bs[i]) ? std::weak_ordering::less : std::weak_ordering::greater);
    }

    struct Vec4Comparitor {
        auto operator()(const vec4& lhs, const vec4& rhs) const {
            return compareVec(lhs, rhs) == std::weak_ordering::less;
        }
    };

    std::pair<std::vector<vec4>, std::vector<unsigned int>>
    getVertexIndexPairs(const std::vector<vec4> &vertices) {
        std::vector<vec4> uniqueVertices;
        uniqueVertices.reserve(vertices.size());
        std::vector<unsigned int> indices;
        indices.reserve(vertices.size());

        std::map<vec4, unsigned int, Vec4Comparitor> lookupMap;

        for (const auto& v : vertices) {
            const auto pos = lookupMap.find(v);
            if (pos != lookupMap.end()) {
                indices.push_back(pos->second);
            } else {
                uniqueVertices.push_back(v);
                const auto i = static_cast<unsigned int>(lookupMap.size());
                indices.push_back(i);
                lookupMap.emplace(v, i);
            }
        }

        uniqueVertices.shrink_to_fit();
        indices.shrink_to_fit();

        return std::make_pair(uniqueVertices, indices);
    }

    auto sortedEdge(auto a, auto b) {
        return a < b ? std::make_pair(a, b) : std::make_pair(b,a);
    }

    std::set<std::pair<unsigned int, unsigned int>> getEdges(const std::vector<unsigned int> &indices) {
        std::set<std::pair<unsigned int, unsigned int>> edges;
        for (uint i{0}; i+2 < indices.size(); i+=3) {
            for (const auto& edge : {
                sortedEdge(indices.at(i), indices.at(i+1)),
                sortedEdge(indices.at(i+1), indices.at(i+2)),
                sortedEdge(indices.at(i+2), indices.at(i))
            })
                edges.insert(edge);
        }

        return edges;
    }
}