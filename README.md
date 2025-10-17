# Pub/Sub Latency Comparison

This project implements three different approaches to inter-process communication (IPC) and measures their latency characteristics:

1. **UDP Ping-Pong** - Raw UDP sockets with kernel networking stack
2. **Shared Memory (SHM)** - Lock-free ring buffer with memory-mapped files
3. **ZeroMQ REQ-REP** - High-level messaging library with built-in queuing

## Quick Start

### Prerequisites

```bash
# macOS
brew install zeromq cppzmq cmake

# Ubuntu/Debian
sudo apt-get install libzmq3-dev libcppzmq-dev cmake build-essential

# CentOS/RHEL
sudo yum install zeromq-devel cppzmq-devel cmake gcc-c++
```

### Build All Implementations

```bash
# Using CMake (recommended)
mkdir build && cd build
cmake ..
make -j$(nproc)

# Or build individually
cd udp && make
cd ../buffer && make
cd ../zmq && make
```

### Run All Tests

```bash
# Run unified test harness
./latency_test 10000 1000

# Or run individual tests
./run_udp_test
./run_shm_test
./run_zmq_test
```

## Expected Performance

| Implementation    | Latency Range | Use Case                                |
| ----------------- | ------------- | --------------------------------------- |
| **Shared Memory** | 0.5-5 μs      | Ultra-low latency, local IPC            |
| **UDP**           | 5-50 μs       | Network communication, moderate latency |
| **ZeroMQ**        | 20-100 μs     | High-level messaging, reliability       |

## Architecture Overview

### Shared Memory (Fastest)

- **Transport**: Memory-mapped files (`/tmp/`)
- **Synchronization**: Lock-free ring buffer with atomic operations
- **Latency**: Sub-microsecond to low microsecond
- **Use Case**: Ultra-low latency local communication

### UDP (Balanced)

- **Transport**: Raw UDP sockets
- **Synchronization**: Kernel networking stack
- **Latency**: Few to tens of microseconds
- **Use Case**: Network communication, moderate latency

### ZeroMQ (Most Features)

- **Transport**: TCP with ZeroMQ abstraction
- **Synchronization**: Built-in queuing and reliability
- **Latency**: Tens to hundreds of microseconds
- **Use Case**: High-level messaging, reliability, cross-platform

## Detailed Documentation

- [UDP Implementation](udp/README.md) - Raw UDP ping-pong
- [Shared Memory Implementation](buffer/README.md) - Lock-free ring buffer
- [ZeroMQ Implementation](zmq/README.md) - High-level messaging

## Performance Analysis

### Latency Sources

**Shared Memory:**

- Memory access (L1/L2/L3 cache)
- Atomic operations
- Busy waiting overhead

**UDP:**

- Kernel networking stack
- System call overhead (`sendto`/`recvfrom`)
- Memory copies (kernel ↔ userspace)

**ZeroMQ:**

- Message queuing
- TCP protocol overhead
- ZeroMQ internal processing

### Optimization Tips

```bash
# CPU affinity for minimal jitter
taskset -c 0 ./shm_subscriber test_shm &
taskset -c 1 ./shm_publisher test_shm 10000

# High priority scheduling
nice -20 ./shm_publisher test_shm 10000

# Disable frequency scaling
echo performance > /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```

## Test Results Format

The test harness produces output in the following format:

```
Implementation: SHM
Message Count: 10000
Average RTT: 2.1 μs
Median RTT: 1.8 μs
P95 RTT: 4.2 μs
P99 RTT: 6.7 μs
One-way Latency: 1.05 μs
```

## Production Considerations

### Shared Memory

- ✅ Lowest latency
- ✅ Highest throughput
- ❌ Single producer/consumer only
- ❌ No persistence
- ❌ Platform-specific implementation

### UDP

- ✅ Low latency
- ✅ Network capable
- ✅ Simple implementation
- ❌ No reliability guarantees
- ❌ Kernel networking overhead

### ZeroMQ

- ✅ High-level API
- ✅ Built-in reliability
- ✅ Cross-platform
- ❌ Higher latency
- ❌ Additional dependencies

## Advanced Usage

### Custom Message Sizes

```cpp
// Modify MSG_SIZE in buffer implementations
constexpr size_t MSG_SIZE = 128; // 128-byte messages
```

### Different ZeroMQ Patterns

```cpp
// Use PAIR sockets for lower latency
zmq::socket_t sock(ctx, zmq::socket_type::pair);

// Use inproc transport for local communication
sock.connect("inproc://test");
```

### CPU Pinning Script

```bash
#!/bin/bash
# pin_cpu.sh - Run with CPU affinity

taskset -c 0 ./shm_subscriber test_shm &
SUB_PID=$!
sleep 1
taskset -c 1 ./shm_publisher test_shm 10000
kill $SUB_PID
```

## Troubleshooting

### Common Issues

1. **"Address already in use"** - Port conflict, try different port
2. **"No such file or directory"** - Publisher must start before subscriber
3. **High latency** - System under load, try CPU affinity
4. **ZeroMQ not found** - Install libzmq and cppzmq

### Performance Debugging

```bash
# Monitor system load
htop

# Check CPU frequency
cat /proc/cpuinfo | grep MHz

# Monitor network traffic
tcpdump -i lo

# Profile with perf
perf record ./shm_publisher test_shm 10000
perf report
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Submit a pull request

## License

MIT License - see LICENSE file for details.
