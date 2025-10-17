# ZeroMQ Ping-Pong Latency Test

This implementation demonstrates a ZeroMQ-based ping-pong latency measurement system using REQ-REP sockets. ZeroMQ provides a high-level messaging abstraction with built-in reliability and queuing.

## How It Works

### Architecture

- **REQ-REP Pattern**: Request-Reply socket pattern for synchronous communication
- **ZeroMQ Context**: Manages sockets and threading
- **Message Queuing**: ZeroMQ handles message queuing and reliability
- **TCP Transport**: Uses TCP for reliable message delivery

### Key Components

- **Publisher (REQ)**: Sends requests and waits for replies
- **Subscriber (REP)**: Receives requests and sends replies
- **Message Format**: 8-byte timestamp messages
- **Automatic Queuing**: ZeroMQ handles message buffering

## Building

### Prerequisites

Install ZeroMQ and cppzmq:

```bash
# macOS
brew install zeromq cppzmq

# Ubuntu/Debian
sudo apt-get install libzmq3-dev libcppzmq-dev

# CentOS/RHEL
sudo yum install zeromq-devel cppzmq-devel
```

### Compilation

```bash
cd zmq
clang++ -std=c++17 -lzmq -o zmq_publisher zmq_publisher.cpp
clang++ -std=c++17 -lzmq -o zmq_subscriber zmq_subscriber.cpp
```

## Usage

### Terminal 1: Start Subscriber

```bash
./zmq_subscriber
```

Output:

```
ZeroMQ subscriber listening on tcp://*:5556
```

### Terminal 2: Start Publisher

```bash
./zmq_publisher 1000
```

Output:

```
ZeroMQ publisher sending 1000 messages...
ZeroMQ ping-pong count=1000 avg_RTT_us=45.2 median_RTT_us=42.1 p95_RTT_us=67.8 p99_RTT_us=89.3 avg_one_way_us=22.6
```

## Parameters

- **Subscriber**: No parameters (binds to port 5556)
- **Publisher**: `<count>` - Number of messages to send

## Expected Performance

On modern hardware with localhost communication:

- **RTT**: 20-100 microseconds (higher than raw UDP due to queuing)
- **One-way latency**: 10-50 microseconds
- **Throughput**: Limited by ZeroMQ's internal queuing and TCP overhead

## Technical Details

### Message Structure

```cpp
// 8-byte timestamp message
long timestamp;  // Nanoseconds since epoch
```

### ZeroMQ Socket Types

- **REQ (Publisher)**: Request socket, must send then receive
- **REP (Subscriber)**: Reply socket, must receive then send
- **Automatic Queuing**: ZeroMQ buffers messages internally

### Synchronization

- **Blocking I/O**: `sock.send()` and `sock.recv()` are blocking
- **Automatic Serialization**: ZeroMQ handles message framing
- **Error Handling**: Built-in reconnection and error recovery

## Performance Characteristics

### Advantages

- **High-Level API**: Easy to use, less error-prone
- **Built-in Reliability**: Automatic reconnection and error handling
- **Message Queuing**: Handles backpressure and buffering
- **Cross-Platform**: Works on all major operating systems

### Disadvantages

- **Higher Latency**: Due to internal queuing and TCP overhead
- **Memory Overhead**: Message queuing uses additional memory
- **Less Control**: Limited control over low-level behavior

### Latency Sources

1. **ZeroMQ Queuing**: Internal message queuing
2. **TCP Overhead**: TCP protocol stack processing
3. **Message Framing**: ZeroMQ message envelope overhead
4. **Context Switching**: Threading and I/O multiplexing

## Comparison with Other Implementations

- **vs UDP**: 2-5x higher latency (queuing + TCP vs raw UDP)
- **vs Shared Memory**: 10-50x higher latency (kernel networking vs direct memory)
- **vs Raw TCP**: 1.5-3x higher latency (ZeroMQ overhead)

## Advanced Usage

### Different Socket Types

```cpp
// For lower latency, try PAIR sockets
zmq::socket_t sock(ctx, zmq::socket_type::pair);

// For higher throughput, try PUSH-PULL
zmq::socket_t sock(ctx, zmq::socket_type::push);
```

### Transport Options

```cpp
// In-process communication (fastest)
sock.connect("inproc://test");

// Inter-process communication
sock.connect("ipc:///tmp/test.ipc");

// Network communication
sock.connect("tcp://127.0.0.1:5556");
```

## Troubleshooting

### Common Issues

1. **"Address already in use"**: Port 5556 already bound
2. **"Connection refused"**: Subscriber not running
3. **High latency**: System under load or network issues

### Performance Tips

- Use `inproc://` transport for local communication
- Use `PAIR` sockets for lowest latency
- Set socket options for performance tuning
- Use multiple contexts for parallel processing

## Production Considerations

### Socket Options

```cpp
// Set high water marks
sock.set(zmq::sockopt::rcvhwm, 1000);
sock.set(zmq::sockopt::sndhwm, 1000);

// Set linger time
sock.set(zmq::sockopt::linger, 0);
```

### Error Handling

```cpp
try {
    sock.send(msg, zmq::send_flags::none);
} catch (const zmq::error_t& e) {
    cerr << "Send error: " << e.what() << endl;
}
```

This ZeroMQ implementation provides a good balance between ease of use and performance, making it suitable for many production messaging scenarios.
