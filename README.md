# WorldCreatorToGLTF
convert WorldCreator export files into glTF 2.0 metallic-roughness glb

## Overview
This project is a C++ application that imports maps created by WorldCreator and exports them as glTF 2.0 using the metal-roughness workflow.

## Prerequisites
- CMake (version 3.10 or higher)
- A C++ compiler (e.g., GCC, Clang, MSVC)
- [vcpkg](https://github.com/microsoft/vcpkg) package manager

## Dependencies
This project uses the following libraries:
- Assimp
- tinygltf

Install with ``vcpkg install assimp tinygltf``

## Setup Instructions

```
   mkdir build
   cd build
   cmake ..
   cmake --build .
```

   