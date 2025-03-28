cmake_minimum_required(VERSION 3.10)
project(WorldCreatorToGLTF)

if(MSVC)
  message(STATUS "UNICODE chars enabled for MSVC")
  add_definitions(-DUNICODE -D_UNICODE)
  add_compile_options(/sdl)
  # comment next line to get deprecation errors:
  add_compile_options(/wd4996)
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=deprecated")
elseif(APPLE)
  add_definitions(-DALLOW_USING_NON_TRANSFER_ONLY_QUEUE)
  add_compile_options(-Wall -Wextra -Werror -Wno-error=unused-variable -Wno-error=unused-function
  -Wno-error=implicit-function-declaration -Wno-return-type -Wno-unused-parameter -Wno-unknown-pragmas
  -Wno-defaulted-function-deleted -Wno-unused-private-field -Wno-sign-compare -Wno-deprecated-declarations
  -Wno-unused-but-set-variable -Wno-format-security -Wno-null-conversion -Wno-error=deprecated
  )
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Check if VCPKG_ROOT is defined
if(NOT DEFINED ENV{VCPKG_ROOT})
    message(FATAL_ERROR "VCPKG_ROOT environment variable is not defined. Please set it to the root of your vcpkg installation.")
endif()

# Set the vcpkg toolchain file
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

# Check if the vcpkg toolchain file exists
if(NOT EXISTS ${CMAKE_TOOLCHAIN_FILE})
    message(FATAL_ERROR "vcpkg toolchain file not found at ${CMAKE_TOOLCHAIN_FILE}. Please ensure vcpkg is installed correctly.")
endif()

# Include the vcpkg toolchain file
include(${CMAKE_TOOLCHAIN_FILE})

# Find packages using vcpkg
find_package(assimp CONFIG REQUIRED)
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
message(INFO "TINYGLTF_INCLUDE_DIRS ${TINYGLTF_INCLUDE_DIRS}")

# Add executable
add_executable(WorldCreatorToGLTF main.cpp)

# Link libraries
target_link_libraries(WorldCreatorToGLTF PRIVATE assimp::assimp)

# Include directories
target_include_directories(WorldCreatorToGLTF PRIVATE ${ASSIMP_INCLUDE_DIRS} ${TINYGLTF_INCLUDE_DIRS})
