#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <vector>

#include "peer.h"

peers::peers() {
    sock = -1;
    port = 0;
    address = "";
    am_choking = true;
    peer_choking = true;
    am_interested = false;
    peer_interested = false;
    peer_id = "";
    int bitfield;
    //time struct
}
    // Initializes the peer information using the ip, port, and peer id
bool peers::setup(string addr, int p, std::string id) {
    if (sock == -1) {

        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) {
            cout << "Socket failed\n";
            return false;
        }
    }   
    sockaddr_in h;
    h.sin_family = AF_INET;
    h.sin_port = htons(port);
    inet_pton(AF_INET, addr.c_str(), &h.sin_addr);

    port = p;
    address = addr;
    int conn = connect(sock, (sockaddr*)&h, sizeof(h));
    if (conn == -1) {
        return false;
    }
    peer_id = id;
    return true;
}
    // Send data to the socket connection
bool peers::Send(uint8_t *data, size_t size) {
    if (sock != -1) {

        if (send(sock, data, size, 0 ) < 0) {
            cout << "Sending failed : " << data << "\n";
            return false;
        }
    }
    else {
        return false;
    }
    return true;
}
    // Reads set amount of bytes from recv
string peers::receive(int size) {
    char buff[size];
    memset(&buff[0], 0, sizeof(buff));

    string reply;
    if (recv(sock, buff, size, 0) < 0) {
        cout << "receive failed\n";
        return nullptr;
    }
    buff[size-1] = '\0';
    reply = buff;
    return reply;
}
    // Same as receive but takes it byte-by-byte
string peers::read() {
    char buffer[1] = {0};
    string reply;
    while (buffer[0] != '\n' && buffer[0] != '\0') {
        if (recv(sock, buffer, sizeof(buffer), 0) < 0) {
            cout << "receive failed\n";
            return nullptr;
        }
        reply += buffer[0];
    }
    return reply;
}

