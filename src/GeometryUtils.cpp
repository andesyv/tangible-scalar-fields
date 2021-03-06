#include "GeometryUtils.h"

#include <deque>
#include <tuple>
#include <iostream>
#include <utility>
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

    auto getHexagonPositions(const std::vector<vec4> &vertices) {
        constexpr auto stride = 3u * 6u * 2u;
        std::vector<std::pair<vec4, unsigned int>> centers;
        centers.reserve(vertices.size() / stride);
        // hexID * 3 * 6 * 2
        // First vertex in every stride offset is the center
        for (uint i{0}; i < vertices.size(); i += stride)
            centers.emplace_back(vertices.at(i), i);
        return centers;
    }

    template<typename T, int N>
    std::weak_ordering
    compareVec(const vec<N, T> &as, const vec<N, T> &bs, unsigned int i = 0, float epsilon = 0.0001f) {
        return N <= i ? std::weak_ordering::equivalent : (as[i] < bs[i] + epsilon && bs[i] < as[i] + epsilon) ?
                                                         compareVec(as, bs, i + 1, epsilon) :
                                                         ((as[i] < bs[i]) ? std::weak_ordering::less
                                                                          : std::weak_ordering::greater);
    }

    struct vec4_comparator {
        auto operator()(const vec4 &lhs, const vec4 &rhs) const {
            return compareVec(lhs, rhs) == std::weak_ordering::less;
        }
    };

    std::pair<std::vector<vec4>, std::vector<unsigned int>> getVertexIndexPairs(const std::vector<vec4> &vertices) {
        std::vector<vec4> uniqueVertices;
        uniqueVertices.reserve(vertices.size());
        std::vector<unsigned int> indices;
        indices.reserve(vertices.size());

        std::map<vec4, unsigned int, vec4_comparator> lookupMap;

        for (const auto &v: vertices) {
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

    // Graham scan implementation (first point should be start point / bounds max)
    std::vector<unsigned int>
    createConvexHull(const std::vector<std::pair<glm::dvec2, unsigned int>> &points, glm::dvec2 boundingCenter = {},
                     const std::weak_ptr<bool> &controlFlag = {}) {
        // Set comparison direction, the start of the polar angle circle, to be the first point
        const auto compareDir = normalize(points[0].first - boundingCenter);

        if (controlFlag.expired() || points.empty()) return {};

        std::vector<std::tuple<dvec2, double, unsigned int>> nonEmptyValuesPolarAngled;
        nonEmptyValuesPolarAngled.reserve(points.size());
        nonEmptyValuesPolarAngled.emplace_back(points.at(0).first, 0.0, points.at(0).second);
        for (auto it{points.begin() + 1}; it != points.end(); ++it) {
            const auto &[p, i] = *it;
            const auto dir = p - boundingCenter;
            const auto dir_len = length(dir);
            if (EPS < dir_len) {
                const auto pa = dotBetween(compareDir, dir * (1. / dir_len));
                nonEmptyValuesPolarAngled.emplace_back(p, pa, i);
            }
        }

        if (controlFlag.expired()) return {};

        // Sort points on polar angle
        std::sort(nonEmptyValuesPolarAngled.begin(), nonEmptyValuesPolarAngled.end(), [](const auto &a, const auto &b) {
            return std::get<1>(a) < std::get<1>(b);
        });

        if (controlFlag.expired()) return {};

        // Graham scan implementation:
        std::deque<std::pair<dvec2, unsigned int>> convexHullStack{};
        for (const auto&[p, polar_angle, i]: nonEmptyValuesPolarAngled) {
            // Skip empty/near-empty increments:
            if (!convexHullStack.empty()) {
                const auto last = convexHullStack[0].first;
                const auto b = p - last;

                if (length(b) < EPS)
                    continue;
            }

            while (2 <= convexHullStack.size()) {
                const auto last = convexHullStack[0].first;
                const auto second_last = convexHullStack[1].first;
                auto a = last - second_last;
                auto b = p - last;

                // If the new point forms a clockwise turn with the last points in the stack,
                // pop the last point from the stack
                if (!ccw(a, b, EPS))
                    // Only pop last edge if new edge is a "valid" edge
                    convexHullStack.pop_front();
                else
                    break;
            }

            // Since points are sorted on polar coordinates and empty increments are skipped,
            // no point that arrives here should give an empty increment

            // Only add points that has a connected edge (or if there are no edges yet)
            convexHullStack.emplace_front(p, i);

            if (controlFlag.expired()) return {};
        }

        // After adding all points, end might be concave:
        // pop end points until end -> start is convex
        const auto p = convexHullStack.back().first;
        while (2 <= convexHullStack.size()) {
            const auto last = convexHullStack[0].first;
            const auto second_last = convexHullStack[1].first;
            auto a = last - second_last;
            auto b = p - last;

            if (!ccw(a, b, EPS))
                convexHullStack.pop_front();
            else
                break;
        }

        return map(convexHullStack.begin(), convexHullStack.end(), [](const auto &h) { return h.second; });
    }

    std::optional<std::vector<vec4>>
    geometryPostProcessing(const std::vector<vec4> &vertices, const std::weak_ptr<bool> &controlFlag) {
        // If we at this point don't have a buffer, it means it got recreated somewhere in the meantime.
        // In which case we don't need this thread anymore.
        if (controlFlag.expired() || vertices.size() < 3)
            return std::nullopt;

        // Filter out empty vertices
        const auto filterEmptiesPoints = [](const auto &points) {
            std::vector<vec4> data;
            data.reserve(points.size());
            for (auto it{points.begin()};
                 it != points.end() && it + 1 != points.end() && it + 2 != points.end(); it += 3)
                if (std::all_of(it, it + 3, [](const auto &p) { return EPS < p.w; }))
                    data.insert(data.end(), it, it + 3);

            data.shrink_to_fit();
            return data;
        };

        return controlFlag.expired() ? std::nullopt : std::make_optional(filterEmptiesPoints(vertices));
    }

    std::optional<std::vector<uint>>
    getHexagonConvexHull(const std::vector<vec4> &vertices, const std::weak_ptr<bool> &controlFlag,
                         float upperThreshold, float lowerThreshold) {
        // If we at this point don't have a buffer, it means it got recreated somewhere in the meantime.
        // In which case we don't need this thread anymore.
        if (controlFlag.expired() || vertices.size() < 2)
            return std::nullopt;

        const auto hexPositions = getHexagonPositions(vertices);

        if (controlFlag.expired()) return std::nullopt;

        // Find the points which has a non-zero value of z (z height is hex-value, meaning empty ones are empty hexes)
        std::vector<std::pair<dvec2, uint>> nonEmptyValues;
        uint first = 0;
        dvec2 mi{std::numeric_limits<float>::max()}, ma{std::numeric_limits<float>::min()};
        nonEmptyValues.reserve(hexPositions.size());
        for (uint i{0}; i < hexPositions.size(); ++i) {
            const auto&[p, j] = hexPositions.at(i);
            const auto w = dvec2{p.x, p.y};

            if (upperThreshold + static_cast<float>(EPS) < p.z || p.z < lowerThreshold - static_cast<float>(EPS)) {
                mi = min(w, mi);
                ma = max(w, ma);

                nonEmptyValues.emplace_back(w, j);
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

        // Create convex hull from hexagon positions
        const auto convexHull = createConvexHull(nonEmptyValues, boundingCenter, controlFlag);

        return controlFlag.expired() ? std::nullopt : std::make_optional(convexHull);
    }
}