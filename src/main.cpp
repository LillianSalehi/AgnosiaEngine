#include "entrypoint.h"
int main() {
  EntryApp::getInstance().initialize();

  try {
    EntryApp::getInstance().run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

