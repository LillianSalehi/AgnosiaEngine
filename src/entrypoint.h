#pragma once

#include <GLFW/glfw3.h>
#include <mutex>

class EntryApp {
public:
  void initialize();
  bool isInitialized() const;
  void run();
  void setFramebufferResized(bool frame);
  bool getFramebufferResized() const;
  static GLFWwindow *getWindow();

  // Prevent singleton from being cloned and assigned.
  EntryApp(EntryApp &) = delete;
  void operator=(const EntryApp &) = delete;
  
  static EntryApp* getInstance();

protected:
  EntryApp();
  ~EntryApp();
  
private:
  static EntryApp* instance_;
  static std::mutex mutex_;

  bool framebufferResized;
  bool initialized;
};
