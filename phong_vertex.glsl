#version 120

layout(location=0) in vec3 vertex;

void main() {
  //gl_TexCoord[0] = gl_MultiTexCoord0;

  // Set the position of the current vertex
  gl_Position = gl_ModelViewProjectionMatrix * vertex;
}