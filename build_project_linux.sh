#!/bin/bash

echo "========================================"
echo "app - Build Script"
echo "========================================"
echo

if [ -z "$DISPLAY" ]; then
    echo "Warning: DISPLAY is not set. Setting to :0"
    export DISPLAY=:0
fi

# ================
#      Clear
# ================
echo "[1/3] Cleaning build directory..."
if [ -d "build" ]; then
    rm -rf build
    if [ $? -ne 0 ]; then
        echo "[ERROR] Failed to delete build directory"
        read -p "Press Enter to continue..."
        exit 1
    fi
    echo "[OK] Build directory cleaned"
else
    echo "[INFO] Build directory does not exist, skipping clean"
fi
echo

# ================
#      Configure
# ================
echo "[2/3] Configuring with CMake..."
cmake -B build -G "Unix Makefiles"
if [ $? -ne 0 ]; then
    echo
    echo "[ERROR] CMake configuration failed!"
    echo
    echo "Please check:"
    echo "  1. CMake is installed (sudo apt install cmake)"
    echo "  2. build-essential is installed (sudo apt install build-essential)"
    echo "  3. All required libraries are in 3rdparty/"
    echo
    read -p "Press Enter to continue..."
    exit 1
fi
echo "[OK] Configuration successful"
echo

# ================
#      Build
# ================
echo "[3/3] Building project..."
cmake --build build
if [ $? -ne 0 ]; then
    echo
    echo "[ERROR] Build failed!"
    echo
    echo "Please check the error messages above."
    echo
    read -p "Press Enter to continue..."
    exit 1
fi
echo "[OK] Build successful"
echo

# ================
#      Summary
# ================
echo "========================================"
echo "Build completed successfully!"
echo "========================================"
echo
echo "Executable: build/app"
echo

# ================
#      Execute
# ================
echo
echo "Running application..."
echo "----------------------------------------"
echo
./build/app

echo
echo "----------------------------------------"
echo "Application exited"
echo