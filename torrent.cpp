#include <stdlib.h>
#include <string>
#include <sstream>
#include <poll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "torrent.h"
#include "peer.h"
#include "msg_protocol.h"
#include "block_t.h"

std::string Torrent::make_peer_id()
{
    time_t t;
    srand(time(&t));
    std::string id;

    for (unsigned i = 0; i < 20; i++)
        id.append(to_string(rand() % 10));

    return id;
}

Torrent::Torrent(char* metafile_path, int port)
{
    meta = Metafile::create_metafile(metafile_path);
    //meta.print_meta();
    TrackerConnector connector(meta.announce);
    std::string peer_id = make_peer_id();

    connector.request(meta.info_hash, (uint8_t*)peer_id.c_str(), port);

    vector<peer> coonector_peers = connector.get_peers();
    vector<peers> sock_peer;
    struct sockaddr_in servAddr; //Servers address made up of port afinet and inaddrany
    struct sockaddr_in clntAddr; // Client address
    socklen_t clntAddrLen;
    int sock, clntSock;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;                //IPV4 address family
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY); //any incoming
    servAddr.sin_port = htons(port);   
    bind(sock, (struct sockaddr *)&servAddr, sizeof(servAddr));
    listen(sock, 5);
    int fd_count = 0;
    int fd_size = 5;
    struct pollfd *pfds = (struct pollfd *)malloc(sizeof *pfds * fd_size);
    pfds[0].fd = sock;
    pfds[0].events = POLLIN;
    ssize_t numBytes;
    for(int i = 0; i < (int) coonector_peers.size(); i++){
        peers p;
        p.setup(coonector_peers[i].ip, coonector_peers[i].port, coonector_peers[i].peer_id);
        sock_peer.push_back(p);
        //sending handshake to each client
        MSG m;
        uint8_t* msg = (uint8_t*)malloc(sizeof(68));
        uint8_t* peerid = (uint8_t*)malloc(sizeof(p.peer_id.length()));
        for(int j = 0; j < (int) p.peer_id.length(); j++){
            peerid[j] = (uint8_t)p.peer_id[j];
        }
        size_t *size = NULL;

        m.handshake(msg, size, meta.info_hash, peerid);
        sock_peer[i].Send(msg, *size);
    }
    /*meta.pieces.size();
    uint16_t bitfield = 0;
    number of pieces = meta.pieces.size()
    bitfield's number of bits is the closest power of two that is larger thamn meta.pieces.size()
    The high bit in the first byte corresponds to piece index 0. Bits that are cleared indicated a missing piece,
    and set bits indicate a valid and available piece. Spare bits at the end are set to zero.*/
    vector<vector<block_t>> pieces(meta.pieces.size());

    while(1){
        poll(pfds, fd_count, -1);
        for (int i = 0; i < fd_count; i++){
            if (pfds[i].revents & POLLIN){ 

                if (pfds[i].fd == sock){
                    clntAddrLen = sizeof(clntAddr);
                    // Wait client to connect and accept
                    clntSock = accept(sock, (struct sockaddr *)&clntAddr, &clntAddrLen); //different clntsock for each client
                    if (fd_count == fd_size){
                        fd_size *= 2; // Double it
                        pfds = (pollfd *)realloc(pfds, sizeof(*pfds) * (fd_size));
                    }
                    pfds[fd_count].fd = clntSock;
                    pfds[fd_count].events = POLLIN; // Check ready-to-read
                    (fd_count)++;
                } else {
                    //getting lenght of message - first four bytes of message
                    int length;
                    char buf_length[4];
                    numBytes = 0;
                    while (numBytes < 4){
                        numBytes = recv(pfds[i].fd, buf_length, 4 - numBytes, 0);
                    }
                    length = ntohl(*((int *)(&buf_length[0])));
                    //getting the rest of the message 
                    char *buffer = (char *) malloc(length);
                    while (numBytes < length){
                        numBytes += recv(pfds[i].fd, buffer, length - numBytes, 0);
                    }
                    uint8_t *msg = (uint8_t*) malloc(length + 4);
                    //concatenating the strings from the two recieves to send to decode in msg_protocal.cpp
                    memcpy((char*)msg, strcat(buf_length, buffer), length + 4);
                    MSG m; //i think this can be unintilized because its not used in the function 
                    MSG retMsg; 
                    retMsg = m.decode(msg, (size_t)(length + 4));

                    if (retMsg.msgType == 4){
                        for (int k = 0; k < (int)sock_peer.size(); k++){
                            if (sock_peer[k].sock == pfds[i].fd){
                                break;
                            }
                        }
                        //change sock_peer[k].bifield; might not need to
                        if(1/*doesnt have piece - marked by bitfield for that index is 0*/){
                            MSG m;
                            uint8_t* msg = (uint8_t*)malloc(sizeof(17));
                            size_t *size = NULL;
                            //the mainline disconnected on requests larger than 2^14 (16KB);
                            if (meta.piece_length > 16384){
                                int acc = 0;
                                int left = meta.piece_length;
                                while(left >  16384){  //if piece is larger that 16KB must do in pieces until the remainder is smaller that 16KB
                                    //multiple requests will be needed to download a whole piece.
                                    m.request(msg, size, retMsg.index, acc, 16384);
                                    numBytes = 0;    
                                    while (numBytes < (int) *size){
                                        numBytes += send(pfds[i].fd, msg, (int) *size - numBytes, 0);
                                    }
                                    left -= 16384;
                                    acc += 16384;
                                }
                                //get remainder of the piece which is less that 16KB
                                m.request(msg, size, retMsg.index, acc, left);
                                numBytes = 0;    
                                while (numBytes < (int) *size){
                                    numBytes += send(pfds[i].fd, msg, (int) *size - numBytes, 0);
                                }

                            } else {
                                m.request(msg, size, retMsg.index, 0, meta.piece_length);
                                numBytes = 0;    
                                while (numBytes < (int) *size){
                                    numBytes += send(pfds[i].fd, msg, (int) *size - numBytes, 0);
                                }
                            }
                        }
                    } else if (retMsg.msgType == 6){
                        MSG m;
                        uint8_t* msg = (uint8_t*)malloc(sizeof(17));
                        size_t *size = NULL;
                        m.piece(msg, size, retMsg.index, retMsg.begin, pieces[retMsg.index], retMsg.blockLen);
                        numBytes = 0;    
                        while (numBytes < (int) *size){
                            numBytes += send(pfds[i].fd, msg, (int) *size - numBytes, 0);
                        }
                    }  else if (retMsg.msgType == 7) { // Piece

                        // Need to check first if the block_t already exists in the structure
                        // If it doesn't exist yet in the pieces structure, add it and then
                        // perform a check to see if all the blocks for the piece is there,
                        // we hash it and then check it with the corresponding hash from the
                        // metafile. If it matches, write it to the file at position
                        // piece_length*index and then set that vector to null in pieces
                        block_t p(retMsg.index, retMsg.begin, retMsg.block);

                        bool inserted = false;

                        for (size_t i = 0; i < pieces.size(); i++) {

                            //vector<block_t> temp = pieces[i];
                            // If the block received is part of the piece at this spot in the pieces list
                            if (pieces[i][0].index == p.index) {
                                // Check if the piece is already in this vector
                                for (size_t j = 0; j < pieces[i].size(); j++) {
                                    if (!inserted) {
                                        // If the offsets for the block match then we ditch this piece
                                        // and do nothing
                                        if (pieces[i][j].offset == p.offset) {
                                            break;
                                        }
                                        // If the offset of the block to insert is less than the current offset,
                                        // then insert here. If p offset is greater, its insertion point is further
                                        // down
                                        else if (temp[j].offset > p.offset {
                                            pieces[i].insert(pieces[i].begin()+j, p);
                                            inserted = true;
                                        }
                                    }
                                }
                                // If we inserted the block, then we check if all the blocks are
                                // there for the piece.
                                if (inserted) {

                                    unsigned char buf[meta.piece_length];
                                    unsigned char hsh[20];
                                    int e = 0;
                                    int piece = pieces[i][0].piece_index;

                                    for (size_t x = 0; x < pieces[i].size();) {
                                        
                                        // For each block, add the data to the buf
                                        for (int h = 0; h < pieces[i][x].data.size(); h++) {
                                            buf[e] = pieces[i][x].data[h];
                                            e++;
                                        }
                                    }
                                    // e should be the pieces length, should/could check
                                    // hash the data
                                    SHA1(buf, strlen(buf), hsh);
                                    // Check if hash matches the hash from the meta file
                                    string sha_string;
                                    sha_string.assign(hsh, 20);
                                    if(shar_string.compare(meta.pieces[piece]) == 0) {
                                        // If the hashes match, then we can write to the file and delete
                                        // the vector of blocks for that piece.
                                    }
                                    // If the hash doesn't match, then we either don't have all the data yet
                                    // Or there is an error in the hashing/making the string from the data
                                    else {
                                        
                                    }

                                }

                            }
                            
                        }

                    }
                    /*recieve a hand reply with bitfield
                    recv(4) length int;
                    recv(lenght) buffer length
                    decode MSG handshajke we send bitfile 
                    based type if else 
                    have 7 
                    respond and manipulate data structures;
                    figure out how to respond to each message
                    what request from who peer is not chonking and you are interested in a piece they hav e you would send a request*/
                }
            }
        }
    }
                        
}

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cout << "need <path> <port>" << std::endl;

        return -1;
    }

    int port;
    std::stringstream argv2;
    argv2 << argv[2];
    argv2 >> port;

    std::cout << argv[1] << std::endl;

    Torrent torrent(argv[1], port);

    return 0;
}
