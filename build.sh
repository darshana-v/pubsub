#!/bin/bash
# Build script for pub/sub latency test

set -e

echo "Pub/Sub Latency Test Build Script"
echo "=================================="

# Check dependencies
echo "Checking dependencies..."

# Check for CMake
if ! command -v cmake &> /dev/null; then
    echo "Error: CMake not found. Please install CMake."
    exit 1
fi

# Check for ZeroMQ
if ! pkg-config --exists libzmq; then
    echo "Warning: ZeroMQ not found. ZeroMQ tests will not be built."
    echo "Install with: brew install zeromq cppzmq (macOS) or apt-get install libzmq3-dev libcppzmq-dev (Ubuntu)"
fi

# Create build directory
echo "Creating build directory..."
mkdir -p build
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake ..

# Build
echo "Building..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo ""
echo "Build completed successfully!"
echo ""
echo "Available executables:"
echo "  UDP:           udp_publisher, udp_subscriber"
echo "  Shared Memory: shm_publisher, shm_subscriber"
echo "  Improved SHM:  shm_publisher_improved, shm_subscriber_improved"
echo "  ZeroMQ:        zmq_publisher, zmq_subscriber"
echo "  Test Harness:  latency_test"
echo ""
echo "To run all tests:"
echo "  ./latency_test 10000 1000"
echo ""
echo "To run individual tests:"
echo "  make run_udp_test"
echo "  make run_shm_test"
echo "  make run_zmq_test"
echo "  make run_all_tests"
