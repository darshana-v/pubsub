#!/bin/bash
# Simple test script for UDP and SHM implementations

echo "Pub/Sub Latency Test Results"
echo "============================"
echo ""

# Test UDP
echo "Testing UDP implementation..."
./udp_subscriber 5555 &
UDP_SUB_PID=$!
sleep 1
echo "UDP Results:"
./udp_publisher 127.0.0.1 5555 1000
kill $UDP_SUB_PID 2>/dev/null
wait $UDP_SUB_PID 2>/dev/null
echo ""

# Test SHM
echo "Testing SHM implementation..."
./shm_subscriber test_shm &
SHM_SUB_PID=$!
sleep 1
echo "SHM Results:"
./shm_publisher test_shm 1000
kill $SHM_SUB_PID 2>/dev/null
wait $SHM_SUB_PID 2>/dev/null
echo ""

# Test Improved SHM
echo "Testing Improved SHM implementation..."
./shm_subscriber_improved test_shm_improved &
SHM_IMPROVED_SUB_PID=$!
sleep 1
echo "Improved SHM Results:"
./shm_publisher_improved test_shm_improved 1000
kill $SHM_IMPROVED_SUB_PID 2>/dev/null
wait $SHM_IMPROVED_SUB_PID 2>/dev/null
echo ""

echo "Test completed!"
echo ""
echo "Expected performance ranking (fastest to slowest):"
echo "1. Improved SHM - sub-microsecond to low microsecond"
echo "2. SHM - sub-microsecond to low microsecond" 
echo "3. UDP - few to tens of microseconds"
