// iResolution, iGlobalTime (also as iTime), iTimeDelta, iFrame, iMouse, iMouseButton, 

#define PI 3.14159
#define MAX_STEPS 200
const float tanCoefficients = 0.5 * (PI / 180.0);

#iChannel2 "file://resources/worley_{}.png" // wildcard '{}' for filenames nx, px, ny, py, nz, pz
#iChannel2::Type "CubeMap"
#iChannel2::WrapMode "Repeat"

mat4 perspective(float n, float f, float aspect, float FOV) {
    float S = 1.0 / tan(FOV * tanCoefficients);

    return mat4(
        S, 0, 0, 0,
        0, S, 0, 0,
        0, 0, -f/(f-n), -1.0,
        0, 0, (-2.0 * f * n) / (f-n), 0
    );
}

vec4 makeQuat(float rad, vec3 axis) {
    return vec4(sin(rad * 0.5) * axis, cos(rad * 0.5));
}

vec4 mulQuat(vec4 q1, vec4 q2) {
    return vec4(
        cross(q1.xyz, q2.xyz) + q1.w * q2.xyz + q2.w * q1.xyz,
        q1.w * q2.w - dot(q1.xyz, q2.xyz)
    );
}

mat3 quatToRot(vec4 q) {
    return mat3(
        1.0 - 2.0*q.y*q.y - 2.0*q.z*q.z,2.0*q.x*q.y - 2.0*q.z*q.w,      2.0*q.x*q.z + 2.0*q.y*q.w,
        2.0*q.x*q.y + 2.0*q.z*q.w,      1.0 - 2.0*q.x*q.x - 2.0*q.z*q.z,2.0*q.y*q.z - 2.0*q.x*q.w,
        2.0*q.x*q.z - 2.0*q.y*q.w,      2.0*q.y*q.z + 2.0*q.x*q.w,      1.0 - 2.0*q.x*q.x - 2.0*q.y*q.y
    );
}

mat4 translate(mat4 mat, vec3 pos) {
    mat4 t = mat4(1.0);
    t[3].xyz = pos;
    return t * mat;
}

vec4 tf(vec3 p) {
    // return length(p-vec3(3.0, 1.6, 0.0)) < 4.0 ? vec4(1.0, 0.0, 0.0, 0.3) : vec4(vec3(1.0), 0.003);
    float r = length(p) < 0.06 ? 1.0 : 0.0;
    float d = texture(iChannel2, p).r * 2.0 * r / float(MAX_STEPS);
    return vec4(0.0, 0.7, 1.0, d);
}

void mainImage(out vec4 fragColor, in vec2 texCoords) {
    vec2 uv = 2.0 * texCoords / iResolution.xy - 1.0;
    vec2 mPos = 2.0 * iMouse.xy / iResolution.xy - 1.0;

    mat4 pMat = perspective(0.1, 100.0, iResolution.x / iResolution.y, 60.0);
    mat4 vMat = translate(mat4(quatToRot(
        mulQuat(
            makeQuat(-mPos.x * PI, vec3(0., 1.0, 0.)),
            makeQuat(mPos.y * PI, vec3(1., 0.0, 0.))
        )
    )), vec3(0., 0., -40.0));

    mat4 MVP = inverse(pMat * vMat);
    
    vec4 near = MVP * vec4(uv, -1.0, 1.0);
    near /= near.w;
    vec4 far = MVP * vec4(uv, 1.0, 1.0);
    far /= far.w;


    vec4 ro = vec4(near.xyz, 0.0);
    vec4 rd = vec4((far - near).xyz, 1.0);
    float farDist = length(rd.xyz);
    rd.xyz /= farDist;

    vec4 col = vec4(0.0);

    vec4 lightPos = MVP * vec4(0., 0., -1.0, 1.0);
    lightPos /= lightPos.w;

    for (int i = 0; i < MAX_STEPS; ++i) {
        vec3 p = ro.xyz + ro.w * rd.xyz;
        vec3 uv = p / farDist;

        vec4 d = tf(uv);

        float transmittance = 1.0;
        vec3 lightDir = lightPos.xyz - p;
        float lightDist = length(lightDir);
        lightDir /= lightDist;
        vec3 l_p;
        for (int j = 0; j < 10; ++j) {
            transmittance = clamp(transmittance - tf(l_p).a * pow(float(MAX_STEPS) / 18.0, 2.0), 0.0, 1.0);
            l_p += lightDir * (lightDist / float(10));
        }

        col.rgb += (1.0 - col.a) * d.rgb * 0.1 * transmittance;
        col.a += (1.0 - col.a) * d.a;

        if (1.0 <= col.a)
            break;

        ro.w += farDist / float(MAX_STEPS);
        if (farDist < ro.w)
            break;
    }

    col.a = min(col.a, 1.0);
    fragColor = vec4((1.0 - col.a) * rd.xyz + col.a * col.rgb, 1.0);
}