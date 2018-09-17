#version 450

const float KERNEL_RADIUS = 5;
  
uniform float sharpness = 32.0;
uniform vec2  offset; // either set x to 1/width or y to 1/height

layout(binding=0) uniform sampler2D ambientTexture;

layout(pixel_center_integer) in vec4 gl_FragCoord;
in vec4 gFragmentPosition;
out vec4 fragColor;


//-------------------------------------------------------------------------

float BlurFunction(vec2 uv, float r, float center_c, float center_d, inout float w_total)
{
  vec4  aoz = texture2D( ambientTexture, uv );
  float c = aoz.x;
  float d = aoz.w;
  
  const float BlurSigma = float(KERNEL_RADIUS) * 0.5;
  const float BlurFalloff = 1.0 / (2.0*BlurSigma*BlurSigma);
  
  float ddiff = (d - center_d) * sharpness;
  float w = exp2(-r*r*BlurFalloff - ddiff*ddiff);
  w_total += w;

  return c*w;
}

void main()
{
  vec4  ambient = texelFetch( ambientTexture, ivec2(gl_FragCoord.xy),0);
  
  float center_c = ambient.x;
  float center_d = ambient.w;
  
  float c_total = center_c;
  float w_total = 1.0;

  vec2 texCoord = (gFragmentPosition.xy+1.0)*0.5;
  
  for (float r = 1; r <= KERNEL_RADIUS; ++r)
  {
    vec2 uv = texCoord + offset * (r+0.5);
    c_total += BlurFunction(uv, r, center_c, center_d, w_total);  
  }
  
  for (float r = 1; r <= KERNEL_RADIUS; ++r)
  {
    vec2 uv = texCoord - offset * (r+0.5);
    c_total += BlurFunction(uv, r, center_c, center_d, w_total);  
  }
  
  fragColor = vec4(c_total/w_total,c_total/w_total,c_total/w_total,center_d);
}