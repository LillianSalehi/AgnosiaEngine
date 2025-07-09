#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class EntryApp {
public:
  void initialize();
  bool isInitialized() const;
  void run();
  void setFramebufferResized(bool frame);
  bool getFramebufferResized() const;
  static GLFWwindow *getWindow();
  
  static EntryApp& getInstance();
protected:
  EntryApp();
private:
  bool framebufferResized;
  bool initialized;
};
