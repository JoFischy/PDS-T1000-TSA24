@echo off
echo Building PDS-T1000-TSA24 simplified version...

g++ -std=c++17 -Wall -Wextra -Iexternal/raylib/src -Iinclude -I"C:/Program Files/Python311/include" src/main.cpp src/py_runner.cpp -Lexternal/raylib/src -lraylib -lopengl32 -lgdi32 -lwinmm -L"C:/Program Files/Python311/libs" -lpython311 -o main

if %ERRORLEVEL% EQU 0 (
    echo Build successful! 
    echo.
    echo You can now run:
    echo   python src/Farberkennung.py    [Test color detection]
    echo   .\main.exe                     [Run C++ program]
) else (
    echo Build failed!
)

pause
