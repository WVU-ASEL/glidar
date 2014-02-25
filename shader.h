/*
 * Copyright (c) 2014, John O. Woods, Ph.D.
 *   West Virginia University Applied Space Exploration Lab
 *   West Virginia Robotic Technology Center
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */


#ifndef SHADER_H
#define SHADER_H

#include <fstream>

class Shader {
public:
  Shader() { }
  Shader(const char * vs_filename, const char * fs_filename) {
    init(vs_filename, fs_filename);
  }
  ~Shader() {
    glDetachShader(shader_id, fragment_shader);
    glDetachShader(shader_id, vertex_shader);

    glDeleteShader(fragment_shader);
    glDeleteShader(vertex_shader);
    glDeleteProgram(shader_id);
  }

  void init(const char * vs_filename, const char * fs_filename) {
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    std::string vertex_code, fragment_code;
    load(vs_filename, vertex_code);
    load(fs_filename, fragment_code);

    char const * vs_pointer = vertex_code.c_str();
    char const * fs_pointer = fragment_code.c_str();

    glShaderSource(vertex_shader, 1, &vs_pointer, NULL);
    glShaderSource(fragment_shader, 1, &fs_pointer, NULL);

    glCompileShader(vertex_shader);
    validate_shader(vertex_shader, vs_filename);

    glCompileShader(fragment_shader);
    validate_shader(fragment_shader, fs_filename);

    shader_id = glCreateProgram();

    glAttachShader(shader_id, fragment_shader);
    glAttachShader(shader_id, vertex_shader);
    glLinkProgram(shader_id);
    validate_program(shader_id);

  }

  void bind() {
    glUseProgram(shader_id);
  }
  void unbind() {
    glUseProgram(0);
  }

  GLuint id() { return shader_id; }

private:
  void validate_shader(GLuint shader, const char * filename = NULL) {
    const unsigned int BUFFER_SIZE = 512;
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    GLsizei length = 0;

    glGetShaderInfoLog(shader, BUFFER_SIZE, &length, buffer);
    if (length > 0)
      std::cerr << "Shader " << shader << "(" << (filename?filename:"") << ") compile error: " << buffer << std::endl;
  }


  void validate_program(GLuint program) {
    const unsigned int BUFFER_SIZE = 512;
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    GLsizei length = 0;

    glGetProgramInfoLog(program, BUFFER_SIZE, &length, buffer);
    if (length > 0)
      std::cerr << "Program " << program << " link error: " << buffer << std::endl;

    glValidateProgram(program);
    GLint status;
    glGetProgramiv(program, GL_VALIDATE_STATUS, &status);
    if (status == GL_FALSE)
      std::cerr << "Error validating shader " << program << std::endl;
  }


  bool load(const char * path, std::string& shader_code) {
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

  GLuint shader_id;
  GLuint vertex_shader;
  GLuint fragment_shader;
};

#endif // SHADER_H