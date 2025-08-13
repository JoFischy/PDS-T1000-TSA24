@echo off
echo Building PDS-T1000-TSA24 PERFORMANCE OPTIMIERT...

g++ -std=c++17 -O3 -DNDEBUG -Wall -Iexternal/raylib/src -Iinclude -Isrc/pybind11/include -I"C:/Program Files/Python311/include" src/main.cpp src/py_runner.cpp src/car_simulation.cpp src/auto.cpp src/point.cpp src/renderer.cpp src/coordinate_filter.cpp src/coordinate_filter_fast.cpp src/test_window.cpp src/path_system.cpp src/segment_manager.cpp src/vehicle_controller.cpp -Lexternal/raylib/src -lraylib -lopengl32 -lgdi32 -lwinmm -lcomctl32 -L"C:/Program Files/Python311/libs" -lpython311 -o main

if %ERRORLEVEL% EQU 0 (
    echo Build successful! MAXIMALE PERFORMANCE aktiviert
    echo main.exe ready - FAST MODE
) else (
    echo Build failed!
    exit /b 1
)
