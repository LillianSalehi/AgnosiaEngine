cmake_minimum_required(VERSION 3.22)
project(agnosia C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



#NB! Include directories are included so that you can reference them without a relative path in code, just the file.
include_directories(lib lib/imgui)

file(GLOB SHADERS src/shaders/*)
file(GLOB ASSETS assets/*)

file(GLOB LIBS lib/*.c lib/*.h lib/imgui/*.cpp lib/imgui/*.h)
file(GLOB SOURCES src/*.cpp src/*.h src/graphics/*.cpp src/graphics/*.h)

add_executable(agnosia ${LIBS} ${SOURCES})

add_definitions(-DIMGUI_IMPL_VULKAN_NO_PROTOTYPES)

# Link libraries (-llib )
target_link_libraries(agnosia
    glfw
    dl
    pthread
    X11
    Xxf86vm
    Xrandr
    Xi
    tinyobjloader
    glslang
    glslang-default-resource-limits
)
# These directories are referenced relatively in code, to find shaders and assets, so we move them to the destination location as well.
file(COPY ${SHADERS} DESTINATION src/shaders)
file(COPY ${ASSETS} DESTINATION assets)
