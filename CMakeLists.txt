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



# Camera Red Object Detection Program
add_executable(camera_detection
    src/main.cpp
    src/CameraDisplay.cpp
    src/py_runner.cpp
)
target_link_libraries(camera_detection PRIVATE raylib python313)
