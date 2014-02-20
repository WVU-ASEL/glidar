#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>

#include <GL/glew.h>

#include "shader.h"


bool load_shader(const char * path, std::string& shader_code) {
  std::ifstream shader_stream(path, std::ios::in);

  if (shader_stream.is_open()) {
    std::string line = "";
    while (std::getline(shader_stream, line)) shader_code += "\n" + line;
    shader_stream.close();
  } else {
    std::cerr << "Impossible to open " << path << ". Are you sure you're in the right directory?" << std::endl;
    return false;
  }
  return true;
}


void compile_and_check_shader(const char* path, const std::string& shader_code, GLuint shader_id, GLint& result) {

  // compile vertex shader
  std::cout << "compiling shader: " << path << std::endl;

  char const * source_pointer = shader_code.c_str();
  int info_log_length = 0;

  glShaderSource(shader_id, 1, &source_pointer, NULL);
  glCompileShader(shader_id);

  // check vertex shader
  glGetShaderiv(shader_id, GL_COMPILE_STATUS, &result);
  glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &info_log_length);

  std::string error_message(info_log_length, ' ');
  glGetShaderInfoLog(shader_id, info_log_length, NULL, (char*)(&error_message));

  if (info_log_length > 0)
    std::cerr << error_message << std::endl;
}


GLuint load_shaders(const char * vertex_file_path, const char * fragment_file_path) {

  // Create the shaders
  GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
  GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

  // Read the vertex and fragment shader code from the files
  std::string vertex_shader_code, fragment_shader_code;
  if (!load_shader(vertex_file_path, vertex_shader_code))     return 0;
  if (!load_shader(fragment_file_path, fragment_shader_code)) return 0;

  GLint result = GL_FALSE;

  compile_and_check_shader(vertex_file_path, vertex_shader_code, vertex_shader_id, result);
  compile_and_check_shader(fragment_file_path, fragment_shader_code, fragment_shader_id, result);

  // Link the program
  std::cout << "Linking" << std::endl;
  GLuint program_id = glCreateProgram();
  glAttachShader(program_id, vertex_shader_id);
  glAttachShader(program_id, fragment_shader_id);
  glLinkProgram(program_id);

  // Check the program
  glGetProgramiv(program_id, GL_LINK_STATUS, &result);
  int info_log_length = 0;
  glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_length);
  std::string error_message(info_log_length, ' ');
  glGetProgramInfoLog(program_id, info_log_length, NULL, (char*)(&error_message));

  if (info_log_length > 0)
    std::cerr << error_message << std::endl;

  std::cout << "Done" << std::endl;

  // Not quite sure why we delete these
  glDeleteShader(vertex_shader_id);
  glDeleteShader(fragment_shader_id);

  return program_id;
}