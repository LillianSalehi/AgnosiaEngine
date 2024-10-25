#include "graphics/graphicspipeline.h"
#include "graphics/render.h"
#include "global.h"
#include "graphics/model.h"
#include "graphics/texture.h"

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
