const float PI = 3.1415926;
const float HEX_ANGLE = PI / 180.0 * 60.0;
const float bufferAccumulationFactor = 100.0; // Uniform constant in hexagon-tile shaders (could be a define)
const float EPSILON = 0.001;
const uint hexValueIntMax = 1000000; // The higher the number, the more accurate (max 32-bit uint)
const ivec2 NEIGHBORS[6] = ivec2[6](
    ivec2(1, 1), ivec2(0, 2), ivec2(-1, 1),
    ivec2(-1,-1), ivec2(0, -2), ivec2(1, -1)
);

uint getVertexCount(uint count) {
    return count * 6 * 2 * 3; // hex_count * hex_size * (inner+outer) * triangle_size
}