// Unified test harness for all three latency implementations
// Usage: ./latency_test [count] [warmup]

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cstdlib>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;
using ns = chrono::nanoseconds;
using clk = chrono::high_resolution_clock;

struct LatencyStats {
    string name;
    vector<double> rtts;
    double avg;
    double median;
    double p95;
    double p99;
    double min;
    double max;
};

class LatencyTest {
private:
    int count;
    int warmup;
    vector<LatencyStats> results;
    
public:
    LatencyTest(int msg_count = 10000, int warmup_count = 1000) 
        : count(msg_count), warmup(warmup_count) {}
    
    void runUDPTest() {
        cout << "\n=== UDP Latency Test ===" << endl;
        
        // Start subscriber
        pid_t sub_pid = fork();
        if (sub_pid == 0) {
            execl("./udp_subscriber", "udp_subscriber", "5555", nullptr);
            exit(1);
        }
        
        // Wait for subscriber to start
        this_thread::sleep_for(chrono::milliseconds(100));
        
        // Run publisher
        pid_t pub_pid = fork();
        if (pub_pid == 0) {
            string count_str = to_string(count);
            execl("./udp_publisher", "udp_publisher", "127.0.0.1", "5555", count_str.c_str(), nullptr);
            exit(1);
        }
        
        // Wait for publisher to complete
        int status;
        waitpid(pub_pid, &status, 0);
        
        // Kill subscriber
        kill(sub_pid, SIGTERM);
        waitpid(sub_pid, &status, 0);
        
        cout << "UDP test completed" << endl;
    }
    
    void runSHMTest() {
        cout << "\n=== Shared Memory Latency Test ===" << endl;
        
        // Start subscriber
        pid_t sub_pid = fork();
        if (sub_pid == 0) {
            execl("./shm_subscriber", "shm_subscriber", "test_shm", nullptr);
            exit(1);
        }
        
        // Wait for subscriber to start
        this_thread::sleep_for(chrono::milliseconds(100));
        
        // Run publisher
        pid_t pub_pid = fork();
        if (pub_pid == 0) {
            string count_str = to_string(count);
            execl("./shm_publisher", "shm_publisher", "test_shm", count_str.c_str(), nullptr);
            exit(1);
        }
        
        // Wait for publisher to complete
        int status;
        waitpid(pub_pid, &status, 0);
        
        // Kill subscriber
        kill(sub_pid, SIGTERM);
        waitpid(sub_pid, &status, 0);
        
        cout << "SHM test completed" << endl;
    }
    
    void runZeroMQTest() {
        cout << "\n=== ZeroMQ Latency Test ===" << endl;
        
        // Start subscriber
        pid_t sub_pid = fork();
        if (sub_pid == 0) {
            execl("./zmq_subscriber", "zmq_subscriber", nullptr);
            exit(1);
        }
        
        // Wait for subscriber to start
        this_thread::sleep_for(chrono::milliseconds(100));
        
        // Run publisher
        pid_t pub_pid = fork();
        if (pub_pid == 0) {
            string count_str = to_string(count);
            execl("./zmq_publisher", "zmq_publisher", count_str.c_str(), nullptr);
            exit(1);
        }
        
        // Wait for publisher to complete
        int status;
        waitpid(pub_pid, &status, 0);
        
        // Kill subscriber
        kill(sub_pid, SIGTERM);
        waitpid(sub_pid, &status, 0);
        
        cout << "ZeroMQ test completed" << endl;
    }
    
    void runAllTests() {
        cout << "Starting latency comparison test..." << endl;
        cout << "Message count: " << count << endl;
        cout << "Warmup count: " << warmup << endl;
        
        // Run all tests
        runUDPTest();
        runSHMTest();
        runZeroMQTest();
        
        cout << "\n=== Test Summary ===" << endl;
        cout << "All tests completed. Check individual outputs above for detailed results." << endl;
        cout << "\nExpected performance ranking (fastest to slowest):" << endl;
        cout << "1. Shared Memory (SHM) - sub-microsecond to low microsecond" << endl;
        cout << "2. UDP - few to tens of microseconds" << endl;
        cout << "3. ZeroMQ - tens to hundreds of microseconds" << endl;
    }
    
    void generateCSV() {
        ofstream csv("latency_results.csv");
        csv << "Implementation,Message_Count,Avg_RTT_us,Median_RTT_us,P95_RTT_us,P99_RTT_us,Min_RTT_us,Max_RTT_us\n";
        
        for (const auto& result : results) {
            csv << result.name << "," << result.rtts.size() << ","
                << result.avg << "," << result.median << "," << result.p95 << ","
                << result.p99 << "," << result.min << "," << result.max << "\n";
        }
        
        csv.close();
        cout << "Results saved to latency_results.csv" << endl;
    }
};

void printUsage(const char* prog) {
    cout << "Usage: " << prog << " [count] [warmup]" << endl;
    cout << "  count:   Number of messages to send (default: 10000)" << endl;
    cout << "  warmup:  Number of warmup messages (default: 1000)" << endl;
    cout << endl;
    cout << "This test harness runs all three latency implementations:" << endl;
    cout << "1. UDP ping-pong" << endl;
    cout << "2. Shared Memory (SHM) ping-pong" << endl;
    cout << "3. ZeroMQ REQ-REP ping-pong" << endl;
    cout << endl;
    cout << "Make sure all executables are built and in the current directory." << endl;
}

int main(int argc, char* argv[]) {
    int count = 10000;
    int warmup = 1000;
    
    if (argc > 1) {
        count = stoi(argv[1]);
    }
    if (argc > 2) {
        warmup = stoi(argv[2]);
    }
    
    if (argc > 1 && (string(argv[1]) == "-h" || string(argv[1]) == "--help")) {
        printUsage(argv[0]);
        return 0;
    }
    
    cout << "Latency Test Harness" << endl;
    cout << "===================" << endl;
    
    LatencyTest test(count, warmup);
    test.runAllTests();
    
    return 0;
}
