#version 120

attribute vec3 position;
attribute vec2 tex;
attribute vec3 normal;

uniform float camera_z;

varying vec3 normal0;

varying vec3 light_dir;

varying vec4  diffuse;
varying vec4  ambient;
varying vec4  specular;
varying vec3  half_vector;

void main() {
  float n_dot_l;
  const vec4 SPOTLIGHT_DIFFUSE = vec4(0.2, 0.2, 0.2, 1.0);
  const vec4 SPOTLIGHT_AMBIENT = vec4(0.2, 0.2, 0.2, 1.0);
  specular = vec4(0.1, 0.1, 0.1, 1.0);

  normal0 = normalize(gl_NormalMatrix * normal);

  light_dir = normalize(vec3(0.0, 0.0, 1.0));

  half_vector = normalize(light_dir + light_dir);

  gl_TexCoord[0].st = tex;

  diffuse = gl_FrontMaterial.diffuse * SPOTLIGHT_DIFFUSE;
  ambient = gl_FrontMaterial.ambient * SPOTLIGHT_AMBIENT;
  //ambient += gl_LightModel.ambient * gl_FrontMaterial.ambient;

  n_dot_l = max(dot(normal0, light_dir), 0.0);

  gl_Position = gl_ModelViewProjectionMatrix * vec4(position, 1.0);

  //cos_angle = dot(normalize(position.xyz), normalize(light_dir.xyz));
}