#pragma once

#include <GLFW/glfw3.h>

class EntryApp {
public:
  static EntryApp &getInstance();
  void initialize();
  bool isInitialized() const;
  void run();
  void setFramebufferResized(bool frame);
  bool getFramebufferResized() const;
  static GLFWwindow *getWindow();

private:
  EntryApp();

  EntryApp(const EntryApp &) = delete;
  void operator=(const EntryApp &) = delete;

  bool framebufferResized;
  bool initialized;
};
