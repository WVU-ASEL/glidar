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

/** Class which handles shader programs.
 *
 */
class Shader {
public:
  /** Initialize a new (empty) shader program.
   */
  Shader() { }

  /** Constructor, which basically just calls init().
   *
   * @param[in] Vertex shader program filename.
   * @param[in] Fragment shader program filename.
   */ 
  Shader(const char * vs_filename, const char * fs_filename) {
    init(vs_filename, fs_filename);
  }

  /** Unregister shaders from OpenGL (cleanup).
   *
   */
  ~Shader() {
    glDetachShader(shader_id, fragment_shader);
    glDetachShader(shader_id, vertex_shader);

    glDeleteShader(fragment_shader);
    glDeleteShader(vertex_shader);
    glDeleteProgram(shader_id);
  }


  /** Initialize a shader program using a vertex shader and a fragment shader (called directly by constructor).
   *
   * @param[in] Vertex shader program filename.
   * @param[in] Fragment shader program filename.
   */
  void init(const char * vs_filename, const char * fs_filename) {
    check_gl_error();
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
    check_gl_error();
  }


  /** Bind this shader program for rendering.
   *
   */
  void bind() {
    check_gl_error();
    glUseProgram(shader_id);
    check_gl_error();
  }

  /** Unbind this shader program so it isn't used for rendering.
   *
   */
  void unbind() {
    check_gl_error();
    glUseProgram(0);
    check_gl_error();
  }

  /** Get the OpenGL id for this shader program.
   *
   */
  GLuint id() {
    return shader_id;
  }

private:
  /** Attempt to make sure the shader compiles correctly.
   *
   * @param[in] OpenGL shader ID number.
   * @param[in] shader filename (optional, in case a shader is provided some other way).
   */
  void validate_shader(GLuint shader, const char * filename = NULL) {
    const unsigned int BUFFER_SIZE = 512;
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    GLsizei length = 0;

    glGetShaderInfoLog(shader, BUFFER_SIZE, &length, buffer);
    if (length > 0)
      std::cerr << "Shader " << shader << "(" << (filename?filename:"") << ") compile error: " << buffer << std::endl;
  }


  /** Attempt to make sure some GLSL program is valid.
   *
   * @param[in] GLSL program ID number used by OpenGL.
   */
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


  /** Load a shader program into a string.
   *
   * @param[in] shader program file path.
   * @param[out] source code of shader program, once loaded.
   *
   * \returns Whether opening the shader program and loading it was successful or not (not including validation).
   */
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
