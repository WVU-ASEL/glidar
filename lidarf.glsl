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

uniform sampler2D texture_color;

// Random number generator without any real testing done.
// Comes from: http://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl
//float rand(vec2 co) {
//  return fract(sin(dot(co.xy,vec2(12.9898,78.233))) * 43758.5453);
//}

float rand(vec2 co) {
  return noise1(co) * 0.5 + 0.5;
}

void main() {
  const float GLOBAL_AMBIENT = 0.2;
  float n_dot_l, n_dot_hv, att;
  vec3 cf;
  vec4 color = gl_FrontMaterial.emission * texture2D(texture_color, gl_TexCoord[0].st);
  float spot_effect;

  vec3 light_dir0 = gl_LightSource[0].position.xyz - ec_pos;

  float dist = length(light_dir0); // - rand(ec_pos.xy) * 10;// - rand(ec_pos.xy) * length(light_dir0) * 0.95;

  n_dot_l = max(dot(normal0, normalize(light_dir0)), 0.0);

  color += GLOBAL_AMBIENT * gl_FrontMaterial.ambient;

  if (n_dot_l > 0.0) {

    spot_effect = dot(normalize(light_dir), normalize(light_dir0));

    if (spot_effect > gl_LightSource[0].spotCosCutoff) {
      spot_effect = pow(spot_effect, gl_LightSource[0].spotExponent);
      att = spot_effect / (gl_LightSource[0].constantAttenuation + gl_LightSource[0].linearAttenuation*dist + gl_LightSource[0].quadraticAttenuation*dist*dist);

      color += att * (n_dot_l * diffuse + ambient); //diffuse * n_dot_l;

      n_dot_hv = max(dot(normal0,half_vector), 0.0);
      color += att * gl_FrontMaterial.specular * specular * pow(n_dot_hv, gl_FrontMaterial.shininess); // gl_FrontMaterial.specular * specular; //

      color.b = dist / far_plane; //(dist * (1.0 - 0.05*rand(ec_pos.xy))) / far_plane;
      color.a = 1.0;
    } else {
      color.a = 1.0;
    }
  } else {
    color.a = 1.0;
  }

  color.g = 0;
  //color.b = dist;

  gl_FragColor = color;
}