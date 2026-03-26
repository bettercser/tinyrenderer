cmake_minimum_required(VERSION 3.12...3.26)

get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT is_multi_config AND NOT (CMAKE_BUILD_TYPE OR DEFINED ENV{CMAKE_BUILD_TYPE}))
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Release default")
endif()

project(tinyrenderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

option(iwyu "Run include-what-you-use")
if(iwyu)
  find_program(IWYU_EXE NAMES include-what-you-use REQUIRED)
  set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${IWYU_EXE})
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU|Intel")
  add_compile_options(-Wall)
endif()

find_package(OpenMP COMPONENTS CXX)

set(SOURCES main.cpp 
tgaimage.cpp 
model.hpp 
Vector.hh 
gl.cpp 
gl.hh 
model.cpp 
render/shader.hpp 
render/shaders/random_shader.hpp 
render/shaders/phong_shader.hpp 
render/shaders/smooth_shader.hpp 
render/shaders/texture_shader.hpp 
render/shaders/depth_shader.hpp 
render/renderer.cpp 
render/renderer.hpp
render/rasterizer.cpp
render/rasterizer.hpp
render/utils/render_util.hpp
render/frame.hpp
render/material.hpp
scene/scene.hpp
render/hit_record.hpp
scene/camera.hpp
render/ray.hpp
scene/camera.cpp
scene/sphere.hpp
scene/sphere.cpp
render/tracer.hpp
render/tracer.cpp
scene/scene.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE $<$<BOOL:${OpenMP_CXX_FOUND}>:OpenMP::OpenMP_CXX>)

file(GENERATE OUTPUT .gitignore CONTENT "*")
