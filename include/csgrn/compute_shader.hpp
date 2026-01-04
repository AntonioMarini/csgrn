#ifndef COMPUTE_SHADER_H
#define COMPUTE_SHADER_H

#include <fstream>
#include <glad/glad.h>
#include <iostream>
#include <sstream>
#include <string>

class compute_shader {
public:
  unsigned int id;

  compute_shader(const char *path) {

    std::string compute_code;
    std::ifstream c_shader_file;
    c_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
      c_shader_file.open(path);
      std::stringstream c_shader_stream;
      c_shader_stream << c_shader_file.rdbuf();

      c_shader_file.close();
      compute_code = c_shader_stream.str();
    } catch (std::ifstream::failure e) {
      std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
    }

    unsigned int compute = glCreateShader(GL_COMPUTE_SHADER);
    const char *c_shader_code = compute_code.c_str();

    glShaderSource(compute, 1, &c_shader_code, NULL);
    glCompileShader(compute);
    check_compile_errors(compute, "COMPUTE");

    // shader Program
    id = glCreateProgram();
    glAttachShader(id, compute);
    glLinkProgram(id);
    check_compile_errors(id, "PROGRAM");
  }

  void use() { glUseProgram(id); }

  // utility uniform functions
  void set_bool(const std::string &name, bool value) const {
    glUniform1i(glGetUniformLocation(id, name.c_str()), (int)value);
  }
  void set_int(const std::string &name, int value) const {
    glUniform1i(glGetUniformLocation(id, name.c_str()), value);
  }
  void set_float(const std::string &name, float value) const {
    glUniform1f(glGetUniformLocation(id, name.c_str()), value);
  }
  void set_vec3(const std::string &name, float x, float y, float z) const {
    glUniform3f(glGetUniformLocation(id, name.c_str()), x, y, z);
  }

  // utility for loading ssbos.

private:
  // utility function for checking shader compilation/linking errors.
  // ------------------------------------------------------------------------
  void check_compile_errors(unsigned int shader, std::string type) {
    int success;
    char info_log[1024];
    if (type != "PROGRAM") {
      glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
      if (!success) {
        glGetShaderInfoLog(shader, 1024, NULL, info_log);
        std::cout
            << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n"
            << info_log
            << "\n -- --------------------------------------------------- -- "
            << std::endl;
      }
    } else {
      glGetProgramiv(shader, GL_LINK_STATUS, &success);
      if (!success) {
        glGetProgramInfoLog(shader, 1024, NULL, info_log);
        std::cout
            << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n"
            << info_log
            << "\n -- --------------------------------------------------- -- "
            << std::endl;
      }
    }
  }
};

#endif // !COMPUTE_SHADER_H
