#version 120

// Input vertex data, different for all executions of this shader.
attribute vec3 vertex_position_model_space;

// Values that stay constant for the whole mesh.
uniform mat4 mvp;

void main() {
  // Output position of the vertex in clip space : MVP * position
  gl_Position = mvp * vec4(vertex_position_model_space, 1);
}