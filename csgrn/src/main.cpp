
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <vector>

#include "glm/fwd.hpp"
#include "material.hpp"
#include "shader.hpp"
#include "sphere.hpp"
#include "compute_shader.hpp"

const int LOCAL_SIZE_X = 8;
const int LOCAL_SIZE_Y = 8;

const int WIDTH = 800;
const int HEIGHT = 600;

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

int main() {

  // LOAD CSG MODEL
  // //////////////

  // INIT GLFW AND OpenGL Context

  if (!glfwInit())
    return -1;

  glfwWindowHintString(GLFW_WAYLAND_APP_ID, "csgrn");
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window =
      glfwCreateWindow(WIDTH, HEIGHT, "csgrn", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);

  // set screen callbvack
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "GLAD failed\n";
    return -1;
  }

  std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";

  // GET PARAMETERS TO CREATE WORKGROUPS LATER
  // ////

  ComputeShader rayTracer("src/shaders/raytracer.glsl");

  // define scene test objects
  std::vector<Sphere> spheres;

  Material mat;
  mat.albedo = glm::vec3(1.0f, 1.0f, 1.0f);
  mat.spec = 0.0f;

  spheres.push_back({{.0f, .0f, -1.0f}, .5f, mat});

  // create ssbo and populate it with scene
  unsigned int ssbo;
  glGenBuffers(1, &ssbo);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);

  glBufferData(GL_SHADER_STORAGE_BUFFER, spheres.size() * sizeof(Sphere),
               spheres.data(), GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

  // Create Texture for output
  unsigned int textureOut;
  glGenTextures(1, &textureOut);
  glActiveTexture(GL_TEXTURE);
  glBindTexture(GL_TEXTURE_2D, textureOut);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, WIDTH, HEIGHT);
  glBindImageTexture(0, textureOut, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

  // quad setup
  float quadVertices[] = {
      -1.0f, 1.0f, 0.0f, 0.0f,  1.0f, -1.0f, -1.0f, 0.0f,
      0.0f,  0.0f, 1.0f, -1.0f, 0.0f, 1.0f,  0.0f,

      -1.0f, 1.0f, 0.0f, 0.0f,  1.0f, 1.0f,  -1.0f, 0.0f,
      1.0f,  0.0f, 1.0f, 1.0f,  0.0f, 1.0f,  1.0f,
  };

  unsigned int quadVAO, quadVBO;
  glGenVertexArrays(1, &quadVAO);
  glGenBuffers(1, &quadVBO);
  glBindVertexArray(quadVAO);
  glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices,
               GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        (void *)(3 * sizeof(float)));

  Shader baseShader("src/shaders/base.vert", "src/shaders/base.frag");
  baseShader.use();
  baseShader.setInt("screenTexture", 0);

  while (!glfwWindowShouldClose(window)) {

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, true);

    // Compute shader do its things very efficiently
    rayTracer.use();
    glDispatchCompute(WIDTH / LOCAL_SIZE_X, HEIGHT / LOCAL_SIZE_Y, 1);
    glMemoryBarrier(
        GL_SHADER_IMAGE_ACCESS_BARRIER_BIT); // wait for compute shader to
                                             // finish writing on image

    glClear(GL_COLOR_BUFFER_BIT);

    baseShader.use();
    glBindVertexArray(quadVAO);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureOut);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glDeleteVertexArrays(1, &quadVAO);
  glDeleteBuffers(1, &quadVBO);
  glfwDestroyWindow(window);
  glfwTerminate();
}
