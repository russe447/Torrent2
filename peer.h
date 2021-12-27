#ifndef PEER_H
#define PEER_H

#include <string>

using namespace std;

class peers {
public:
    int sock;
    int port;
    string address;
    bool am_choking;
    bool peer_choking;
    bool am_interested;
    bool peer_interested;
    std::string peer_id;
    int bitfield;
    //time struct for timeout to send keep alive;

    peers();
    bool setup(string addr, int p, string id);
    bool Send(uint8_t *data, size_t size);
    string receive(int size);
    string read();
};

#endif