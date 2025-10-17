// Improved SHM Subscriber with std::atomic_ref and better synchronization
// Usage: ./shm_subscriber_improved <shm_name>

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
    if (argc < 2) { 
        cerr << "Usage: " << argv[0] << " <shm_name>\n"; 
        return 1; 
    }
    
    string name = "/tmp/" + string(argv[1]);

    size_t total_size = sizeof(ShmHeader) + RING_SIZE * sizeof(ShmMsg);

    int fd = open(name.c_str(), O_RDWR, 0600);
    if (fd < 0) { perror("open"); return 1; }
    
    void* base = mmap(nullptr, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (base == MAP_FAILED) { perror("mmap"); return 1; }

    auto hdr = reinterpret_cast<ShmHeader*>(base);
    auto msgs = reinterpret_cast<ShmMsg*>((char*)base + sizeof(ShmHeader));

    cout << "shm_sub_improved started on " << name << "\n";
    
    ExponentialBackoff backoff;
    uint64_t processed_count = 0;

    while (true) {
        // Check for new messages with exponential backoff
        uint64_t current_head = hdr->head.load(memory_order_acquire);
        uint64_t current_tail = hdr->tail.load(memory_order_acquire);
        
        if (current_tail < current_head) {
            // Process message
            uint64_t idx = current_tail % RING_SIZE;
            ShmMsg &m = msgs[idx];
            
            // Process: just consume the message
            hdr->tail.store(current_tail + 1, memory_order_release);
            processed_count++;
            backoff.reset();
            
            if (processed_count % 1000 == 0) {
                cout << "Processed " << processed_count << " messages\n";
            }
        } else {
            // No new messages, use exponential backoff
            backoff.wait();
        }
    }

    munmap(base, total_size);
    close(fd);
    return 0;
}
