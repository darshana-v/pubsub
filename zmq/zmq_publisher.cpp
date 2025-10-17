// Usage: ./zmq_publisher <count>
// ZeroMQ REQ-REP ping-pong latency test

#include <zmq.hpp>
#include <chrono>
#include <iostream>
#include <vector>
#include <string>

using namespace std;
using ns = chrono::nanoseconds;
using clk = chrono::high_resolution_clock;

int main(int argc, char** argv) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <count>\n";
        return 1;
    }
    
    int count = stoi(argv[1]);
    
    // Create ZeroMQ context and socket
    zmq::context_t ctx{1};
    zmq::socket_t sock(ctx, zmq::socket_type::req);
    
    // Connect to subscriber
    sock.connect("tcp://127.0.0.1:5556");
    
    vector<double> rtts;
    rtts.reserve(count);
    
    cout << "ZeroMQ publisher sending " << count << " messages...\n";
    
    for (int i = 0; i < count; ++i) {
        // Record send time
        auto send_time = clk::now();
        long send_ts = chrono::duration_cast<ns>(send_time.time_since_epoch()).count();
        
        // Create and send message
        zmq::message_t msg(sizeof(long));
        memcpy(msg.data(), &send_ts, sizeof(send_ts));
        sock.send(msg, zmq::send_flags::none);
        
        // Receive reply
        zmq::message_t reply;
        sock.recv(reply);
        
        // Calculate RTT
        auto recv_time = clk::now();
        auto rtt_duration = recv_time - send_time;
        double rtt_us = chrono::duration_cast<chrono::microseconds>(rtt_duration).count();
        rtts.push_back(rtt_us);
    }
    
    // Calculate statistics
    double sum = 0;
    for (double rtt : rtts) sum += rtt;
    double avg = sum / rtts.size();
    
    // Sort for percentiles
    sort(rtts.begin(), rtts.end());
    double median = rtts[rtts.size() / 2];
    double p95 = rtts[static_cast<size_t>(rtts.size() * 0.95)];
    double p99 = rtts[static_cast<size_t>(rtts.size() * 0.99)];
    
    cout << "ZeroMQ ping-pong count=" << rtts.size() 
         << " avg_RTT_us=" << avg 
         << " median_RTT_us=" << median
         << " p95_RTT_us=" << p95
         << " p99_RTT_us=" << p99
         << " avg_one_way_us=" << (avg/2.0) << "\n";
    
    return 0;
}
