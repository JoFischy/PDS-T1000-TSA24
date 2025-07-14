cmake_minimum_required(VERSION 3.11)
project(TestPyCpp)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include/pybind11)

# Find Python installation automatically
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

# Python-Header und Libs automatisch einbinden
include_directories(${Python3_INCLUDE_DIRS})
link_directories(${Python3_LIBRARY_DIRS})


# Raylib: Suche installiertes Paket, sonst lade als Tarball

set(RAYLIB_VERSION 5.0)
find_package(raylib ${RAYLIB_VERSION} QUIET)
if (NOT raylib_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG ${RAYLIB_VERSION}
  )
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED)
    set(FETCHCONTENT_QUIET NO)
    FetchContent_MakeAvailable(raylib)
  endif()
endif()

# Raylib-Header für den Compiler verfügbar machen
include_directories(${raylib_SOURCE_DIR}/src)



# Nur Raylib-Testprogramm bauen
add_executable(raylib_example
    src/main.cpp
    src/RaylibPythonAdder.cpp
    src/py_runner.cpp
)
target_link_libraries(raylib_example PRIVATE raylib ${Python3_LIBRARIES})
