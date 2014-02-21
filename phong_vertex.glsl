#version 120


attribute vec3 position;
attribute vec2 tex;
attribute vec3 normal;

varying vec2 tex_coord0;
varying vec3 normal0;

varying vec3 light_dir;


void main() {
  //gl_TexCoord[0] = gl_MultiTexCoord0;
  //tex_coord0 = tex;
  //normal0    = (gl_ModelViewMatrix * vec4(normal, 0.0)).xyz;
  //world_pos0 = (gl_ModelViewMatrix * vec4(position, 1.0)).xyz;

  // Set the position of the current vertex
  gl_Position = gl_ModelViewProjectionMatrix * vec4(position, 1.0);
  gl_TexCoord[0].st = tex;
  tex_coord0 = tex;
  normal0 = normalize(gl_NormalMatrix * normal);
  light_dir = normalize(vec3(gl_LightSource[0].position));
}