#pragma once
#include "../global.h"


namespace RenderPresent {
class render {
  public:
    void drawFrame();
    void createSyncObject();
    void destroyFenceSemaphores();
    void cleanupSwapChain();
  };
}
