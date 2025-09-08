cmake_minimum_required(VERSION 3.20)
set(CMAKE_POLICY_VERSION_MINIMUM 3.10)

project(spinning_ball_lbm LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# ---- GLAD (ขอ API ที่มี compute: GL 4.6 core) ----
set(GLAD_PROFILE "core")
set(GLAD_API "gl=4.6")
set(GLAD_GENERATOR "c")
set(GLAD_EXTENSIONS "")

FetchContent_Declare(
  glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG        v0.1.36
)
FetchContent_MakeAvailable(glad)

# ---- GLFW ----
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.3.9
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

find_package(OpenGL REQUIRED)

add_executable(spinning_ball_lbm
  src/main.cpp
  src/gl_utils.cpp
  src/gl_utils.hpp
)

# ให้ทุก TU เห็น macro นี้ (ไม่ต้อง define ในโค้ด)
target_compile_definitions(spinning_ball_lbm PRIVATE GLFW_INCLUDE_NONE)

# include pat
target_include_directories(spinning_ball_lbm PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${glad_BINARY_DIR}/include
  ${glfw_SOURCE_DIR}/include
)

target_link_libraries(spinning_ball_lbm PRIVATE glad glfw OpenGL::GL)

# copy shaders
add_custom_command(TARGET spinning_ball_lbm POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/shaders
          $<TARGET_FILE_DIR:spinning_ball_lbm>/shaders)

FetchContent_MakeAvailable(glad)

# --------- GLFW (window/context) ----------
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.3.9
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

find_package(OpenGL REQUIRED)


target_include_directories(spinning_ball_lbm PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${glad_BINARY_DIR}/include  
  ${CMAKE_CURRENT_SOURCE_DIR}/external/stb
  ${glfw_SOURCE_DIR}/include
)
target_compile_definitions(spinning_ball_lbm PRIVATE GLFW_INCLUDE_NONE)
target_link_libraries(spinning_ball_lbm PRIVATE glad glfw OpenGL::GL)


add_custom_command(TARGET spinning_ball_lbm POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/shaders
          $<TARGET_FILE_DIR:spinning_ball_lbm>/shaders)
