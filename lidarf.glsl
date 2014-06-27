#version 120

varying vec3 normal0;

varying vec3 light_dir;

varying vec4  diffuse;
varying vec4  ambient;
varying vec4  specular;

varying vec3 half_vector;
varying vec3 ec_pos;

uniform float camera_z;
uniform float far_plane;
uniform float near_plane;

uniform sampler2D diffuse_texture_color;
uniform sampler2D specular_texture_color;

// Random number generator without any real testing done.
// Comes from: http://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl
//float rand(vec2 co) {
//  return fract(sin(dot(co.xy,vec2(12.9898,78.233))) * 43758.5453);
//}

float rand_positive(vec2 co) {
  return noise1(co) * 0.5 + 0.5;
}

float rand_0_mean(vec2 co) {
  return noise1(co*2);
}

void main() {
  const float GLOBAL_AMBIENT = 0.2;
  vec4 color = vec4((gl_FrontMaterial.emission * texture2D(diffuse_texture_color, gl_TexCoord[0].st)).r, 0.0, 0.0, 1.0);
  float bounce = (gl_FrontMaterial.emission * texture2D(specular_texture_color, gl_TexCoord[1].st)).r * rand_positive(gl_FragCoord.xy);

  vec3 light_dir0 = gl_LightSource[0].position.xyz - ec_pos;

  float dist = length(light_dir0);

  float n_dot_l = max(dot(normal0, normalize(light_dir0)), 0.0);

  color = vec4(color.r + GLOBAL_AMBIENT * gl_FrontMaterial.ambient.r, 0.0, 0.0, 1.0);

  if (n_dot_l > 0.0) {

    // Calculate the angle w.r.t. the spotlight.
    float spot_effect = dot(normalize(light_dir), normalize(light_dir0));

    if (spot_effect > gl_LightSource[0].spotCosCutoff) {
      spot_effect = pow(spot_effect, gl_LightSource[0].spotExponent);
      float att = spot_effect / (gl_LightSource[0].constantAttenuation + gl_LightSource[0].linearAttenuation*dist + gl_LightSource[0].quadraticAttenuation*dist*dist);

      color += att * (n_dot_l * diffuse + ambient);

      float n_dot_hv = max(dot(normal0,half_vector), 0.0);
      color += att * gl_FrontMaterial.specular * specular * pow(n_dot_hv, gl_FrontMaterial.shininess);

      color.r *= rand_positive(gl_FragCoord.xy);

      float dist_ratio = 65536.0f * (1.0 + 0.1 * bounce) * (dist - near_plane) / (far_plane - near_plane);
      color.g = floor(dist_ratio / 256.0f) / 256.0;
      color.b = floor(mod(dist_ratio, 256.0f)) / 256.0;
      color.a = 1.0;
    } else {
      color = vec4(0.0,0.0,0.0,1.0);
    }
  } else {
    color = vec4(0.0,0.0,0.0,1.0);
  }

  gl_FragColor = color;
  //gl_FragDepth = dist;
}