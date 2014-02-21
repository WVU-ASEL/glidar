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