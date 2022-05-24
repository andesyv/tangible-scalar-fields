float random(vec2 st) {
    return fract(sin(dot(st.xy,
                         vec2(12.9898,78.233)))*
        43758.5453123);
}

vec2 ranp(vec2 p) {
    float r = random(p);
    return vec2(r, 1.0 - fract(r));
}

float worley(vec2 p) {
    p *= 9.;
    float dist = 100.0;
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            vec2 i_uv = floor(p);
            vec2 f_uv = fract(p);
            vec2 point = ranp(i_uv + vec2(x, y));
            dist = min(dist, length(point - f_uv + vec2(x, y)));
        }
    }
    return dist;
}

float rand3d(vec3 p) {
    return fract(sin(dot(p,
                         vec3(12.9898,78.233, 34.8731)))*
        43758.5453123);
}

float worley3d(vec3 p) {
    p *= 9.;
    vec3 i_uv = floor(p);
    vec3 f_uv = fract(p);
    float dist = 100.0;
    for (int z = -1; z <= 1; ++z) {
        for (int y = -1; y <= 1; ++y) {
            for (int x = -1; x <= 1; ++x) {
                float r = rand3d(i_uv + vec3(x, y, z));
                vec3 point = vec3(r, 1.0 - fract(r), fract(r * 3.));
                dist = min(dist, length(point - f_uv + vec3(x, y, z)));
            }
        }
    }
    return dist;
}

float noise3d(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);

    // 8 end points of square
    float e[8] = float[8](
        rand3d(i),
        rand3d(i + vec3(1.0, 0.0, 0.0)),
        rand3d(i + vec3(0.0, 1.0, 0.0)),
        rand3d(i + vec3(1.0, 1.0, 0.0)),
        rand3d(i + vec3(0.0, 0.0, 1.0)),
        rand3d(i + vec3(1.0, 0.0, 1.0)),
        rand3d(i + vec3(0.0, 1.0, 1.0)),
        rand3d(i + vec3(1.0, 1.0, 1.0))
    );

    vec3 u = smoothstep(0., 1., f);

    return e[0] * (1.0 - u.x) * (1.0 - u.y) * (1.0 - u.z) +
            e[1] * u.x * (1.0 - u.y) * (1.0 - u.z) +
            e[2] * (1.0 - u.x) * u.y * (1.0 - u.z) +
            e[3] * u.x * u.y * (1.0 - u.z) + 
            e[4] * (1.0 - u.x) * (1.0 - u.y) * u.z +
            e[5] * u.x * (1.0 - u.y) * u.z +
            e[6] * (1.0 - u.x) * u.y * u.z +
            e[7] * u.x * u.y * u.z;
}

// Random hashing function
vec2 rand_dir(vec2 p) {
    return fract(vec2(
        sin(dot(p, vec2(12.9898,78.233))),
        cos(dot(p, vec2(34.8731, 68.15872) * 123.02392))
    ) * 43758.5453123);
}

vec2 hash( vec2 x )  // replace this by something better
{
    const vec2 k = vec2( 0.3183099, 0.3678794 );
    x = x*k + k.yx;
    return -1.0 + 2.0*fract( 16.0 * k*fract( x.x*x.y*(x.x+x.y)) );
}

float gradient_noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);

    vec2 u = smoothstep(0., 1., f);

    // vec2 e[4] = vec2[4](
    //     hash(i),
    //     hash(i + vec2(1.0, 0.0)),
    //     hash(i + vec2(0.0, 1.0)),
    //     hash(i + vec2(1.0, 1.0))
    // );

    return mix(
        mix(
            dot(hash(i + vec2(0., 0.)), f - vec2(0., 0.)),
            dot(hash(i + vec2(1., 0.)), f - vec2(1., 0.)),
            u.x
        ),
        mix(
            dot(hash(i + vec2(0., 1.)), f - vec2(0., 1.)),
            dot(hash(i + vec2(1., 1.)), f - vec2(1., 1.)),
            u.x
        ),
        u.y
    );
}

vec3 mod289(vec3 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec2 mod289(vec2 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec3 permute(vec3 x) {
  return mod289(((x*34.0)+1.0)*x);
}

//
// Description : Array and textureless GLSL 2D simplex noise function.
//      Author : Ian McEwan, Ashima Arts.
//  Maintainer : stegu
//     Lastmod : 20110822 (ijm)
//     License : Copyright (C) 2011 Ashima Arts. All rights reserved.
//               Distributed under the MIT License. See LICENSE file.
//               https://github.com/ashima/webgl-noise
//               https://github.com/stegu/webgl-noise
// 

float snoise(vec2 v)
  {
  const vec4 C = vec4(0.211324865405187,  // (3.0-sqrt(3.0))/6.0
                      0.366025403784439,  // 0.5*(sqrt(3.0)-1.0)
                     -0.577350269189626,  // -1.0 + 2.0 * C.x
                      0.024390243902439); // 1.0 / 41.0
// First corner
  vec2 i  = floor(v + dot(v, C.yy) );
  vec2 x0 = v -   i + dot(i, C.xx);

// Other corners
  vec2 i1;
  //i1.x = step( x0.y, x0.x ); // x0.x > x0.y ? 1.0 : 0.0
  //i1.y = 1.0 - i1.x;
  i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
  // x0 = x0 - 0.0 + 0.0 * C.xx ;
  // x1 = x0 - i1 + 1.0 * C.xx ;
  // x2 = x0 - 1.0 + 2.0 * C.xx ;
  vec4 x12 = x0.xyxy + C.xxzz;
  x12.xy -= i1;

// Permutations
  i = mod289(i); // Avoid truncation effects in permutation
  vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 ))
		+ i.x + vec3(0.0, i1.x, 1.0 ));

  vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);
  m = m*m ;
  m = m*m ;

// Gradients: 41 points uniformly over a line, mapped onto a diamond.
// The ring size 17*17 = 289 is close to a multiple of 41 (41*7 = 287)

  vec3 x = 2.0 * fract(p * C.www) - 1.0;
  vec3 h = abs(x) - 0.5;
  vec3 ox = floor(x + 0.5);
  vec3 a0 = x - ox;

// Normalise gradients implicitly by scaling m
// Approximation of: m *= inversesqrt( a0*a0 + h*h );
  m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );

// Compute final noise value at P
  vec3 g;
  g.x  = a0.x  * x0.x  + h.x  * x0.y;
  g.yz = a0.yz * x12.xz + h.yz * x12.yw;
  return 130.0 * dot(m, g);
}


//
// Description : Array and textureless GLSL 2D/3D/4D simplex 
//               noise functions.
//      Author : Ian McEwan, Ashima Arts.
//  Maintainer : stegu
//     Lastmod : 20201014 (stegu)
//     License : Copyright (C) 2011 Ashima Arts. All rights reserved.
//               Distributed under the MIT License. See LICENSE file.
//               https://github.com/ashima/webgl-noise
//               https://github.com/stegu/webgl-noise
// 

vec4 mod289(vec4 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x) {
     return mod289(((x*34.0)+1.0)*x);
}

vec4 taylorInvSqrt(vec4 r)
{
  return 1.79284291400159 - 0.85373472095314 * r;
}

float snoise(vec3 v)
  { 
  const vec2  C = vec2(1.0/6.0, 1.0/3.0) ;
  const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);

// First corner
  vec3 i  = floor(v + dot(v, C.yyy) );
  vec3 x0 =   v - i + dot(i, C.xxx) ;

// Other corners
  vec3 g = step(x0.yzx, x0.xyz);
  vec3 l = 1.0 - g;
  vec3 i1 = min( g.xyz, l.zxy );
  vec3 i2 = max( g.xyz, l.zxy );

  //   x0 = x0 - 0.0 + 0.0 * C.xxx;
  //   x1 = x0 - i1  + 1.0 * C.xxx;
  //   x2 = x0 - i2  + 2.0 * C.xxx;
  //   x3 = x0 - 1.0 + 3.0 * C.xxx;
  vec3 x1 = x0 - i1 + C.xxx;
  vec3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y
  vec3 x3 = x0 - D.yyy;      // -1.0+3.0*C.x = -0.5 = -D.y

// Permutations
  i = mod289(i); 
  vec4 p = permute( permute( permute( 
             i.z + vec4(0.0, i1.z, i2.z, 1.0 ))
           + i.y + vec4(0.0, i1.y, i2.y, 1.0 )) 
           + i.x + vec4(0.0, i1.x, i2.x, 1.0 ));

// Gradients: 7x7 points over a square, mapped onto an octahedron.
// The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
  float n_ = 0.142857142857; // 1.0/7.0
  vec3  ns = n_ * D.wyz - D.xzx;

  vec4 j = p - 49.0 * floor(p * ns.z * ns.z);  //  mod(p,7*7)

  vec4 x_ = floor(j * ns.z);
  vec4 y_ = floor(j - 7.0 * x_ );    // mod(j,N)

  vec4 x = x_ *ns.x + ns.yyyy;
  vec4 y = y_ *ns.x + ns.yyyy;
  vec4 h = 1.0 - abs(x) - abs(y);

  vec4 b0 = vec4( x.xy, y.xy );
  vec4 b1 = vec4( x.zw, y.zw );

  //vec4 s0 = vec4(lessThan(b0,0.0))*2.0 - 1.0;
  //vec4 s1 = vec4(lessThan(b1,0.0))*2.0 - 1.0;
  vec4 s0 = floor(b0)*2.0 + 1.0;
  vec4 s1 = floor(b1)*2.0 + 1.0;
  vec4 sh = -step(h, vec4(0.0));

  vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
  vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;

  vec3 p0 = vec3(a0.xy,h.x);
  vec3 p1 = vec3(a0.zw,h.y);
  vec3 p2 = vec3(a1.xy,h.z);
  vec3 p3 = vec3(a1.zw,h.w);

//Normalise gradients
  vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
  p0 *= norm.x;
  p1 *= norm.y;
  p2 *= norm.z;
  p3 *= norm.w;

// Mix final noise value
  vec4 m = max(0.5 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
  m = m * m;
  return 105.0 * dot( m*m, vec4( dot(p0,x0), dot(p1,x1), 
                                dot(p2,x2), dot(p3,x3) ) );
  }


void mainImage(out vec4 fragColor, in vec2 texCoords) {
    vec2 uv = texCoords / iResolution.xy;
    vec2 mousePos = iMouse.xy / iResolution.xy;
    float exponent = mix(1.0, 12.0, 1.0 - mousePos.y);
    const float freq = 3.4;

    vec3 p = vec3(uv, 0.);

    // fragColor = vec4(vec3((1.0 - worley3d(p)) * noise3d(p * 10.0)), 1.);
    // fragColor = vec4(vec3(noise3d(p * sin(iTime * 0.1) * 100.)), 1.0);
    float v = snoise(vec3(uv * 0.23 * freq, mousePos.x));
    float v2 = snoise(vec3(uv * 1.73 * freq, mousePos.x * 2.3));
    float v3 = snoise(vec3(uv * 0.64 * freq, mousePos.x * 12.8));
    v = 0.5 + 0.5 * (v * 1.9 + v2 * 1.3 + v3 * 1.9) / 3.0;
    v = 2.0 * pow(clamp(1.0 - v, 0.0, 1.0), exponent) - 1.0;

    vec2 p_2d = uv * 2.0 - 1.0;
    float b = pow(mix(0.0, 1.0, 1.0 - abs(p_2d.x)) * mix(0.0, 1.0, 1.0 - abs(p_2d.y)), 0.01);
    float r = pow(clamp(1.0 - 0.3 * dot(p_2d, p_2d), 0.0, 1.0), 2.3);
    fragColor = vec4(vec3(0.5 * v + 0.5), 1.);
}