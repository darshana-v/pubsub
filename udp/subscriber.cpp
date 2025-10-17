// Usage: ./subscriber <listen_port>
// receives ping messages and immediately replies with the same struct
// updating t_ns to current time so publisher can measure RTT.

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <iostream>

using namespace std;
using ns = std::chrono::nanoseconds;
using clk = std::chrono::high_resolution_clock;

struct Msg {
    uint64_t seq;
    uint64_t t_ns;
};

int main(int argc, char** argv) {
    if (argc < 2) { cerr << "Usage: " << argv[0] << " <listen_port>\n"; return 1; }
    int port = stoi(argv[1]);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (::bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); return 1; }

    cout << "subscriber_udp listening on port " << port << "\n";
    while (true) {
        sockaddr_in src{};
        socklen_t srclen = sizeof(src);
        Msg m;
        ssize_t rec = recvfrom(sock, &m, sizeof(m), 0, (sockaddr*)&src, &srclen);
        if (rec <= 0) { perror("recvfrom"); break; }
        // set t_ns at reply time (so publisher can compute RTT)
        m.t_ns = (uint64_t)chrono::duration_cast<ns>(clk::now().time_since_epoch()).count();
        ssize_t sent = sendto(sock, &m, sizeof(m), 0, (sockaddr*)&src, srclen);
        if (sent != sizeof(m)) { perror("sendto"); break; }
    }

    close(sock);
    return 0;
}
