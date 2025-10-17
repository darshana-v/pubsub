# UDP Ping-Pong Latency Test

This implementation demonstrates a simple UDP-based ping-pong latency measurement system. The publisher sends timestamped messages to a subscriber, which immediately echoes them back, allowing the publisher to measure round-trip time (RTT).

## How It Works

### Architecture

- **Publisher**: Sends UDP packets with sequence numbers and timestamps to the subscriber
- **Subscriber**: Receives packets and immediately echoes them back with updated timestamps
- **Measurement**: Publisher measures RTT by comparing send time with echo receive time

### Key Components

- **UDP Socket**: Uses `SOCK_DGRAM` for connectionless communication
- **Timestamping**: Uses `std::chrono::high_resolution_clock` for nanosecond precision
- **Ephemeral Port**: Publisher binds to port 0 (system-assigned) to receive replies
- **Atomic Operations**: Uses memory barriers for proper synchronization

## Building

```bash
cd udp
clang++ -std=c++17 -o publisher publisher.cpp
clang++ -std=c++17 -o subscriber subscriber.cpp
```

## Usage

### Terminal 1: Start Subscriber

```bash
./subscriber 5555
```

Output:

```
subscriber_udp listening on port 5555
```

### Terminal 2: Start Publisher

```bash
./publisher 127.0.0.1 5555 1000
```

Output:

```
UDP ping-pong count=1000 avg_RTT_us=15.2 avg_one_way_us=7.6
```

## Parameters

- **Subscriber**: `<port>` - Port to listen on
- **Publisher**: `<subscriber_ip> <subscriber_port> <count>` - Target IP, port, and number of messages

## Expected Performance

On modern hardware with localhost communication:

- **RTT**: 5-50 microseconds (depending on system load)
- **One-way latency**: 2.5-25 microseconds
- **Throughput**: Limited by kernel networking stack

## Technical Details

### Message Structure

```cpp
struct Msg {
    uint64_t seq;    // Sequence number
    uint64_t t_ns;   // Timestamp in nanoseconds
};
```

### Synchronization

- Uses `__atomic_thread_fence(__ATOMIC_RELEASE)` for memory ordering
- Publisher waits for replies using `recvfrom()`
- Subscriber immediately echoes using `sendto()`

### Latency Sources

1. **Kernel networking stack**: Packet processing overhead
2. **System call overhead**: `sendto()`/`recvfrom()` system calls
3. **Memory copies**: Kernel-to-userspace data movement
4. **Scheduling delays**: Process scheduling and context switches

## Troubleshooting

### Common Issues

1. **"Address already in use"**: Port is already bound, try different port
2. **"Connection refused"**: Subscriber not running or wrong IP/port
3. **High latency**: System under load, try with `nice -20` for higher priority

### Performance Tips

- Run with CPU affinity: `taskset -c 0 ./publisher 127.0.0.1 5555 1000`
- Disable frequency scaling: `echo performance > /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor`
- Use dedicated CPU cores for minimal jitter

## Comparison with Other Implementations

- **vs Shared Memory**: Higher latency due to kernel networking stack
- **vs ZeroMQ**: Lower latency but more complex to implement
- **vs TCP**: Lower latency but no reliability guarantees

This UDP implementation provides a good baseline for network-based latency measurements and is useful for comparing against more sophisticated messaging systems.
