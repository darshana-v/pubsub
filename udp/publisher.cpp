// Usage: ./publisher <subscriber_ip> <subscriber_port> <count>
// sends timestamped ping messages, waits for pong and measures RTT.

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
using ns = chrono::nanoseconds;
using clk = chrono::high_resolution_clock;

struct Msg {
    uint64_t seq;
    uint64_t t_ns;
};

int main(int argc, char** argv) {
    if (argc < 4) {
        cerr << "Usage: " << argv[0] << " <subscriber_ip> <subscriber_port> <count>\n";
        return 1;
    }
    string sub_ip = argv[1];
    int sub_port = stoi(argv[2]);
    int count = stoi(argv[3]);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    // bind to ephemeral port to receive replies
    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons(0);
    if (::bind(sock, (sockaddr*)&local, sizeof(local)) < 0) { perror("bind"); return 1; }

    sockaddr_in sub{};
    sub.sin_family = AF_INET;
    inet_pton(AF_INET, sub_ip.c_str(), &sub.sin_addr);
    sub.sin_port = htons(sub_port);

    socklen_t sublen = sizeof(sub);
    vector<double> rtts;
    rtts.reserve(count);

    for (uint64_t i = 0; i < (uint64_t)count; ++i) {
        Msg m;
        m.seq = i;
        m.t_ns = (uint64_t)chrono::duration_cast<ns>(clk::now().time_since_epoch()).count();
        ssize_t sent = sendto(sock, &m, sizeof(m), 0, (sockaddr*)&sub, sublen);
        if (sent != sizeof(m)) { perror("sendto"); break; }

        // wait for reply (pong)
        Msg reply;
        ssize_t rec = recvfrom(sock, &reply, sizeof(reply), 0, nullptr, nullptr);
        if (rec < 0) { perror("recvfrom"); break; }
        auto now_ns = (uint64_t)chrono::duration_cast<ns>(clk::now().time_since_epoch()).count();
        double rtt_us = (now_ns - reply.t_ns) / 1000.0; // RTT in microseconds (assuming reply.t_ns is ping time at subscriber echo)
        rtts.push_back(rtt_us);
    }

    // stats
    double sum = 0;
    for (double v : rtts) sum += v;
    double avg = sum / rtts.size();
    cout << "UDP ping-pong count=" << rtts.size() << " avg_RTT_us=" << avg << " avg_one_way_us=" << (avg/2.0) << "\n";

    close(sock);
    return 0;
}
