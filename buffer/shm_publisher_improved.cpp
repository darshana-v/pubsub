// Improved SHM Publisher with std::atomic_ref and better synchronization
// Usage: ./shm_publisher_improved <shm_name> <count>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace std;
using ns = chrono::nanoseconds;
using clk = chrono::high_resolution_clock;

constexpr size_t MSG_SIZE = 64;
constexpr size_t RING_SIZE = 1024;
constexpr size_t MAX_BACKOFF = 1000; // Maximum backoff iterations

struct ShmHeader {
    std::atomic<uint64_t> head; // Producer index
    std::atomic<uint64_t> tail; // Consumer index
    std::atomic<bool> initialized; // Initialization flag
    // messages follow
};

struct ShmMsg {
    uint64_t seq;
    uint64_t t_ns;
    char payload[MSG_SIZE - 16];
};

class ExponentialBackoff {
private:
    int current_delay;
    int max_delay;
    
public:
    ExponentialBackoff(int initial = 1, int max = MAX_BACKOFF) 
        : current_delay(initial), max_delay(max) {}
    
    void wait() {
        for (int i = 0; i < current_delay; ++i) {
            std::this_thread::yield();
        }
        current_delay = min(current_delay * 2, max_delay);
    }
    
    void reset() {
        current_delay = 1;
    }
};

int main(int argc, char** argv) {
    if (argc < 3) { 
        cerr << "Usage: " << argv[0] << " <shm_name> <count>\n"; 
        return 1; 
    }
    
    string name = "/tmp/" + string(argv[1]);
    int count = stoi(argv[2]);

    size_t total_size = sizeof(ShmHeader) + RING_SIZE * sizeof(ShmMsg);

    // Use a regular file for shared memory on macOS
    int fd = open(name.c_str(), O_CREAT | O_RDWR, 0600);
    if (fd < 0) { perror("open"); return 1; }
    
    if (ftruncate(fd, total_size) < 0) { 
        perror("ftruncate"); 
        close(fd);
        return 1; 
    }
    
    void* base = mmap(nullptr, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (base == MAP_FAILED) { perror("mmap"); return 1; }

    auto hdr = reinterpret_cast<ShmHeader*>(base);
    auto msgs = reinterpret_cast<ShmMsg*>((char*)base + sizeof(ShmHeader));

    // Initialize header if first time
    bool expected = false;
    if (hdr->initialized.compare_exchange_strong(expected, true)) {
        hdr->head.store(0, memory_order_relaxed);
        hdr->tail.store(0, memory_order_relaxed);
    }

    vector<double> rtts;
    rtts.reserve(count);
    
    ExponentialBackoff backoff;

    for (uint64_t i = 0; i < (uint64_t)count; ++i) {
        // Wait for space with exponential backoff
        while (true) {
            uint64_t current_head = hdr->head.load(memory_order_acquire);
            uint64_t current_tail = hdr->tail.load(memory_order_acquire);
            
            if ((current_head - current_tail) < RING_SIZE) {
                break;
            }
            backoff.wait();
        }
        
        uint64_t idx = hdr->head.load(memory_order_acquire) % RING_SIZE;
        ShmMsg &m = msgs[idx];
        m.seq = i;
        m.t_ns = (uint64_t)chrono::duration_cast<ns>(clk::now().time_since_epoch()).count();
        
        // Publish with release semantics
        hdr->head.store(hdr->head.load(memory_order_acquire) + 1, memory_order_release);
        backoff.reset();

        // Wait for consumer to process with exponential backoff
        while (true) {
            uint64_t current_tail = hdr->tail.load(memory_order_acquire);
            if (current_tail > i) {
                break;
            }
            backoff.wait();
        }
        
        // Calculate RTT (simplified - just measure time since we sent)
        uint64_t now_ns = (uint64_t)chrono::duration_cast<ns>(clk::now().time_since_epoch()).count();
        double rtt_us = (now_ns - m.t_ns) / 1000.0;
        rtts.push_back(rtt_us);
    }

    // Calculate statistics
    double sum = 0;
    for (double v : rtts) sum += v;
    double avg = sum / rtts.size();
    
    // Sort for percentiles
    sort(rtts.begin(), rtts.end());
    double median = rtts[rtts.size() / 2];
    double p95 = rtts[static_cast<size_t>(rtts.size() * 0.95)];
    double p99 = rtts[static_cast<size_t>(rtts.size() * 0.99)];
    
    cout << "SHM pub count=" << rtts.size() 
         << " avg_RTT_us=" << avg 
         << " median_RTT_us=" << median
         << " p95_RTT_us=" << p95
         << " p99_RTT_us=" << p99
         << " avg_one_way_us=" << (avg/2.0) << "\n";

    munmap(base, total_size);
    close(fd);
    return 0;
}
