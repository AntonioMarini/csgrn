#include "csgrn.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "csgrn/compute_shader.hpp"
#include "csgrn/csg_parser.hpp"
#include "csgrn/csg_tree.hpp"
#include "csgrn/op_instruction.hpp"
#include "csgrn/primitive.hpp"
#include <iostream>
#include <iomanip> // For std::setw

const int LOCAL_SIZE_X = 8;
const int LOCAL_SIZE_Y = 8;

const int WIDTH = 800;
const int HEIGHT = 600;

const int M_SPANS = 0;
const int M_RAYMARCH = 1;

struct CSGContext {
  int width;
  int height;
  GLFWwindow *window;
  int rendering_mode = M_SPANS;
  glm::vec3 light_position = glm::vec3(1);
  float dt = 0.0;
  float fps = 0.0;
};
CSGContext ctx;

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  ctx.width = width;
  ctx.height = height;
  glViewport(0, 0, width, height);
}

// --- Camera Setup ---
#include "csgrn/camera.hpp"
camera camera(glm::vec3(0.0f, 0.0f, 5.0f));
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
bool firstMouse = true;

// --- Timing ---
float deltaTime = 0.0f;
float lastFrame = 0.0f;

void update_fps() {
  float dt = ctx.dt;

  static float accumulator = 0.0f;
  static int frame_count = 0;

  accumulator += dt;
  frame_count++;

  // Update FPS only if 0.5 seconds have passed
  if (accumulator >= 0.5f) {
    ctx.fps = static_cast<float>(frame_count) / accumulator;

    // Reset for the next batch
    accumulator = 0.0f;
    frame_count = 0;
  }
}

// --- GLFW Callbacks & Input ---
void mouse_callback(GLFWwindow *window, double xposIn, double yposIn) {
  float xpos = static_cast<float>(xposIn);
  float ypos = static_cast<float>(yposIn);

  if (firstMouse) {
    lastX = xpos;
    lastY = ypos;
    firstMouse = false;
  }

  float xoffset = xpos - lastX;
  float yoffset =
      lastY - ypos; // reversed since y-coordinates go from bottom to top

  lastX = xpos;
  lastY = ypos;

  camera.process_input_mouse(xoffset, yoffset);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  camera.process_input_scroll(static_cast<float>(yoffset));
}

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods) {
  if (key == GLFW_KEY_PERIOD && action == GLFW_PRESS) {
    ctx.rendering_mode = !ctx.rendering_mode;
  }
}

void process_input(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    camera.process_input_keys(FORWARD, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    camera.process_input_keys(BACKWARD, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    camera.process_input_keys(LEFT, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    camera.process_input_keys(RIGHT, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
    camera.process_input_keys(ROT_UP, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
    camera.process_input_keys(ROT_DOWN, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
    camera.process_input_keys(ROT_LEFT, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
    camera.process_input_keys(ROT_RIGHT, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    camera.process_input_keys(UP, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
    camera.process_input_keys(DOWN, deltaTime);
}

// Deletes the entire CSG tree to prevent memory leaks.
void delete_tree(csg_node *node) {
  if (!node)
    return;
  delete_tree(node->left);
  delete_tree(node->right);
  delete node;
}

std::string readFile(const std::string &filepath) {
  std::ifstream file(filepath);
  if (!file.is_open()) {
    std::cerr << "Error: Could not open file at " << filepath << std::endl;
    return "";
  }

  std::stringstream buffer;
  buffer << file.rdbuf(); // Read the whole file into the buffer
  return buffer.str();
}

void printSSBODebug(const std::vector<instruction> &id_ops,
                    const std::vector<primitive> &primitives,
                    const std::vector<operation> &operations) {

  std::cout
      << "\n================ SSBO ID_OPS DUMP (RPN ORDER) ================\n";
  std::cout << "| idx | Inst. Type | ID Ref | Detail                       |\n";
  std::cout << "|-----|------------|--------|------------------------------|\n";

  for (size_t i = 0; i < id_ops.size(); ++i) {
    const auto &inst = id_ops[i];

    std::cout << "| " << std::setw(3) << i << " | ";

    if (inst.type == (glm::uint)node_type::PRIMITIVE) { // ID_OP_TYPE_PRIMITIVE
      // Safety check
      std::string detail = "INVALID ID";
      if (inst.id < primitives.size()) {
        const auto &prim = primitives[inst.id];
        std::string type_name;
        switch (prim.type) {
        case primitive_types::sphere:
          type_name = "Sphere";
          break;
        case primitive_types::cube:
          type_name = "Cube";
          break;
        case primitive_types::cylinder:
          type_name = "Cylinder";
          break;
        default:
          type_name = "Unknown";
          break;
        }
        detail = "Type: " + type_name;
      }

      std::cout << " PRIMITIVE  | " << std::setw(6) << inst.id << " | "
                << std::left << std::setw(28) << detail << std::right << " |";
    } else if (inst.type ==
               (glm::uint)node_type::OPERATION) { // ID_OP_TYPE_OPERATION
      // Safety check
      std::string detail = "INVALID ID";
      if (inst.id < operations.size()) {
        detail = "OP:   " +
                 operation::get_op_name((op_types)operations[inst.id].type);
      }

      std::cout << " OPERATION  | " << std::setw(6) << inst.id << " | "
                << "\033[1;33m" // Yellow color for Ops to stand out
                << std::left << std::setw(28) << detail << "\033[0m"
                << std::right << " |";
    } else {
      std::cout << " UNKNOWN    | " << std::setw(6) << inst.id << " | "
                << std::setw(28) << "???" << " |";
    }
    std::cout << "\n";
  }
  std::cout
      << "==============================================================\n\n";
}

void update_ImGui() {
  // IMGUI //
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  ImVec4 yellow = ImVec4(1, 1, 0, 1);

  // UI DEFINITION
  ImGui::Begin("Debug Info"); // Create a window called "My Tools"
  ImGui::DragFloat("Light X", &ctx.light_position.x, 0.02f, 0.0f, 1.0f);
  ImGui::DragFloat("Light Y", &ctx.light_position.y, 0.02f, 0.0f, 1.0f);
  ImGui::DragFloat("Light Z", &ctx.light_position.z, 0.02f, 0.0f, 1.0f);
  ImGui::TextColored(yellow, "FPS: %.1f", ctx.fps);
  ImGui::Separator();
  ImGui::TextColored(yellow, "Rendering Mode: %s",
                     (ctx.rendering_mode == 0) ? "SPANS" : "RAY MARCHING");
  ImGui::End();

  // -- Rendering --
  ImGui::Render(); // Generate render data
}

int main() {

  std::string filepath = "models/wikipedia.csg";

  // 2. Read the file content
  std::string csgFileContent = readFile(filepath);

  if (csgFileContent.empty()) {
    return -1; // Exit if file read failed
  }

  // 3. Parse
  csg_parser parser(csgFileContent);
  csg_node *root_node = parser.parse();

  if (root_node == nullptr) {
    std::cout << "Exiting: CSG parsing failed." << std::endl;
    return -1;
  }

  csg_tree tree(*root_node); // CSGTree constructor takes root by value, works
                             // for this test.

  std::vector<primitive> primitives;
  std::vector<operation> operations;
  std::vector<instruction> instructions;

  tree.flatten_tree(root_node, primitives, operations, instructions);
  printSSBODebug(instructions, primitives, operations);

  delete_tree(root_node);

  // INIT GLFW AND OpenGL Context
  if (!glfwInit())
    return -1;

  glfwWindowHintString(GLFW_WAYLAND_APP_ID, "csgrn");
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *win = glfwCreateWindow(WIDTH, HEIGHT, "csgrn", nullptr, nullptr);
  if (!win) {
    glfwTerminate();
    return -1;
  }

  ctx.width = WIDTH;
  ctx.height = HEIGHT;
  ctx.window = win;

  glfwMakeContextCurrent(ctx.window);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard;           // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking (Optional)

  ImGui::StyleColorsDark(); // Setup Dear ImGui style

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(ctx.window, true);
  ImGui_ImplOpenGL3_Init("#version 460"); // Use your GL version

  // SET CALLBACKS
  glfwSetKeyCallback(ctx.window, key_callback);
  // glfwSetInputMode(ctx.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  // glfwSetCursorPosCallback(ctx.window, mouse_callback);
  // glfwSetScrollCallback(ctx.window, scroll_callback);
  glfwSetFramebufferSizeCallback(ctx.window, framebuffer_size_callback);

  // LOAD OpenGL functions
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "GLAD failed\n";
    return -1;
  }

  std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";

  compute_shader ray_tracer("src/shaders/raytracer.glsl");

  // Create SSBOs for the flattened CSG tree data
  unsigned int ssbo_primitives, ssbo_operations, ssbo_id_ops;

  // Primitives SSBO
  glGenBuffers(1, &ssbo_primitives);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_primitives);
  glBufferData(GL_SHADER_STORAGE_BUFFER, primitives.size() * sizeof(primitive),
               primitives.data(), GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_primitives);

  // Operations SSBO
  glGenBuffers(1, &ssbo_operations);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_operations);
  glBufferData(GL_SHADER_STORAGE_BUFFER, operations.size() * sizeof(operation),
               operations.data(), GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_operations);

  //(Instruction List) SSBO
  glGenBuffers(1, &ssbo_id_ops);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_id_ops);
  glBufferData(GL_SHADER_STORAGE_BUFFER,
               instructions.size() * sizeof(instruction), instructions.data(),
               GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_id_ops);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // Unbind

  // Create Texture for output
  unsigned int textureOut;
  glGenTextures(1, &textureOut);
  glActiveTexture(GL_TEXTURE);
  glBindTexture(GL_TEXTURE_2D, textureOut);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, ctx.width, ctx.height);
  glBindImageTexture(0, textureOut, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

  // QUAD VERTEX DATA
  float quadVertices[] = {
      -1.0f, 1.0f,  0.0f, 0.0f, 1.0f, //
      -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, //
      1.0f,  -1.0f, 0.0f, 1.0f, 0.0f, //
      -1.0f, 1.0f,  0.0f, 0.0f, 1.0f, //
      1.0f,  -1.0f, 0.0f, 1.0f, 0.0f, //
      1.0f,  1.0f,  0.0f, 1.0f, 1.0f, //
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

  while (!glfwWindowShouldClose(ctx.window)) {
    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    ctx.dt = deltaTime;
    update_fps();

    process_input(ctx.window);

    update_ImGui();

    ray_tracer.use();

    // Set rendering method uniform
    ray_tracer.set_int("u_rendering_mode", ctx.rendering_mode);

    // Set light uniforms
    ray_tracer.set_vec3("u_light_dir", ctx.light_position.x,
                        ctx.light_position.y, ctx.light_position.z);

    // Set camera uniforms
    glUniform3fv(glGetUniformLocation(ray_tracer.id, "u_camera_pos"), 1,
                 &camera.position[0]);
    glm::mat4 invView = glm::inverse(camera.get_view_mat());
    glUniformMatrix4fv(glGetUniformLocation(ray_tracer.id, "u_inv_view"), 1,
                       GL_FALSE, &invView[0][0]);

    glDispatchCompute(ctx.width / LOCAL_SIZE_X, ctx.height / LOCAL_SIZE_Y, 1);
    glMemoryBarrier(
        GL_SHADER_IMAGE_ACCESS_BARRIER_BIT); // wait for compute shader to
                                             // finish writing on the textureOut
    glClear(GL_COLOR_BUFFER_BIT);

    baseShader.use();
    glBindVertexArray(quadVAO);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureOut);

    glDrawArrays(GL_TRIANGLES, 0, 6); // draw the quad with textureOut applied

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(ctx.window);
    glfwPollEvents();
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glDeleteVertexArrays(1, &quadVAO);
  glDeleteBuffers(1, &quadVBO);
  glfwDestroyWindow(ctx.window);
  glfwTerminate();
}
