@echo off
title Plagiarism Detection and Document Similarity Analysis System
echo ============================================================
echo Starting Plagiarism Detection System UI...
echo Backend: C11 DSA Engine (plagiarism_detector.exe)
echo Bridge:  Python HTTP Server (server.py)
echo ============================================================
echo.

if not exist plagiarism_detector.exe (
    echo Compiling C engine...
    gcc -O2 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -std=c11 -Iinclude -o plagiarism_detector.exe src/*.c
    if %errorlevel% neq 0 (
        echo Compilation failed. Please ensure GCC is installed and in your PATH.
        pause
        exit /b 1
    )
)

echo Starting server on http://localhost:8080 ...
start "" http://localhost:8080
python server.py 8080
pause
