#version 120

// ganked from: https://code.google.com/p/opengl-tutorial-org/source/browse/tutorial02_red_triangle/SimpleVertexShader.vertexshader?name=2.1%20branch

// Input vertex data, different for all executions of this shader.
attribute vec3 vertex;

void main(){

  // vertex_position_model_space is size 3, and represents x,y,z. 1.0 is for w.
  gl_Position = vec4(vertex, 1.0);

}