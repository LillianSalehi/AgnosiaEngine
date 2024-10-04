#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>

// Define a base class structure to handle public and private methods
class TriangleTestApplication {
  const uint32_t WIDTH = 800;
  const uint32_t HEIGHT = 600;
  // Public facing function, Initializes our private functions and GLFW
public:
  void run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
  }

private:
  GLFWwindow *window;
  // Initialize GLFW Window. First, Initialize GLFW lib, disable resizing for
  // now, and create window.
  void initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // Settings for the window are set, create window reference.
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
  }
  void initVulkan() {}
  void mainLoop() {
    // Update window whilst open
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();
    }
  }
  void cleanup() {
    // Cleanup window when destroyed.
    glfwDestroyWindow(window);
    glfwTerminate();
  }
};

int main() {
  TriangleTestApplication app;

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
