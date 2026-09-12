#!/usr/bin/env bash
# build.sh - Build script for Render deployment
set -e

echo "=== Building Plagiarism Detection C11 DSA Engine ==="
gcc -O2 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -std=c11 -Iinclude -o plagiarism_detector src/*.c
chmod +x plagiarism_detector

echo "=== Installing Python Dependencies ==="
pip install --upgrade pip
pip install -r requirements.txt

echo "=== Build Completed Successfully ==="
