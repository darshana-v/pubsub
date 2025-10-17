// Usage: ./zmq_subscriber
// ZeroMQ REQ-REP ping-pong latency test - subscriber (REP socket)

#include <zmq.hpp>
#include <chrono>
#include <iostream>

using namespace std;
using ns = chrono::nanoseconds;
using clk = chrono::high_resolution_clock;

int main(int argc, char** argv) {
    // Create ZeroMQ context and socket
    zmq::context_t ctx{1};
    zmq::socket_t sock(ctx, zmq::socket_type::rep);
    
    // Bind to port 5556
    sock.bind("tcp://*:5556");
    
    cout << "ZeroMQ subscriber listening on tcp://*:5556\n";
    
    while (true) {
        // Receive message
        zmq::message_t msg;
        sock.recv(msg);
        
        // Extract timestamp from message
        long send_ts;
        memcpy(&send_ts, msg.data(), sizeof(send_ts));
        
        // Update timestamp to current time (echo)
        long echo_ts = chrono::duration_cast<ns>(clk::now().time_since_epoch()).count();
        
        // Send echo back
        zmq::message_t reply(sizeof(long));
        memcpy(reply.data(), &echo_ts, sizeof(echo_ts));
        sock.send(reply, zmq::send_flags::none);
    }
    
    return 0;
}
