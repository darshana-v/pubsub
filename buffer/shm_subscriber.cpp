// Usage: ./shm_subscriber <shm_name>
// Consumer polls ring buffer, echoes by updating t_ns to its timestamp.

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

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

struct ShmHeader {
    uint64_t head;
    uint64_t tail;
};

struct ShmMsg {
    uint64_t seq;
    uint64_t t_ns;
    char payload[MSG_SIZE - 16];
};

int main(int argc, char** argv) {
    if (argc < 2) { cerr << "Usage: " << argv[0] << " <shm_name>\n"; return 1; }
    string name = "/tmp/" + string(argv[1]);

    size_t total_size = sizeof(ShmHeader) + RING_SIZE * sizeof(ShmMsg);

    int fd = open(name.c_str(), O_RDWR, 0600);
    if (fd < 0) { perror("open"); return 1; }
    void* base = mmap(nullptr, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (base == MAP_FAILED) { perror("mmap"); return 1; }

    auto hdr = reinterpret_cast<ShmHeader*>(base);
    auto msgs = reinterpret_cast<ShmMsg*>((char*)base + sizeof(ShmHeader));

    cout << "shm_sub started on " << name << "\n";

    while (true) {
        // consume new messages
        if (hdr->tail < hdr->head) {
            uint64_t idx = hdr->tail % RING_SIZE;
            ShmMsg &m = msgs[idx];
            // process: just consume the message
            __atomic_thread_fence(__ATOMIC_ACQUIRE);
            hdr->tail++;
        } else {
            // no new messages, small pause to reduce busy spin
            this_thread::yield();
        }
    }

    munmap(base, total_size);
    close(fd);
    return 0;
}
