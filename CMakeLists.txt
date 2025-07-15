cmake_minimum_required(VERSION 3.11)
project(TestPyCpp)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include/pybind11)

# Python-Header und Libs aus der lokalen Installation einbinden
# TODO: Make this portable for team collaboration
include_directories("C:/Users/henri/AppData/Local/Programs/Python/Python313/include")
link_directories("C:/Users/henri/AppData/Local/Programs/Python/Python313/libs")


# Raylib setup
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/raylib/src")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/external")
    execute_process(
        COMMAND git clone --depth 1 --branch 5.0 https://github.com/raysan5/raylib.git raylib
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/external"
    )
endif()

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/raylib" raylib_build)

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
