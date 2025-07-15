cmake_minimum_required(VERSION 3.11)
project(TestPyCpp)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include/pybind11)

# Python-Header und Libs aus der lokalen Installation einbinden
# TODO: Make this portable for team collaboration
include_directories("C:/Users/jonas/AppData/Local/Programs/Python/Python313/include")
link_directories("C:/Users/jonas/AppData/Local/Programs/Python/Python313/libs")


# Raylib: Use vcpkg or system installation, fallback to FetchContent
find_package(raylib QUIET)
if (NOT raylib_FOUND)
  include(FetchContent)
  
  # Disable raylib examples and other extras to speed up build
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
  
  FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.0
  )
  FetchContent_MakeAvailable(raylib)
endif()

# Raylib-Header für den Compiler verfügbar machen
include_directories(${raylib_SOURCE_DIR}/src)



# Multi-Vehicle Fleet Detection Program
add_executable(camera_detection
    src/main.cpp
    src/MultiCarDisplay.cpp
    src/VehicleFleet.cpp
    src/py_runner.cpp
)
target_link_libraries(camera_detection PRIVATE raylib python313)
