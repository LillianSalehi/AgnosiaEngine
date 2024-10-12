#include <cstdlib>
#include "devicelibrary.h" // Device Library includes global, redundant to include with it here
#include "debug/vulkandebuglibs.h"
#include "graphics/graphicspipeline.h"
#include "graphics/render.h"
#include "global.h"
class EntryApp {
  public: 
    static EntryApp& getInstance();
    void initialize();
    bool isInitialized() const;
    void run();
    void setFramebufferResized(bool frame);
    bool getFramebufferResized() const;
  private:
    EntryApp();
    
    EntryApp(const EntryApp&) = delete;
    void operator=(const EntryApp&) = delete;

    bool framebufferResized;
    bool initialized;
};
