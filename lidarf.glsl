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

float rand(vec2 co) {
  return fract(sin(dot(co.xy,vec2(12.9898,78.233))) * 43758.5453);
}

void main() {
  const float GLOBAL_AMBIENT = 0.2;
  float n_dot_l, n_dot_hv, att;
  vec3 cf;
  vec4 color = gl_FrontMaterial.emission * texture2D(texture_color, gl_TexCoord[0].st);
  float spot_effect;
  float SHININESS = 128.0;
  float CATT = 0.9, LATT = 0.01, QATT = 0.01, SPOT_EXP = 0.01;

  vec3 light_dir0 = vec3(0.0, 0.0, camera_z) - ec_pos;

  float dist = length(normalize(light_dir0)) - rand(ec_pos.xy) * 10;// - rand(ec_pos.xy) * length(light_dir0) * 0.95;

  n_dot_l = max(dot(normal0, normalize(light_dir0)), 0.0);

  color += GLOBAL_AMBIENT * gl_FrontMaterial.ambient;


  if (n_dot_l > 0.0) {

    spot_effect = dot(normalize(light_dir), normalize(light_dir0));

    if (spot_effect > cos(radians(10))) {
      spot_effect = pow(spot_effect, SPOT_EXP);
      att = spot_effect / (gl_LightSource[0].constantAttenuation + gl_LightSource[0].linearAttenuation*dist + gl_LightSource[0].quadraticAttenuation*dist*dist);


      color += att * (n_dot_l * diffuse + ambient); //diffuse * n_dot_l;

      n_dot_hv = max(dot(normal0,half_vector), 0.0);
      color += att * gl_FrontMaterial.specular * specular * pow(n_dot_hv, gl_FrontMaterial.shininess); // gl_FrontMaterial.specular * specular; //

      color.b = (dist + camera_z) / far_plane;
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