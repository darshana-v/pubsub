# Shared Memory (SHM) Ping-Pong Latency Test

This implementation demonstrates a high-performance shared memory-based ping-pong latency measurement system using a lock-free ring buffer. This approach achieves the lowest possible latency by avoiding kernel networking overhead.

## How It Works

### Architecture

- **Shared Memory**: Uses memory-mapped files (`mmap`) for inter-process communication
- **Ring Buffer**: Lock-free single-producer, single-consumer (SPSC) ring buffer
- **Atomic Operations**: Uses memory fences for proper synchronization
- **File-based SHM**: Uses `/tmp/` files instead of POSIX shared memory for macOS compatibility

### Key Components

- **Ring Buffer**: Fixed-size circular buffer with head/tail pointers
- **Message Structure**: 64-byte messages with sequence numbers and timestamps
- **Memory Mapping**: `mmap()` for shared memory between processes
- **Atomic Fences**: `__atomic_thread_fence()` for memory ordering

## Building

```bash
cd buffer
clang++ -std=c++17 -o shm_publisher shm_publisher.cpp
clang++ -std=c++17 -o shm_subscriber shm_subscriber.cpp
```

## Usage

### Terminal 1: Start Subscriber

```bash
./shm_subscriber test_shm
```

Output:

```
shm_sub started on /tmp/test_shm
```

### Terminal 2: Start Publisher

```bash
./shm_publisher test_shm 1000
```

Output:

```
SHM pub count=1000 avg_RTT_us=2.1 avg_one_way_us=1.05
```

## Parameters

- **Subscriber**: `<shm_name>` - Name for shared memory object
- **Publisher**: `<shm_name> <count>` - Shared memory name and number of messages

## Expected Performance

On modern hardware:

- **RTT**: 0.5-5 microseconds (sub-microsecond to low microsecond range)
- **One-way latency**: 0.25-2.5 microseconds
- **Throughput**: Limited only by memory bandwidth and CPU speed

## Technical Details

### Ring Buffer Structure

```cpp
struct ShmHeader {
    uint64_t head;  // Producer index (atomic)
    uint64_t tail;  // Consumer index (atomic)
};

struct ShmMsg {
    uint64_t seq;                    // Sequence number
    uint64_t t_ns;                   // Timestamp in nanoseconds
    char payload[MSG_SIZE - 16];     // Additional data
};
```

### Synchronization Protocol

1. **Producer (Publisher)**:

   - Waits for space: `while ((head - tail) >= RING_SIZE)`
   - Writes message to `msgs[head % RING_SIZE]`
   - Publishes with `__atomic_thread_fence(__ATOMIC_RELEASE)`
   - Increments `head`

2. **Consumer (Subscriber)**:
   - Waits for messages: `while (tail < head)`
   - Processes message at `msgs[tail % RING_SIZE]`
   - Consumes with `__atomic_thread_fence(__ATOMIC_ACQUIRE)`
   - Increments `tail`

### Memory Layout

```
+------------------+
| ShmHeader (16B)  |  <- head, tail pointers
+------------------+
| ShmMsg[0] (64B)  |  <- Ring buffer starts
+------------------+
| ShmMsg[1] (64B)  |
+------------------+
| ...              |
+------------------+
| ShmMsg[1023]     |  <- Ring buffer ends
+------------------+
```

## Performance Characteristics

### Advantages

- **Lowest Latency**: Direct memory access, no kernel involvement
- **High Throughput**: Limited only by memory bandwidth
- **CPU Efficient**: Minimal overhead, no system calls during operation
- **Deterministic**: Predictable performance characteristics

### Latency Sources

1. **Memory Access**: L1/L2/L3 cache hierarchy
2. **Atomic Operations**: Memory fence overhead
3. **Busy Waiting**: CPU cycles spent polling
4. **Scheduling**: Process scheduling delays

## macOS Compatibility Notes

### File-based Shared Memory

- Uses `/tmp/` files instead of POSIX shared memory
- Avoids `shm_open()`/`ftruncate()` issues on macOS
- Uses regular `open()` and `mmap()` for compatibility

### ARM64 Support

- Replaced x86 `pause` instruction with `std::this_thread::yield()`
- Uses portable C++ threading primitives
- Compatible with Apple Silicon Macs

## Troubleshooting

### Common Issues

1. **"No such file or directory"**: Publisher must start before subscriber
2. **High latency**: System under load, try CPU affinity
3. **Hanging**: Ring buffer full, check consumer is running

### Performance Optimization

```bash
# Pin to specific CPU cores
taskset -c 0 ./shm_subscriber test_shm &
taskset -c 1 ./shm_publisher test_shm 1000

# Set high priority
nice -20 ./shm_publisher test_shm 1000
```

## Comparison with Other Implementations

- **vs UDP**: 10-100x lower latency (no kernel networking)
- **vs ZeroMQ**: 5-50x lower latency (no message queuing)
- **vs TCP**: 50-500x lower latency (no reliability overhead)

## Production Considerations

### Current Limitations

- **Naive busy-wait**: Could use exponential backoff
- **Single producer/consumer**: Not suitable for multi-threaded scenarios
- **No persistence**: Data lost on process termination
- **No error handling**: Assumes reliable shared memory

### Improvements for Production

- Use `std::atomic_ref` for safer cross-process atomics
- Implement exponential backoff for busy-waiting
- Add robust initialization and cleanup
- Support multiple producers/consumers with proper locking
- Add error handling and recovery mechanisms

This shared memory implementation provides the absolute lowest latency for local inter-process communication and serves as an excellent baseline for comparing other messaging systems.
