// Usage: ./shm_publisher <shm_name> <count>
// Producer writes timestamped messages into ring buffer and waits for consumer echo via same ring by sequence.

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

struct ShmHeader {
    uint64_t head; // producer index
    uint64_t tail; // consumer index
    // messages follow
};

struct ShmMsg {
    uint64_t seq;
    uint64_t t_ns;
    char payload[MSG_SIZE - 16];
};

int main(int argc, char** argv) {
    if (argc < 3) { cerr << "Usage: " << argv[0] << " <shm_name> <count>\n"; return 1; }
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

    // initialize indices if first time (naively)
    hdr->head = 0;
    hdr->tail = 0;

    vector<double> rtts;
    rtts.reserve(count);

    for (uint64_t i = 0; i < (uint64_t)count; ++i) {
        // wait for space
        while ((hdr->head - hdr->tail) >= RING_SIZE) { /* busy-wait */ }
        uint64_t idx = hdr->head % RING_SIZE;
        ShmMsg &m = msgs[idx];
        m.seq = i;
        m.t_ns = (uint64_t)chrono::duration_cast<ns>(clk::now().time_since_epoch()).count();
        // publish (release)
        __atomic_thread_fence(__ATOMIC_RELEASE);
        hdr->head++;

        // wait for consumer to process and increment tail
        while (hdr->tail <= i) {
            this_thread::yield();
        }
        
        // Calculate RTT (simplified - just measure time since we sent)
        uint64_t now_ns = (uint64_t)chrono::duration_cast<ns>(clk::now().time_since_epoch()).count();
        double rtt_us = (now_ns - m.t_ns) / 1000.0;
        rtts.push_back(rtt_us);
    }

    double sum = 0;
    for (double v : rtts) sum += v;
    cout << "SHM pub count=" << rtts.size() << " avg_RTT_us=" << (sum / rtts.size()) << " avg_one_way_us=" << (sum / rtts.size() / 2.0) << "\n";

    munmap(base, total_size);
    close(fd);
    // leave shm object for consumer; remove if desired:
    // shm_unlink(name.c_str());
    return 0;
}
