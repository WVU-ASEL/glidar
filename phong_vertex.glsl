#version 120

attribute vec3 vertex;
uniform mat4 mvp;

void main() {
  gl_TexCoord[0] = gl_MultiTexCoord0;

  // Set the position of the current vertex
  gl_Position = mvp * vec4(vertex, 1);
}