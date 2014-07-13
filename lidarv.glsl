#version 120

attribute vec3 position;
attribute vec2 diffuse_tex;
attribute vec2 specular_tex;
attribute vec3 normal;

//uniform float camera_z;

varying vec3 normal0;

varying vec3 light_dir;

//varying vec4  emission;
varying vec4  diffuse;
varying vec4  ambient;
varying vec4  specular;
varying vec3  half_vector;
varying vec3  ec_pos;

void main() {
  float n_dot_l;
  //specular = vec4(1.0, 1.0, 1.0, 1.0);

  normal0 = normalize(gl_NormalMatrix * normal);

  light_dir = normalize(gl_LightSource[0].position.xyz);

  ec_pos = vec3(gl_ModelViewMatrix * vec4(position, 1.0));

  half_vector = gl_LightSource[0].halfVector.xyz;

  gl_TexCoord[0].st = diffuse_tex; //gl_MultiTexCoord0;
  gl_TexCoord[1].st = specular_tex; //gl_MultiTexCoord1;

  //emission = vec4(1.0, 1.0, 1.0, 1.0);
  diffuse  = gl_FrontMaterial.diffuse  * gl_LightSource[0].diffuse;
  specular = gl_FrontMaterial.specular * gl_LightSource[0].specular;
  ambient  = gl_FrontMaterial.ambient  * gl_LightSource[0].ambient;

  n_dot_l = max(dot(normal0, light_dir), 0.0);

  gl_Position = gl_ModelViewProjectionMatrix * vec4(position, 1.0);
}