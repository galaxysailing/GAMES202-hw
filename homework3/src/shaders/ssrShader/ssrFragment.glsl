#ifdef GL_ES
precision highp float;
#endif

uniform vec3 uLightDir;
uniform vec3 uCameraPos;
uniform vec3 uLightRadiance;
uniform sampler2D uGDiffuse;
uniform sampler2D uGDepth;
uniform sampler2D uGNormalWorld;
uniform sampler2D uGShadow;
uniform sampler2D uGPosWorld;

varying mat4 vWorldToScreen;
varying highp vec4 vPosWorld;

#define M_PI 3.1415926535897932384626433832795
#define TWO_PI 6.283185307
#define INV_PI 0.31830988618
#define INV_TWO_PI 0.15915494309

float Rand1(inout float p) {
  p = fract(p * .1031);
  p *= p + 33.33;
  p *= p + p;
  return fract(p);
}

vec2 Rand2(inout float p) {
  return vec2(Rand1(p), Rand1(p));
}

float InitRand(vec2 uv) {
	vec3 p3  = fract(vec3(uv.xyx) * .1031);
  p3 += dot(p3, p3.yzx + 33.33);
  return fract((p3.x + p3.y) * p3.z);
}

vec3 SampleHemisphereUniform(inout float s, out float pdf) {
  vec2 uv = Rand2(s);
  float z = uv.x;
  float phi = uv.y * TWO_PI;
  float sinTheta = sqrt(1.0 - z*z);
  vec3 dir = vec3(sinTheta * cos(phi), sinTheta * sin(phi), z);
  pdf = INV_TWO_PI;
  return dir;
}

vec3 SampleHemisphereCos(inout float s, out float pdf) {
  vec2 uv = Rand2(s);
  float z = sqrt(1.0 - uv.x);
  float phi = uv.y * TWO_PI;
  float sinTheta = sqrt(uv.x);
  vec3 dir = vec3(sinTheta * cos(phi), sinTheta * sin(phi), z);
  pdf = z * INV_PI;
  return dir;
}

void LocalBasis(vec3 n, out vec3 b1, out vec3 b2) {
  float sign_ = sign(n.z);
  if (n.z == 0.0) {
    sign_ = 1.0;
  }
  float a = -1.0 / (sign_ + n.z);
  float b = n.x * n.y * a;
  b1 = vec3(1.0 + sign_ * n.x * n.x * a, sign_ * b, -sign_ * n.x);
  b2 = vec3(b, sign_ + n.y * n.y * a, -n.y);
}

vec4 Project(vec4 a) {
  return a / a.w;
}

float GetDepth(vec3 posWorld) {
  float depth = (vWorldToScreen * vec4(posWorld, 1.0)).w;
  // vec4 p = vWorldToScreen * vec4(posWorld, 1.0);
  // float depth = p.z / p.w;
  return depth;
}

/*
 * Transform point from world space to screen space([0, 1] x [0, 1])
 *
 */
vec2 GetScreenCoordinate(vec3 posWorld) {
  vec2 uv = Project(vWorldToScreen * vec4(posWorld, 1.0)).xy * 0.5 + 0.5;
  return uv;
}

float GetGBufferDepth(vec2 uv) {
  float depth = texture2D(uGDepth, uv).x;
  if (depth < 1e-2) {
    depth = 1000.0;
  }
  return depth;
}

vec3 GetGBufferNormalWorld(vec2 uv) {
  vec3 normal = texture2D(uGNormalWorld, uv).xyz;
  return normal;
}

vec3 GetGBufferPosWorld(vec2 uv) {
  vec3 posWorld = texture2D(uGPosWorld, uv).xyz;
  return posWorld;
}

float GetGBufferuShadow(vec2 uv) {
  float visibility = texture2D(uGShadow, uv).x;
  return visibility;
}

vec3 GetGBufferDiffuse(vec2 uv) {
  vec3 diffuse = texture2D(uGDiffuse, uv).xyz;
  diffuse = pow(diffuse, vec3(2.2));
  return diffuse;
}

/*
 * Evaluate diffuse bsdf value.
 *
 * wi, wo are all in world space.
 * uv is in screen space, [0, 1] x [0, 1].
 *
 */
vec3 EvalDiffuse(vec3 wi, vec3 wo, vec2 uv) {
  vec3 kd = GetGBufferDiffuse(uv);
  vec3 N = normalize(GetGBufferNormalWorld(uv));
  float costheta = max(0.0, dot(wi, N));
  // vec3 L = vec3(0.0);
  return kd * INV_PI * costheta;
}

/*
 * Evaluate directional light with shadow map
 * uv is in screen space, [0, 1] x [0, 1].
 *
 */
vec3 EvalDirectionalLight(vec2 uv) {
  // vec3 Le = vec3(0.0);
  vec3 Le = uLightRadiance * GetGBufferuShadow(uv);
  return Le;
}

#define NAVIE_RAY_MARCH

bool RayMarch(vec3 ori, vec3 dir, out vec3 hitPos) {
#ifdef NAVIE_RAY_MARCH
  // TODO
  float _step = 0.04;
  vec3 p = ori;
  for(int i = 0; i < 200; ++i){
    p = p + dir * _step;
    vec2 uv = GetScreenCoordinate(p);
    if(uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0){
      break;
    }
    float curDepth = GetDepth(p);
    float texDepth = GetGBufferDepth(uv);
    if(curDepth > texDepth){
      hitPos = p - dir * _step * 0.5;
      return true;
    }
  }
#endif

  return false;
}

vec3 DirLight(vec2 uv){
  // vec2 screen_texcoord = GetScreenCoordinate(vPosWorld.xyz);
  vec3 wi = normalize(uLightDir), wo = vec3(0.0);

  vec3 brdf = EvalDiffuse(wi, wo, uv);
  vec3 Le = EvalDirectionalLight(uv);

  vec3 res = brdf * Le;
  // vec3 res = vec3(0.0);
  return res;
}

#define SAMPLE_NUM 1

vec3 inDirLight(vec2 init_uv, float s){
  vec3 indirL = vec3(0.0);

  vec3 pos = vPosWorld.xyz;
  vec2 screen_texcoord0 = init_uv;
  vec3 normal = normalize(GetGBufferNormalWorld(screen_texcoord0));

  vec3 viewDir = normalize(pos - uCameraPos);
  for(int i = 0; i < SAMPLE_NUM; ++i){
    float pdf;
    s += pos.x * pos.y + pos.z + pos.x;
    vec3 tdir = SampleHemisphereUniform(s, pdf), dir;
    vec3 t, b;
    LocalBasis(normal, t, b);
    t = normalize(t);
    b = normalize(b);
    dir.x = dot(b, tdir);
    dir.y = dot(t, tdir);
    dir.z = dot(normal, tdir);
    dir = normalize(dir);

    // vec3 dir = reflect(viewDir, normal);
    
    vec3 hitPos;
    if(RayMarch(pos, dir, hitPos)){
      vec3 wo;
      vec2 screen_texcoord1 = GetScreenCoordinate(hitPos);
      indirL += (EvalDiffuse(dir, wo, screen_texcoord0) 
        * EvalDiffuse(-dir, wo, screen_texcoord1)
        * EvalDirectionalLight(screen_texcoord1)) / pdf;

      // indirL = (EvalDiffuse(dir, wo, screen_texcoord0) 
      //   * EvalDiffuse(-dir, wo, screen_texcoord1) * M_PI * M_PI
      //   * EvalDirectionalLight(screen_texcoord1));

      // indirL = vec3(1.0);
      // indirL = DirLight(screen_texcoord1);
    }
  }

  if(SAMPLE_NUM > 0){
    indirL /= float(SAMPLE_NUM);
  }
  return indirL;
}

void main() {
  float s = InitRand(gl_FragCoord.xy);

  // vec3 L = vec3(0.0), N;
  // L = GetGBufferDiffuse(GetScreenCoordinate(vPosWorld.xyz));
  // vec3 color = pow(clamp(L, vec3(0.0), vec3(1.0)), vec3(1.0 / 2.2));
  // gl_FragColor = vec4(vec3(color.rgb), 1.0);
  
  vec2 screen_texcoord = GetScreenCoordinate(vPosWorld.xyz);
  // vec3 res = DirLight(screen_texcoord);

  vec3 dirL = DirLight(screen_texcoord);
  vec3 res =  dirL + inDirLight(screen_texcoord, s + dirL.r * dirL.g + dirL.b);
  // vec3 res = inDirLight(screen_texcoord, s + dirL.r * dirL.g + dirL.b);
  gl_FragColor = vec4(pow(res, vec3(0.454545)), 1.0);
  // gl_FragColor = vec4(res, 1.0);


}
