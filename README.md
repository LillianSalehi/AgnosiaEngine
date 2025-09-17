# Agnosia Engine

![GitHub license](https://img.shields.io/github/license/LillianSalehi/AgnosiaEngine.svg)

A very basic C++ vulkan renderer built from scratch 

---

![](https://raw.githubusercontent.com/LillianSalehi/AgnosiaEngine/refs/heads/master/resources/Agnosia.png)

## Features
- 3D Model, Material, and Texture handling.
- Cook-Torrance BRDF PBR Lighting.
- (WIP) Raytracing with the `nvvk` library

## Reporting Issues and Bugs

You can report bugs and crashes by opening an issue on our [issue tracker](https://github.com/LillianSalehi/AgnosiaEngine/issues).
Note that *Agnosia* is a constant work-in-progress unstable engine- problems do exist, and reports should really be limited to crashes and frame drops.

## Building with CMake

Agnosia uses CMake to build the engine, previously Makefile. To build it, run `mkdir build && cd build` and `cmake ..`. If that generates successfully, you can build the project with `make` and get the `agnosia` binary!
